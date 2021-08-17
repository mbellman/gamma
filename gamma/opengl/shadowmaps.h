#pragma once

#include "opengl/framebuffer.h"
#include "opengl/shader.h"
#include "system/entities.h"

namespace Gamma {
  struct OpenGLBaseShadowMap {
    uint32 lightId = UNUSED_LIGHT_INDEX;
  };

  struct OpenGLDirectionalShadowMap : public OpenGLBaseShadowMap {
    OpenGLDirectionalShadowMap(const Light* light);

    OpenGLFrameBuffer buffer;
  };

  struct OpenGLPointShadowMap : public OpenGLBaseShadowMap {
    OpenGLPointShadowMap(const Light* light);

    OpenGLCubeMap buffer;
  };

  struct OpenGLSpotShadowMap : public OpenGLBaseShadowMap {
    OpenGLSpotShadowMap(const Light* light);

    OpenGLFrameBuffer buffer;
  };
}