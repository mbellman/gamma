#pragma once

#include <vector>

#include "math/plane.h"
#include "system/traits.h"
#include "system/type_aliases.h"

namespace Gamma {
  struct ColorAttachment {
    GLuint index;
    GLuint textureId;
    GLenum textureUnit;
  };

  enum ColorFormat {
    R,
    RG,
    RGB,
    RGBA
  };

  class OpenGLFrameBuffer : public Initable, public Destroyable {
  public:
    virtual void init() override;
    virtual void destroy() override;
    void addColorAttachment(ColorFormat format);
    void addColorAttachment(ColorFormat format, uint32 unit);
    void addColorAttachment(ColorFormat format, uint32 unit, GLint clamp);
    void addDepthStencilAttachment();
    void bindColorAttachments();
    void read();
    void setSize(const Area<uint32>& size);
    void shareDepthStencilAttachment(const OpenGLFrameBuffer& target);
    void write();

  private:
    GLuint fbo = 0;
    GLuint depthStencilTextureId = 0;
    std::vector<ColorAttachment> colorAttachments;
    Area<uint32> size;
  };

  class OpenGLCubeMap {
  public:
    // @TODO

  private:
    GLuint fbo = 0;
    GLenum unit;
  };
}