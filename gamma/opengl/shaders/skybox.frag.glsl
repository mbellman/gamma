#version 460 core

uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

// @todo remove test code
uniform samplerCube sky;
uniform bool useTexture;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_colorAndDepth;

#include "utils/conversion.glsl";
#include "utils/skybox.glsl";

void main() {
  // @todo figure out how to calculate direction
  // from camera direction + fragUv
  vec3 position = getWorldPosition(1.0, fragUv, inverseProjection, inverseView) - cameraPosition;
  vec3 direction = normalize(position);

  if (useTexture) {
    // @todo remove test code
    out_colorAndDepth = vec4(texture(sky, direction).rgb, 1.0);
  } else {
    out_colorAndDepth = vec4(getSkyColor(direction), 1.0);
  }
}