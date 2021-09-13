#version 460 core

in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;
layout (location = 1) out vec4 out_normal_and_specularity;

void main() {
  out_color_and_depth = vec4(vec3(1, 0, 0), gl_FragCoord.z);
  out_normal_and_specularity = vec4(vec3(0), 1);
}