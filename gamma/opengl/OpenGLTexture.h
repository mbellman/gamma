#pragma once

#include "system/type_aliases.h"

namespace Gamma {
  class OpenGLTexture {
  public:
    OpenGLTexture(const char* path, GLenum unit);
    ~OpenGLTexture();

    void use();

  private:
    GLuint id;
    GLenum unit;
  };
}