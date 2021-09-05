#version 460 core

#define USE_VARIABLE_PENUMBRA_SIZE 1

struct Light {
  vec3 position;
  float radius;
  vec3 color;
  float power;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform samplerCube shadowMap;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform Light light;

// @todo pass in as a uniform
const float indirect_light_factor = 0.01;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_colorAndDepth;

@include('utils/gl.glsl');
@include('utils/conversion.glsl');
@include('utils/random.glsl');

vec3 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(1.0) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec3(cos(theta), sin(theta), 0.0);
}

float saturate(float value) {
  return clamp(value, 0.0, 1.0);
}

float getLightFactor(vec3 light_to_surface, float light_distance, float incidence) {
  #if USE_VARIABLE_PENUMBRA_SIZE == 1
    float max_spread = light.radius * mix(0.01, 1.0, saturate(light.radius / 10000.0));
    float spread = pow(light_distance / light.radius, 2) * max_spread;
  #else
    float spread = 0.5;
  #endif
  
  float factor = 0.0;

  for (int i = 0; i < 7; i++) {
    vec3 sample_offset = spread * rotatedVogelDisc(7, i);
    float shadow_map_depth = texture(shadowMap, glVec3(light_to_surface + sample_offset)).r * light.radius;
    float bias = spread + pow(1.0 - incidence, 3) * 5.0;

    factor += shadow_map_depth > light_distance - bias ? 1.0 : 0.0;
  }

  return factor / 7.0;
}

/**
 * Calculates the illuminated color of the pixel from
 * a single light's diffuse and specular contributions.
 */
vec3 getIlluminatedColor(Light light, vec3 position, vec3 normal, vec3 color) {
  vec3 radiant_flux = light.color * light.power * light.radius;
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

  vec3 diffuseTerm = radiant_flux * incidence * attenuation * hack_radial_influence * hack_soft_tapering + hack_indirect_light;
  vec3 specularTerm = radiant_flux * specularity * attenuation;

  float light_factor = getLightFactor(surfaceToLight * -1.0, lightDistance, incidence);

  return color * (diffuseTerm + specularTerm) * light_factor;
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w, fragUv, inverseProjection, inverseView);

  if (length(light.position - position) > light.radius) {
    discard;
  }

  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  out_colorAndDepth = vec4(getIlluminatedColor(light, position, normal, color), frag_colorAndDepth.w);
}