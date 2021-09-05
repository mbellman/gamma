#version 460 core

struct Light {
  vec3 position;
  float radius;
  vec3 color;
  float power;
  vec3 direction;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

// @todo pass in as a uniform
const float indirect_light_factor = 0.01;

noperspective in vec2 fragUv;
flat in Light light;

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

/**
 * Calculates the illuminated color of the pixel from
 * a single light's diffuse and specular contributions.
 */
vec3 getIlluminatedColor(Light light, vec3 position, vec3 normal, vec3 color) {
  vec3 adjustedLightColor = light.color * light.power * light.radius;
  vec3 surfaceToLight = light.position - position;
  float lightDistance = length(surfaceToLight);
  vec3 n_surfaceToLight = surfaceToLight / lightDistance;
  vec3 n_surfaceToCamera = normalize(cameraPosition - position);
  vec3 halfVector = normalize(n_surfaceToLight + n_surfaceToCamera);
  float incidence = max(dot(n_surfaceToLight, normal), 0.0);
  float attenuation = pow(1.0 / lightDistance, 2);
  float specularity = pow(max(dot(halfVector, normal), 0.0), 50);

  // Have light intensity 'fall off' toward radius boundary
  float hack_radial_influence = max(1.0 - lightDistance / light.radius, 0.0);
  // Taper light intensity more softly to preserve light with distance
  float hack_soft_tapering = (20.0 * (lightDistance / light.radius));
  // Loosely approximates ambient/indirect lighting
  vec3 hack_indirect_light = light.color * light.power * pow(max(1.0 - dot(n_surfaceToCamera, normal), 0.0), 2) * indirect_light_factor;

  vec3 diffuseTerm = adjustedLightColor * incidence * attenuation * hack_radial_influence * hack_soft_tapering + hack_indirect_light;
  vec3 specularTerm = adjustedLightColor * specularity * attenuation;

  return color * (diffuseTerm + specularTerm);
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w);

  if (length(light.position - position) > light.radius) {
    discard;
  }

  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  out_colorAndDepth = vec4(getIlluminatedColor(light, position, normal, color), frag_colorAndDepth.w);
}