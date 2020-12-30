#version 460 core

uniform sampler2D color_depth;
uniform sampler2D normal_specularity;

noperspective in vec2 fragUv;

layout (location = 0) out vec3 color;

void main() {
  vec4 frag_color_depth = texture(color_depth, fragUv);

  color = vec3((frag_color_depth.w - 150.0) / 100.0, 0.0, 0.0);
  // color = frag_color_depth.rgb;
}