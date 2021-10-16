#version 460 core

#define USE_SCREEN_SPACE_AMBIENT_OCCLUSION 1
#define USE_SCREEN_SPACE_GLOBAL_ILLUMINATION 1

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
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
  vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
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

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);

  if (frag_color_and_depth.w == 1.0) {
    discard;
  }

  vec3 fragment_position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 fragment_normal = texture(normalAndSpecularity, fragUv).xyz;
  vec3 global_illumination = vec3(0.0);
  float ambient_occlusion = 0.0;

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    ambient_occlusion = getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w);
  #endif

  #if USE_SCREEN_SPACE_GLOBAL_ILLUMINATION == 1
    // @todo move to its own function
    const int TOTAL_SAMPLES = 30;
    const float max_sample_radius = 1000.0;
    const float min_sample_radius = 50.0;
    const float max_brightness = 50.0;

    vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
    float linearized_fragment_depth = getLinearizedDepth(frag_color_and_depth.w);
    // @todo define and use easeOut()
    float radius_distance_factor = sqrt(sqrt(linearized_fragment_depth / 500.0));
    float radius = mix(max_sample_radius, min_sample_radius, saturate(radius_distance_factor));

    for (int i = 0; i < TOTAL_SAMPLES; i++) {
      vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
      vec2 coords = fragUv + offset;

      if (isOffScreen(coords, 0.0)) {
        continue;
      }

      vec4 sample_color_and_depth = textureLod(colorAndDepth, fragUv + offset, 6);

      // Diminish illumination where the sample emits
      // less incident bounce light onto the fragment
      vec3 sample_position = getWorldPosition(sample_color_and_depth.w, fragUv + offset, inverseProjection, inverseView);
      float sample_distance = distance(fragment_position, sample_position);
      vec3 normalized_fragment_to_sample = (sample_position - fragment_position) / sample_distance;
      float incidence_factor = max(0.0, dot(fragment_normal, normalized_fragment_to_sample));

      // Diminish illumination with distance
      float distance_factor = max_brightness * saturate(1.0 / sample_distance);

      global_illumination += sample_color_and_depth.rgb * incidence_factor * distance_factor;
    }

    global_illumination /= float(TOTAL_SAMPLES);
  #endif

  vec3 fragment_position_t1 = glVec3(viewT1 * glVec4(fragment_position));
  vec3 fragment_position_t2 = glVec3(viewT2 * glVec4(fragment_position));
  vec2 fragUv_t1 = getScreenCoordinates(fragment_position_t1.xyz, projection);
  vec2 fragUv_t2 = getScreenCoordinates(fragment_position_t2.xyz, projection);
  int total_temporal_samples = 1;

  out_gi_and_ao = vec4(global_illumination, ambient_occlusion);

  if (!isOffScreen(fragUv_t1, 0.001)) {
    out_gi_and_ao += texture(indirectLightT1, fragUv_t1);
    total_temporal_samples++;
  }

  if (!isOffScreen(fragUv_t2, 0.001)) {
    out_gi_and_ao += texture(indirectLightT2, fragUv_t2);
    total_temporal_samples++;
  }

  out_gi_and_ao /= float(total_temporal_samples);
}