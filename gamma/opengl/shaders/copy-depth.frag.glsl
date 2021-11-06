#version 460 core

uniform sampler2D texColorAndDepth;

in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

void main() {
  float frag_depth = texture(texColorAndDepth, fragUv).w;

  out_color_and_depth = vec4(vec3(0), frag_depth);
}