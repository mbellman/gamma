#version 460 core

#define USE_VARIABLE_PENUMBRA_SIZE 1

struct Light {
  vec3 position;
  float radius;
  vec3 color;
  float power;
  vec3 direction;
  float fov;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform sampler2D shadowMap;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform mat4 lightMatrix;

// @todo pass in as a uniform
const float indirect_light_factor = 0.01;

noperspective in vec2 fragUv;
flat in Light light;

layout (location = 0) out vec4 out_colorAndDepth;

@include('utils/gl.glsl');
@include('utils/conversion.glsl');
@include('utils/random.glsl');

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(1.0) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

float getLightFactor(vec3 position, float incidence, float light_distance) {
  vec4 transform = lightMatrix * glVec4(position);

  transform.xyz /= transform.w;
  transform.xyz *= 0.5;
  transform.xyz += 0.5;

  vec2 shadow_map_texel_size = 1.0 / vec2(1024.0);

  #if USE_VARIABLE_PENUMBRA_SIZE == 1
    // @todo max_spread based on light.radius
    float max_spread = 500.0;
    float spread = 1.0 + pow(light_distance / light.radius, 2) * max_spread;
  #else
    float spread = 3.0;
  #endif

  float factor = 0.0;

  for (int i = 0; i < 12; i++) {
    vec2 texel_offset = spread * rotatedVogelDisc(12, i) * shadow_map_texel_size;
    vec2 texel_coords = transform.xy + texel_offset;
    float occluder_distance = texture(shadowMap, texel_coords).r;
    float bias = max(0.00075 + (1.0 - incidence) * 0.001 - (light_distance / light.radius) * 0.01, 0.0);

    if (occluder_distance > transform.z - bias) {
      factor += 1.0;
    }
  }

  return factor / 12.0;
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w, fragUv, inverseProjection, inverseView);

  if (length(light.position - position) > light.radius) {
    // Discard any fragments outside of the light radius
    discard;
  }

  vec3 surface_to_light = light.position - position;
  float light_distance = length(surface_to_light);
  vec3 normalized_surface_to_light = surface_to_light / light_distance;
  vec3 normalized_surface_to_camera = normalize(cameraPosition - position);
  vec3 half_vector = normalize(normalized_surface_to_light + normalized_surface_to_camera);

  float direction_factor = dot(normalized_surface_to_light * -1, normalize(light.direction));
  float max_factor = 1.0;
  float min_factor = 1.0 - (light.fov / 180.0);

  if (direction_factor < min_factor) {
    // Discard any fragments outside of the spot light cone
    discard;
  }

  float range = max_factor - min_factor;
  float adjusted_factor = max(direction_factor - min_factor, 0.0);
  float spot_factor = sqrt(adjusted_factor / range);

  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  float incidence = max(dot(normalized_surface_to_light, normal), 0.0);
  float attenuation = pow(1.0 / light_distance, 2);
  float specularity = pow(max(dot(half_vector, normal), 0.0), 50);

  // Have light intensity 'fall off' toward radius boundary
  float hack_radial_influence = max(1.0 - light_distance / light.radius, 0.0);
  // Taper light intensity more softly to preserve light with distance
  float hack_soft_tapering = (20.0 * (light_distance / light.radius));
  // Loosely approximates ambient/indirect lighting
  vec3 hack_indirect_light = light.color * light.power * pow(max(1.0 - dot(normalized_surface_to_camera, normal), 0.0), 2) * indirect_light_factor;

  vec3 radiant_flux = light.color * light.power * light.radius;
  vec3 diffuse_term = radiant_flux * incidence * attenuation * hack_radial_influence * hack_soft_tapering + hack_indirect_light;
  vec3 specular_term = radiant_flux * specularity * attenuation;

  vec3 illuminated_color = color * spot_factor * (diffuse_term + specular_term);
  float light_factor = getLightFactor(position, incidence, light_distance);

  out_colorAndDepth = vec4(illuminated_color * light_factor, frag_colorAndDepth.w);
}