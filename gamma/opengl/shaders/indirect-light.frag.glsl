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

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(fract(1.0 + time)) * 3.141592 * 2.0;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

// @todo revise to use a sphere or normal-aligned hemisphere with precalculated sample points
// @todo distance-dependent sampling radius
// @todo @bug fix self-occluding surfaces
float getScreenSpaceAmbientOcclusionContribution(float fragment_depth) {
  const int TOTAL_SAMPLES = 10;
  const float radius = 15.0;
  vec3 contribution = vec3(0);
  vec2 texel_size = 1.0 / screenSize;
  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float occlusion = 0.0;

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
    float compared_depth = getLinearizedDepth(textureLod(colorAndDepth, fragUv + offset, 3).w);

    if (compared_depth < linearized_fragment_depth) {
      float occluder_distance = linearized_fragment_depth - compared_depth;

      occlusion += mix(1.0, 0.0, saturate(occluder_distance / 30.0));
    }
  }

  float average_occlusion = occlusion / float(TOTAL_SAMPLES);

  return -average_occlusion * 0.1;
}

vec3 getScreenSpaceGlobalIlluminationContribution(float fragment_depth, vec3 fragment_position, vec3 fragment_normal) {
  const int TOTAL_SAMPLES = 30;
  const float max_sample_radius = 1000.0;
  const float max_brightness = 100.0;
  vec2 texel_size = 1.0 / screenSize;
  vec3 global_illumination = vec3(0.0);

  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float radius = max_sample_radius * saturate(1.0 / (linearized_fragment_depth * 0.01));

  vec3 camera_to_fragment = normalize(fragment_position - cameraPosition);
  vec3 reflection_vector = reflect(camera_to_fragment, fragment_normal);
  vec3 world_sample_center = fragment_position + reflection_vector * 10.0;
  vec3 view_sample_center = glVec3(view * glVec4(world_sample_center));
  vec2 sample_center_uv = getScreenCoordinates(view_sample_center, projection);

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
    vec2 coords = sample_center_uv + offset;

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

/**
 * Determines whether a previous-frame temporal sample
 * has enough color to use for denoising. Fragments where
 * geometry was not rendered during the previous frame
 * will be black or near black, and we want to avoid
 * including these in the temporally averaged color. This
 * is mainly a problem where moving geometry edges meet
 * the sky, and reprojected samples are taken from points
 * where no geometry was rendered during an earlier frame,
 * and the sampled color would be invalid.
 */
bool isUsableTemporalSampleColor(vec3 color) {
  const float threshold = 0.04;

  return color.r >= threshold || color.g >= threshold || color.b >= threshold;
}

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);

  if (frag_color_and_depth.w == 1.0) {
    discard;
  }

  vec3 fragment_position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 fragment_normal = texture(normalAndEmissivity, fragUv).xyz;
  vec3 global_illumination = vec3(0.0);
  float ambient_occlusion = 0.0;

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    ambient_occlusion = getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w);
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

    if (isUsableTemporalSampleColor(sample_t1.rgb)) {
      out_gi_and_ao += sample_t1;
      total_temporal_samples++;
    }
  }

  if (!isOffScreen(fragUv_t2, 0.001)) {
    vec4 sample_t2 = texture(indirectLightT2, fragUv_t2);

    if (isUsableTemporalSampleColor(sample_t2.rgb)) {
      out_gi_and_ao += sample_t2;
      total_temporal_samples++;
    }
  }

  out_gi_and_ao /= float(total_temporal_samples);
}