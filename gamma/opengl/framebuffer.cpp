#include <map>

#include "glew.h"
#include "opengl/framebuffer.h"

namespace Gamma {
  const static float defaultBorderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };

  const static std::map<ColorFormat, GLint> glInternalFormatMap = {
    { ColorFormat::R, GL_R32F },
    { ColorFormat::RG, GL_RG32F },
    { ColorFormat::RGB, GL_RGB32F },
    { ColorFormat::RGBA, GL_RGBA32F }
  };

  const static std::map<ColorFormat, GLenum> glFormatMap = {
    { ColorFormat::R, GL_R },
    { ColorFormat::RG, GL_RG },
    { ColorFormat::RGB, GL_RGB },
    { ColorFormat::RGBA, GL_RGBA }
  };

  void OpenGLFrameBuffer::init() {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }

  void OpenGLFrameBuffer::destroy() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteFramebuffers(1, &fbo);

    for (auto& attachment : colorAttachments) {
      glDeleteTextures(1, &attachment.textureId);
    }

    glDeleteTextures(1, &depthStencilTextureId);

    colorAttachments.clear();
  }

  void OpenGLFrameBuffer::addColorAttachment(ColorFormat format) {
    addColorAttachment(format, colorAttachments.size());
  }

  void OpenGLFrameBuffer::addColorAttachment(ColorFormat format, uint32 unit) {
    addColorAttachment(format, unit, GL_CLAMP_TO_BORDER);
  }

  void OpenGLFrameBuffer::addColorAttachment(ColorFormat format, uint32 unit, GLint clamp) {
    GLuint textureId;
    GLuint index = GL_COLOR_ATTACHMENT0 + colorAttachments.size();
    GLint glInternalFormat = glInternalFormatMap.at(format);
    GLenum glFormat = glFormatMap.at(format);

    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat, size.width, size.height, 0, glFormat, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp);
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, defaultBorderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, index, GL_TEXTURE_2D, textureId, 0);

    colorAttachments.push_back({ index, textureId, unit });
  }

  void OpenGLFrameBuffer::addDepthStencilAttachment() {
    glGenTextures(1, &depthStencilTextureId);
    glBindTexture(GL_TEXTURE_2D, depthStencilTextureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.width, size.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthStencilTextureId, 0);
  }

  void OpenGLFrameBuffer::bindColorAttachments() {
    GLuint* attachments = new GLuint[colorAttachments.size()];

    for (uint32 i = 0; i < colorAttachments.size(); i++) {
      attachments[i] = colorAttachments[i].index;
    }

    glDrawBuffers(colorAttachments.size(), attachments);

    delete[] attachments;
  }

  void OpenGLFrameBuffer::read() {
    for (uint32 i = 0; i < colorAttachments.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + colorAttachments[i].textureUnit);
      glBindTexture(GL_TEXTURE_2D, colorAttachments[i].textureId);
    }

    glBindBuffer(GL_READ_FRAMEBUFFER, fbo);
  }

  void OpenGLFrameBuffer::setSize(const Area<uint32>& size) {
    this->size = size;
  }

  void OpenGLFrameBuffer::write() {
    glBindBuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glViewport(0, 0, size.width, size.height);
  }
}