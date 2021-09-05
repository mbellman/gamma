#version 460 core

uniform sampler2D colorAndDepth;

noperspective in vec2 fragUv;

layout (location = 2) out vec4 out_colorAndDepth;

void main() {
  out_colorAndDepth = texture(colorAndDepth, fragUv);
}