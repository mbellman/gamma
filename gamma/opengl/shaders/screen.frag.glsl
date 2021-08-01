#version 460 core

uniform sampler2D screenTexture;

in vec2 fragUv;

layout (location = 0) out vec4 out_color;

void main() {
  out_color = texture(screenTexture, fragUv);
}