#version 460 core

layout (location = 2) out vec4 out_color_and_depth;

void main() {
  out_color_and_depth = vec4(vec3(1, 0, 1) * 0.2, gl_FragCoord.z);
}