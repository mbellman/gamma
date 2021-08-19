#include <algorithm>
#include <cmath>

#include "opengl/shadowmaps.h"
#include "math/constants.h"

namespace Gamma {
  const static float cascadeDepthRanges[3][2] = {
    { 1.0f, 200.0f },
    { 200.0f, 500.0f },
    { 500.0f, 1250.0f }
  };

  /**
   * OpenGLDirectionalShadowMap
   * --------------------------
   */
  OpenGLDirectionalShadowMap::OpenGLDirectionalShadowMap(const Light* light) {
    lightId = light->id;

    buffer.init();
    buffer.setSize({ 2048, 2048 });
    buffer.addColorAttachment(ColorFormat::R, 3);  // Cascade 0 (GL_TEXTURE3)
    buffer.addColorAttachment(ColorFormat::R, 4);  // Cascade 1 (GL_TEXTURE4)
    buffer.addColorAttachment(ColorFormat::R, 5);  // Cascade 2 (GL_TEXTURE5)
    buffer.addDepthStencilAttachment();
    buffer.bindColorAttachments();
  }

  Matrix4f OpenGLDirectionalShadowMap::createCascadedLightViewMatrix(uint8 cascade, const Vec3f& lightDirection, const Camera& camera) {
    // Determine the near and far ranges of the cascade volume
    float near = cascadeDepthRanges[cascade][0];
    float far = cascadeDepthRanges[cascade][1];

    // Determine cascade volume proportions/orientation
    float tanFov = tanf(0.5f * 90.0f * DEGREES_TO_RADIANS);
    float tanNear = tanFov * near;
    float tanFar = tanFov * far;
    Vec3f camera_forward = camera.orientation.getDirection();
    Vec3f camera_left = camera.orientation.getLeftDirection();
    Vec3f camera_up = camera.orientation.getUpDirection();
    Vec3f nearCenter = camera.position + camera_forward * near;
    Vec3f farCenter = camera.position + camera_forward * far;
    Vec3f volumeCenter = nearCenter + camera_forward * (far - near) * 0.5f;

    // Calculate cascade volume from the light's perspective
    Matrix4f lightLookAtMatrix = Matrix4f::lookAt(volumeCenter, lightDirection, Vec3f(0.0f, 1.0f, 0.0f));

    Vec3f v_near0 = lightLookAtMatrix * (nearCenter + (camera_left * tanNear) + (camera_up * tanNear));
    Vec3f v_near1 = lightLookAtMatrix * (nearCenter - (camera_left * tanNear) + (camera_up * tanNear));
    Vec3f v_near2 = lightLookAtMatrix * (nearCenter + (camera_left * tanNear) - (camera_up * tanNear));
    Vec3f v_near3 = lightLookAtMatrix * (nearCenter - (camera_left * tanNear) - (camera_up * tanNear));

    Vec3f v_far0 = lightLookAtMatrix * (farCenter + (camera_left * tanFar) + (camera_up * tanFar));
    Vec3f v_far1 = lightLookAtMatrix * (farCenter - (camera_left * tanFar) + (camera_up * tanFar));
    Vec3f v_far2 = lightLookAtMatrix * (farCenter + (camera_left * tanFar) - (camera_up * tanFar));
    Vec3f v_far3 = lightLookAtMatrix * (farCenter - (camera_left * tanFar) - (camera_up * tanFar));

    // Determine cascade volume bounds from the light's perspective
    float top = std::max({ v_near0.y, v_near1.y, v_near2.y, v_near3.y, v_far0.y, v_far1.y, v_far2.y, v_far3.y });
    float bottom = std::min({ v_near0.y, v_near1.y, v_near2.y, v_near3.y, v_far0.y, v_far1.y, v_far2.y, v_far3.y });
    float left = std::min({ v_near0.x, v_near1.x, v_near2.x, v_near3.x, v_far0.x, v_far1.x, v_far2.x, v_far3.x });
    float right = std::max({ v_near0.x, v_near1.x, v_near2.x, v_near3.x, v_far0.x, v_far1.x, v_far2.x, v_far3.x });
    float front = std::min({ v_near0.z, v_near1.z, v_near2.z, v_near3.z, v_far0.z, v_far1.z, v_far2.z, v_far3.z });
    float back = std::max({ v_near0.z, v_near1.z, v_near2.z, v_near3.z, v_far0.z, v_far1.z, v_far2.z, v_far3.z });

    Matrix4f projection = Matrix4f::orthographic(top, bottom, left, right, front - 1000.0f, back);
    Matrix4f view = Matrix4f::lookAt(volumeCenter.gl(), lightDirection.invert().gl(), Vec3f(0.0f, 1.0f, 0.0f));

    return (projection * view).transpose();
  }

  /**
   * OpenGLPointShadowMap
   * --------------------
   */
  // @todo
  OpenGLPointShadowMap::OpenGLPointShadowMap(const Light* light) {
    lightId = light->id;
  }

  /**
   * OpenGLSpotShadowMap
   * -------------------
   */
  // @todo
  OpenGLSpotShadowMap::OpenGLSpotShadowMap(const Light* light) {
    lightId = light->id;
  }
}