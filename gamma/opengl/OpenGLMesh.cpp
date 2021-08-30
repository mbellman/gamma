#include "opengl/errors.h"
#include "opengl/indirect_buffer.h"
#include "opengl/OpenGLMesh.h"
#include "system/console.h"

#include "glew.h"

namespace Gamma {
  const enum GLBuffer {
    VERTEX,
    MATRIX,
    COLOR
  };

  const enum GLAttribute {
    VERTEX_POSITION,
    VERTEX_NORMAL,
    VERTEX_TANGENT,
    VERTEX_UV,
    MODEL_MATRIX
  };

  OpenGLMesh::OpenGLMesh(const Mesh* mesh) {
    sourceMesh = mesh;

    glGenVertexArrays(1, &vao);
    glGenBuffers(2, &buffers[0]);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);

    auto& vertices = mesh->vertices;
    auto& faceElements = mesh->faceElements;

    // Buffer vertex data
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::VERTEX]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // Buffer vertex element data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, faceElements.size() * sizeof(uint32), faceElements.data(), GL_STATIC_DRAW);

    // Define vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::VERTEX]);

    glEnableVertexAttribArray(GLAttribute::VERTEX_POSITION);
    glVertexAttribPointer(GLAttribute::VERTEX_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(GLAttribute::VERTEX_NORMAL);
    glVertexAttribPointer(GLAttribute::VERTEX_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(GLAttribute::VERTEX_TANGENT);
    glVertexAttribPointer(GLAttribute::VERTEX_TANGENT, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));

    glEnableVertexAttribArray(GLAttribute::VERTEX_UV);
    glVertexAttribPointer(GLAttribute::VERTEX_UV, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    // Define matrix attributes
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::MATRIX]);

    for (uint32 i = 0; i < 4; i++) {
      glEnableVertexAttribArray(GLAttribute::MODEL_MATRIX + i);
      glVertexAttribPointer(GLAttribute::MODEL_MATRIX + i, 4, GL_FLOAT, GL_FALSE, sizeof(Matrix4f), (void*)(i * 4 * sizeof(float)));
      glVertexAttribDivisor(GLAttribute::MODEL_MATRIX + i, 1);
    }
  }

  OpenGLMesh::~OpenGLMesh() {
    // @todo
  }

  void OpenGLMesh::checkAndLoadTexture(std::string path, OpenGLTexture*& texture, GLenum unit) {
    if (path.size() > 0 && texture == nullptr) {
      // @todo use a texture factory/cache
      texture = new OpenGLTexture(path.c_str(), unit);
    }

    if (texture != nullptr) {
      texture->use();
    }
  }

  uint32 OpenGLMesh::getId() const {
    return sourceMesh->id;
  }

  uint16 OpenGLMesh::getObjectCount() const {
    return sourceMesh->objects.total();
  }

  const Mesh* OpenGLMesh::getSourceMesh() const {
    return sourceMesh;
  }

  bool OpenGLMesh::hasNormalMap() const {
    return glNormalMap != nullptr;
  }

  bool OpenGLMesh::hasTexture() const {
    return glTexture != nullptr;
  }

  bool OpenGLMesh::isMeshType(MeshType type) const {
    return sourceMesh->type == type;
  }

  void OpenGLMesh::render(GLenum primitiveMode) {
    auto& mesh = *sourceMesh;

    if (mesh.type != MeshType::REFRACTIVE) {
      // Don't bind textures for refractive objects, since in
      // the refractive geometry frag shader we need to read
      // from the G-Buffer color texture.
      //
      // @todo if we use texture units which won't conflict with
      // the G-Buffer, we can have textured refractive objects.
      checkAndLoadTexture(mesh.texture, glTexture, GL_TEXTURE0);
      checkAndLoadTexture(mesh.normalMap, glNormalMap, GL_TEXTURE1);
      checkAndLoadTexture(mesh.specularityMap, glSpecularityMap, GL_TEXTURE2);
    }

    // Buffer instance matrices
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::MATRIX]);
    glBufferData(GL_ARRAY_BUFFER, mesh.objects.total() * sizeof(Matrix4f), mesh.objects.getMatrices(), GL_DYNAMIC_DRAW);

    // Bind VAO/EBO and draw instances
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    if (mesh.lods.size() > 0) {
      // For meshes with levels of detail, generate draw
      // commands for mesh instances at each level of detail,
      // and dispatch them all together
      auto* commands = new GlDrawElementsIndirectCommand[mesh.lods.size()];

      for (uint32 i = 0; i < mesh.lods.size(); i++) {
        auto& command = commands[i];
        auto& lod = mesh.lods[i];

        command.count = lod.elementCount;
        command.firstIndex = lod.elementOffset;
        command.instanceCount = lod.instanceCount;
        command.baseInstance = lod.instanceOffset;
        command.baseVertex = lod.vertexOffset;
      }

      Gm_BufferDrawElementsIndirectCommands(commands, 2);

      glMultiDrawElementsIndirect(primitiveMode, GL_UNSIGNED_INT, 0, 2, 0);

      delete[] commands;
    } else {
      // No distinct level of detail meshes defined;
      // draw all mesh instances together
      glDrawElementsInstanced(primitiveMode, mesh.faceElements.size(), GL_UNSIGNED_INT, (void*)0, mesh.objects.total());
    }
  }
}