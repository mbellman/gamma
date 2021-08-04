#version 460 core

uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;

layout (location = 0) out vec3 out_color;

/**
 * Reconstructs the world position from pixel depth.
 */
vec3 getWorldPosition(float depth) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(fragUv * 2.0 - 1.0, z, 1.0);
  vec4 view = inverseProjection * clip;

  view /= view.w;

  vec4 world = inverseView * view;

  return world.xyz * vec3(1.0, 1.0, -1.0);
}

void main() {
  // @todo figure out how to calculate direction
  // from camera direction + fragUv
  vec3 position = getWorldPosition(1) - cameraPosition;
  vec3 direction = normalize(position);

  float intensity = pow(cos(direction.y), 20) + 2 * pow(max(sin(direction.z * 3.141592 * 0.5), 0.0), 300);

  intensity *= 0.3;

  float r = pow(cos(direction.y) * 0.8, 5) + intensity;
  float g = pow(cos(direction.y) * 0.9, 6) + 0.7 * intensity;
  float b = pow(cos(direction.y), 5) + 0.4 * intensity;

  out_color = vec3(r, g, b);
}