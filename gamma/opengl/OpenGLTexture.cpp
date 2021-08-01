#include <string>

#include "opengl/OpenGLTexture.h"
#include "system/assert.h"

#include "glew.h"
#include "SDL_image.h"

namespace Gamma {
  OpenGLTexture::OpenGLTexture(const char* path, GLenum unit) {
    this->unit = unit;
    SDL_Surface* surface = IMG_Load(path);

    assert(surface != 0, "Failed to load texture: " + std::string(path));

    GLuint format = surface->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;

    glGenTextures(1, &id);

    use();

    glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    SDL_FreeSurface(surface);
  }

  OpenGLTexture::~OpenGLTexture() {
    glDeleteTextures(1, &id);
  }

  void OpenGLTexture::use() {
    glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id);
  }
}