#include "glew.h"
#include "opengl/errors.h"
#include "opengl/OpenGLMesh.h"
#include "system/console.h"

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

    // This is incomplete test code for verifying selective LOD mesh rendering.
    //
    // @todo proper LOD determination based on object pool LOD groups
    // @todo glMultiDrawElementsIndirect for grouped LOD rendering
    uint32 firstIndex = 0;
    uint32 baseVertex = 0;
    uint32 baseInstance = 0;
    uint32 count = mesh.faceElements.size();

    if (mesh.firstIndexOffsets.size() > 0) {
      // uint32 lod = mesh.firstIndexOffsets.size() > 2 ? 1 : 0;
      uint32 lod = 0;

      firstIndex = mesh.firstIndexOffsets[lod];
      baseInstance = 0;
      count = mesh.firstIndexOffsets[lod + 1] - mesh.firstIndexOffsets[lod];
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glDrawElementsInstancedBaseVertexBaseInstance(
      primitiveMode,
      count,
      GL_UNSIGNED_INT,
      (void*)(firstIndex * sizeof(uint32)),
      mesh.objects.total(),
      baseVertex,
      baseInstance
    );
  }
}