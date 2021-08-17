#include "opengl/shadowmaps.h"

namespace Gamma {
  OpenGLDirectionalShadowMap::OpenGLDirectionalShadowMap(const Light* light) {
    lightId = light->id;

    buffer.init();
    buffer.addColorAttachment(ColorFormat::R, 3);  // Cascade 0 (GL_TEXTURE3)
    buffer.addColorAttachment(ColorFormat::R, 4);  // Cascade 1 (GL_TEXTURE4)
    buffer.addColorAttachment(ColorFormat::R, 5);  // Cascade 2 (GL_TEXTURE5)
    buffer.addDepthAttachment();
    buffer.bindColorAttachments();
  }

  // @todo
  OpenGLPointShadowMap::OpenGLPointShadowMap(const Light* light) {
    lightId = light->id;
  }

  // @todo
  OpenGLSpotShadowMap::OpenGLSpotShadowMap(const Light* light) {
    lightId = light->id;
  }
}