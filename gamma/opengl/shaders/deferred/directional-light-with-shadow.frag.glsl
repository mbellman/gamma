#version 460 core

#define VARIABLE_PENUMBRA_SIZE 1

struct DirectionalLight {
  vec3 color;
  float power;
  vec3 direction;
};

struct Cascade {
  int index;
  mat4 matrix;
  float bias;
  float spread_factor;
  float occluder_sweep_radius;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform sampler2D shadowMaps[3];
uniform mat4 lightMatrices[3];
uniform DirectionalLight light;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

const float cascade_depth_1 = 200.0;
const float cascade_depth_2 = 600.0;

// @todo move to gl helpers
vec3 glVec3(vec3 vector) {
  return vector * vec3(1, 1, -1);
}

vec4 glVec4(vec4 vector) {
  return vector * vec4(1, 1, -1, 1);
}

vec4 glVec4(vec3 vector) {
  return vec4(glVec3(vector), 1.0);
}

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

// @todo move to a helpers file; allow shader imports
float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  float z_near = 1.0;
  float z_far = 10000.0;

  return 2.0 * z_near * z_far / (z_far + z_near - clip_depth * (z_far - z_near));
}

/**
 * Returns a value within the range -1.0 - 1.0, constant
 * in screen space, acting as a noise filter.
 *
 * @todo move to helpers/allow shader imports
 */
float noise(float seed) {
  return 2.0 * (fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * seed * 43758.545312) - 0.5);
}

Cascade getCascadeByDepth(float linearized_depth) {
  if (linearized_depth < cascade_depth_1) {
    return Cascade(0, lightMatrices[0], 0.0005, 1000.0, 30.0);
  } else if (linearized_depth < cascade_depth_2) {
    return Cascade(1, lightMatrices[1], 0.002, 750.0, 20.0);
  } else {
    return Cascade(2, lightMatrices[2], 0.002, 800.0, 15.0);
  }
}

vec2 rotatedVogelDisk(int samples, int index) {
  float rotation = noise(1.0) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

float getClosestOccluder(sampler2D shadow_map, vec2 shadow_map_texel_size, vec4 transform, float occluder_sweep_radius) {
  const vec2 offsets[9] = {
    vec2(0.0),
    vec2(-1.0, 0.0),
    vec2(-1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, -1.0),
    vec2(0.0, -1.0),
    vec2(-1.0, 1.0)
  };

  float closest_occluder = transform.z;

  for (int i = 0; i < 9; i++) {
    vec2 texel_coords = transform.xy + occluder_sweep_radius * shadow_map_texel_size * offsets[i];
    float shadow_map_depth = texture(shadow_map, texel_coords).r;

    if (shadow_map_depth < closest_occluder) {
      closest_occluder = shadow_map_depth;
    }
  }

  return closest_occluder;
}

float getLightIntensity(Cascade cascade, vec4 transform) {
  if (transform.z > 0.999) {
    // If the position-to-light space transform depth
    // is out of range, we've sampled outside the
    // shadow map and can just render the fragment
    // with full illumination.
    return 1.0;
  }

  vec2 shadow_map_texel_size = 1.0 / textureSize(shadowMaps[cascade.index], 0);

  #if VARIABLE_PENUMBRA_SIZE == 1
    float closest_occluder = getClosestOccluder(shadowMaps[cascade.index], shadow_map_texel_size, transform, cascade.occluder_sweep_radius);
    float spread = 1.0 + cascade.spread_factor * pow(distance(transform.z, closest_occluder), 2);
  #else
    float spread = cascade.spread_factor / 500.0;
  #endif

  float light_intensity = 0.0;

  for (int i = 0; i < 16; i++) {
    vec2 texel_offset = spread * rotatedVogelDisk(16, i) * shadow_map_texel_size;
    vec2 texel_coords = transform.xy + texel_offset;
    float shadow_map_depth = texture(shadowMaps[cascade.index], texel_coords).r;

    if (shadow_map_depth > transform.z - (cascade.bias + spread * 0.00025)) {
      light_intensity += 1.0;
    }
  }

  return light_intensity / 16.0;
}

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
  vec4 frag_normal_and_specularity = texture(normalAndSpecularity, fragUv);
  vec3 position = getWorldPosition(frag_color_and_depth.w);
  vec3 normal = frag_normal_and_specularity.xyz;
  vec3 color = frag_color_and_depth.rgb;

  // Regular directional light calculations
  vec3 adjusted_light_color = light.color * light.power;
  vec3 normalized_surface_to_light = normalize(light.direction) * -1.0;
  vec3 normalized_surface_to_camera = normalize(cameraPosition - position);
  vec3 half_vector = normalize(normalized_surface_to_light + normalized_surface_to_camera);
  float incidence = max(dot(normalized_surface_to_light, normal), 0.0);
  float specularity = pow(max(dot(half_vector, normal), 0.0), 50);

  vec3 diffuse_term = adjusted_light_color * incidence;
  vec3 specular_term = adjusted_light_color * specularity;

  // Loosely approximates indirect lighting
  vec3 hack_ambient_light = color * adjusted_light_color * pow(max(1.0 - dot(normalized_surface_to_camera, normal), 0.0), 2) * 0.2;

  // Shadow/light intensity calculations
  Cascade cascade = getCascadeByDepth(getLinearizedDepth(frag_color_and_depth.w));
  vec4 shadow_map_transform = lightMatrices[cascade.index] * glVec4(position);
  
  shadow_map_transform.xyz /= shadow_map_transform.w;
  shadow_map_transform.xyz *= 0.5;
  shadow_map_transform.xyz += 0.5;

  float light_intensity = getLightIntensity(cascade, shadow_map_transform);

  out_color_and_depth = vec4(color * (diffuse_term + specular_term) * light_intensity + hack_ambient_light, frag_color_and_depth.w);
}