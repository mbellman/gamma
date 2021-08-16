#version 460 core

uniform sampler2D screenTexture;

in vec2 fragUv;

layout (location = 0) out vec4 out_color;

void main() {
  // @todo support custom text/background color
  vec4 texture_color = texture(screenTexture, fragUv);

  out_color = texture_color;

  // if (texture_color.w == 0.0) {
  //   out_color = vec4(0, 0.5, 0, 0.3);
  // } else {
  //   out_color = texture_color * vec4(1, 0, 0, 1);
  // }
}