#version 460 core

uniform sampler2D color_depth;
uniform sampler2D normal_specularity;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;

layout (location = 0) out vec3 color;

vec3 getWorldPosition(float depth) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(fragUv * 2.0 - 1.0, z, 1.0);
  vec4 view = inverseProjection * clip;

  view /= view.w;

  vec4 world = inverseView * view;

  return world.xyz * vec3(1.0, 1.0, -1.0);
}

void main() {
  vec4 frag_color_depth = texture(color_depth, fragUv);
  vec4 frag_normal_specularity = texture(normal_specularity, fragUv);

  // color = frag_color_depth.rgb;
  // color = vec3((frag_color_depth.w - 150.0) / 100.0, 0.0, 0.0);
  color = getWorldPosition(frag_color_depth.w);
}