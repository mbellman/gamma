#version 460 core

uniform sampler2D color;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

#include "utils/random.glsl";

void main() {
  vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);

  // vec3 average = vec3(0);

  // for (int i = -1; i <= 1; i++) {
  //   for (int j = -1; j <= 1; j++) {
  //     average += texture(color, fragUv + vec2(i, j) * texel_size + vec2(noise(1.0), noise(2.0)) * texel_size).rgb;
  //   }
  // }

  // average /= 9.0;

  out_color_and_depth = vec4(texture(color, fragUv).rgb, 0);
}