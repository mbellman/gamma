#version 460 core

uniform mat4 projection;
uniform sampler2D color_and_depth;

layout (location = 2) out vec4 out_color_and_depth;

#include "utils/conversion.glsl";

vec2 getPixelCoords() {
  return gl_FragCoord.xy / vec2(1920.0, 1080.0);
}

void main() {
  float linearized_depth = getLinearizedDepth(gl_FragCoord.z);
  vec4 base_color = texture(color_and_depth, getPixelCoords());

  out_color_and_depth = vec4(vec3(1, 0, 1) * base_color.rgb, gl_FragCoord.z);
  gl_FragDepth = getFragDepth(linearized_depth + 1.0, projection);
}