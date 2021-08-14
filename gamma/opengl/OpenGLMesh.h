#pragma once

#include <string>

#include "opengl/OpenGLTexture.h"
#include "system/entities.h"
#include "system/type_aliases.h"

namespace Gamma {
  class OpenGLMesh {
  public:
    OpenGLMesh(Mesh* mesh);
    ~OpenGLMesh();

    uint32 getId() const;
    uint16 getObjectCount() const;
    bool hasNormalMap() const;
    bool hasTexture() const;
    bool isMeshType(MeshType type) const;
    void render(GLenum primitiveMode);

  private:
    Mesh* sourceMesh = nullptr;
    GLuint vao;
    /**
     * Buffers for instanced object attributes.
     *
     * [0] Vertex
     * [1] Matrix
     */
    GLuint buffers[2];
    GLuint ebo;
    OpenGLTexture* glTexture = nullptr;
    OpenGLTexture* glNormalMap = nullptr;
    OpenGLTexture* glSpecularityMap = nullptr;

    void checkAndLoadTexture(std::string path, OpenGLTexture*& texture, GLenum unit);
  };
}