#version 460 core

#define USE_SCREEN_SPACE_AMBIENT_OCCLUSION 1
#define USE_SCREEN_SPACE_GLOBAL_ILLUMINATION 1

uniform vec2 screenSize;
uniform sampler2D colorAndDepth;
uniform sampler2D normalAndEmissivity;
uniform sampler2D indirectLightT1;
uniform sampler2D indirectLightT2;
uniform vec3 cameraPosition;
uniform mat4 view;
uniform mat4 inverseView;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 viewT1;
uniform mat4 viewT2;
uniform float time;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_gi_and_ao;

#include "utils/conversion.glsl";
#include "utils/random.glsl";
#include "utils/helpers.glsl";
#include "utils/gl.glsl";

const vec3[] ssao_sample_points = {
  vec3(0.063843, -0.07013, 0.031714),
  vec3(-0.134622, 0.03991, 0.068544),
  vec3(0.194556, 0.036847, 0.077114),
  vec3(-0.081489, -0.152912, 0.205436),
  vec3(-0.168704, 0.123969, 0.248587),
  vec3(0.22973, 0.120897, 0.279212),
  vec3(0.141791, 0.178289, 0.373517),
  vec3(0.018047, 0.460015, 0.178465),
  vec3(-0.376326, 0.185857, 0.355437),
  vec3(0.516018, -0.183684, 0.259855),
  vec3(-0.061337, 0.579925, 0.314374),
  vec3(-0.031821, 0.586648, 0.414045),
  vec3(0.082566, -0.726592, 0.256655),
  vec3(-0.503813, 0.102343, 0.653204),
  vec3(0.58586, 0.655299, 0.122507),
  vec3(-0.509251, -0.517294, 0.603104)
};

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(fract(1.0 + time)) * 3.141592 * 2.0;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

// @todo fix distance occlusion issues due to depth mipmap sampling
// @todo fix banding artifacts visible in occlusion density falloff
float getScreenSpaceAmbientOcclusionContribution(float fragment_depth, vec3 fragment_position, vec3 fragment_normal) {
  const int TOTAL_SAMPLES = 16;
  const float radius = 3.0;
  vec3 contribution = vec3(0);
  vec2 texel_size = 1.0 / screenSize;
  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float occlusion = 0.0;

  float t = fract(time);
  vec3 rvec = vec3(noise(1.0 + t), noise(2.0 + t), noise(3.0 + t));
  vec3 tangent = normalize(rvec - fragment_normal * dot(rvec, fragment_normal));
  vec3 bitangent = cross(fragment_normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, fragment_normal);

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    vec3 sample_offset = tbn * ssao_sample_points[i];
    vec3 world_sample_position = fragment_position + sample_offset * radius;
    vec3 fragment_to_sample = normalize(world_sample_position - fragment_position);

    vec3 view_sample_position = glVec3(view * glVec4(world_sample_position));
    vec2 screen_sample_position = getScreenCoordinates(view_sample_position, projection);
    float sample_depth = textureLod(colorAndDepth, screen_sample_position, 3).w;
    float linear_sample_depth = getLinearizedDepth(sample_depth);

    if (linear_sample_depth < view_sample_position.z) {
      float occluder_distance = view_sample_position.z - linear_sample_depth;

      occlusion += mix(1.0, 0.0, saturate(occluder_distance / 3.0));
    }
  }

  return occlusion / float(TOTAL_SAMPLES);
}

vec3 getScreenSpaceGlobalIlluminationContribution(float fragment_depth, vec3 fragment_position, vec3 fragment_normal) {
  const int TOTAL_SAMPLES = 30;
  const float max_sample_radius = 1000.0;
  const float max_brightness = 100.0;
  vec2 texel_size = 1.0 / screenSize;
  vec3 global_illumination = vec3(0.0);

  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float radius = max_sample_radius * saturate(1.0 / (linearized_fragment_depth * 0.01));

  // Bounce a ray off the surface and sample points
  // around the bounce ray screen coordinates
  vec3 camera_to_fragment = normalize(fragment_position - cameraPosition);
  vec3 reflection_vector = reflect(camera_to_fragment, fragment_normal);
  vec3 world_bounce_ray = fragment_position + reflection_vector * 10.0;
  vec3 view_bounce_ray = glVec3(view * glVec4(world_bounce_ray));
  vec2 bounce_ray_coords = getScreenCoordinates(view_bounce_ray, projection);

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
    vec2 coords = bounce_ray_coords + offset;

    if (isOffScreen(coords, 0.0)) {
      continue;
    }

    vec4 sample_color_and_depth = textureLod(colorAndDepth, coords, 3);

    // @todo why is this necessary? why are certain sample
    // color components < 0? note: this is to do with
    // probe reflectors
    sample_color_and_depth.r = saturate(sample_color_and_depth.r);
    sample_color_and_depth.g = saturate(sample_color_and_depth.g);
    sample_color_and_depth.b = saturate(sample_color_and_depth.b);

    // Diminish illumination where the sample emits
    // less incident bounce light onto the fragment
    vec3 sample_position = getWorldPosition(sample_color_and_depth.w, fragUv + offset, inverseProjection, inverseView);
    float sample_distance = distance(fragment_position, sample_position);
    vec3 normalized_fragment_to_sample = (sample_position - fragment_position) / sample_distance;
    float incidence_factor = max(0.0, dot(fragment_normal, normalized_fragment_to_sample));

    // Diminish illumination with distance
    float distance_factor = max_brightness * saturate(1.0 / sample_distance) * saturate(linearized_fragment_depth / 100.0);

    global_illumination += sample_color_and_depth.rgb * incidence_factor * distance_factor;
  }

  return global_illumination / float(TOTAL_SAMPLES);
}

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
  vec3 fragment_position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 fragment_normal = texture(normalAndEmissivity, fragUv).xyz;
  vec3 global_illumination = vec3(0.0);
  float ambient_occlusion = 0.0;

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    ambient_occlusion = getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w, fragment_position, fragment_normal);
  #endif

  #if USE_SCREEN_SPACE_GLOBAL_ILLUMINATION == 1
    global_illumination = getScreenSpaceGlobalIlluminationContribution(frag_color_and_depth.w, fragment_position, fragment_normal);
  #endif

  vec3 fragment_position_t1 = glVec3(viewT1 * glVec4(fragment_position));
  vec3 fragment_position_t2 = glVec3(viewT2 * glVec4(fragment_position));
  vec2 fragUv_t1 = getScreenCoordinates(fragment_position_t1.xyz, projection);
  vec2 fragUv_t2 = getScreenCoordinates(fragment_position_t2.xyz, projection);
  int total_temporal_samples = 1;

  out_gi_and_ao = vec4(global_illumination, ambient_occlusion);

  if (!isOffScreen(fragUv_t1, 0.001)) {
    vec4 sample_t1 = texture(indirectLightT1, fragUv_t1);

    out_gi_and_ao += sample_t1;
    total_temporal_samples++;
  }

  if (!isOffScreen(fragUv_t1, 0.001)) {
    vec4 sample_t2 = texture(indirectLightT2, fragUv_t2);

    out_gi_and_ao += sample_t2;
    total_temporal_samples++;
  }

  out_gi_and_ao /= float(total_temporal_samples);
}