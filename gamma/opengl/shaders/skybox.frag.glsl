#version 460 core

uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_colorAndDepth;

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

vec3 getSkyColor(vec3 direction) {
  // @todo receive parameters as uniforms
  vec3 sunDirection = normalize(vec3(0.3, 0.5, -1.0));
  vec3 sunColor = vec3(1.0, 0.3, 0.1);
  float sunBrightness = 10;
  float altitude = 0.6;

  float y = direction.y + altitude;
  float z = direction.z;

  float base_r = pow(0.5 + 0.5 * cos(y) * 0.8, 6);
  float base_g = pow(0.5 + 0.5 * cos(y) * 0.9, 7);
  float base_b = pow(0.5 + 0.5 * cos(y), 5);

  vec3 skylight = vec3(2 * pow(0.5 * cos(y) + 0.5, 50));
  vec3 sunlight = sunColor * sunBrightness * pow(max(dot(direction, sunDirection), 0.0), 100);
  vec3 atmosphere = 0.2 * (skylight + sunlight);

  return vec3(
    max(base_r + atmosphere.r, 0),
    max(base_g + 0.7 * atmosphere.g, 0),
    max(base_b + 0.4 * atmosphere.b, 0)
  );
}

void main() {
  // @todo figure out how to calculate direction
  // from camera direction + fragUv
  vec3 position = getWorldPosition(1.0) - cameraPosition;
  vec3 direction = normalize(position);

  out_colorAndDepth = vec4(getSkyColor(direction), 1.0);
}