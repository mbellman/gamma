#pragma once

#include "system/entities.h"
#include "system/type_aliases.h"

namespace Gamma {
  class OpenGLMesh {
  public:
    OpenGLMesh(Mesh* mesh);
    ~OpenGLMesh();

    uint32 getId() const;
    void render();

  private:
    Mesh* sourceMesh = nullptr;
    GLuint vao;
    /**
     * Defines three buffers for instanced object attributes.
     *
     * [0] Vertex
     * [1] Matrix
     */
    GLuint buffers[2];
    GLuint ebo;
  };
}