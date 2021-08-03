#version 460 core

struct Light {
  vec3 position;
  float radius;
  vec3 color;
  float power;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;
flat in Light light;

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

/**
 * Calculates the illuminated color of the pixel from
 * a single light's diffuse and specular contributions.
 */
vec3 getIlluminatedColor(Light light, vec3 position, vec3 normal, vec3 color) {
  vec3 surfaceToLight = light.position - position;
  float lightDistance = length(surfaceToLight);
  vec3 n_surfaceToLight = surfaceToLight / lightDistance;
  float incidence = max(dot(n_surfaceToLight, normal), 0.0);
  float influence = max(1.0 - lightDistance / light.radius, 0.0);
  float attenuation = pow(1.0 / lightDistance, 2);
  vec3 diffuseTerm = light.color * light.power * light.radius * incidence * attenuation * sqrt(influence);

  vec3 n_surfaceToCamera = normalize(cameraPosition - position);
  vec3 halfVector = normalize(n_surfaceToLight + n_surfaceToCamera);
  float specularity = pow(max(dot(halfVector, normal), 0.0), 50);
  vec3 specularTerm = light.color * light.power * light.radius * specularity * attenuation;

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

  out_color = getIlluminatedColor(light, position, normal, color);
}