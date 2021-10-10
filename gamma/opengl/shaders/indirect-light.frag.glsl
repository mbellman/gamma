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
uniform float time;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

#include "utils/conversion.glsl";
#include "utils/random.glsl";
#include "utils/helpers.glsl";

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(fract(1.0 + time)) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

vec3 getScreenSpaceAmbientOcclusionContribution(float fragment_depth) {
  vec3 contribution = vec3(0);
  vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float occlusion = 0.0;
  const float radius = 15.0;

  for (int i = 0; i < 10; i++) {
    vec2 offset = texel_size * radius * rotatedVogelDisc(10, i);
    float compared_depth = getLinearizedDepth(texture(colorAndDepth, fragUv + offset).w);

    if (compared_depth < linearized_fragment_depth) {
      float occluder_distance = linearized_fragment_depth - compared_depth;

      occlusion += saturate(mix(1.0, 0.0, occluder_distance / 30.0));
    }
  }

  float average_occlusion = occlusion / 10.0;

  return vec3(-average_occlusion) * 0.1;
}

bool isOffScreen(vec2 uv) {
  return uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
}

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);

  if (frag_color_and_depth.w == 1.0) {
    discard;
  }

  vec3 fragment_position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 fragment_normal = texture(normalAndSpecularity, fragUv).xyz;
  vec3 indirect_light = vec3(0);

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    // @todo improve SSAO quality and fix issues with insufficient darkening
    indirect_light += getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w);
  #endif

  #if USE_SCREEN_SPACE_GLOBAL_ILLUMINATION == 1
    const int TOTAL_SAMPLES = 10;
    const float max_sample_radius = 250.0;
    const float min_sample_radius = 50.0;
    const float max_illumination_distance = 100.0;

    vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
    vec3 global_illumination = vec3(0);
    float linearized_fragment_depth = getLinearizedDepth(frag_color_and_depth.w);

    float radius = clamp(
      mix(max_sample_radius, min_sample_radius, linearized_fragment_depth / 500.0),
      min_sample_radius,
      max_sample_radius
    );

    for (int i = 0; i < TOTAL_SAMPLES; i++) {
      vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
      vec2 coords = fragUv + offset;

      if (isOffScreen(coords)) {
        continue;
      }

      vec4 color_and_depth = texture(colorAndDepth, fragUv + offset);

      // Diminish illumination where the sample emits
      // less incident bounce light onto the fragment
      vec3 world_position = getWorldPosition(color_and_depth.w, fragUv + offset, inverseProjection, inverseView);
      vec3 fragment_to_sample = normalize(world_position - fragment_position);
      float incidence_factor = 0.25 + 0.75 * max(dot(fragment_normal, fragment_to_sample), 0);

      // Diminish illumination with distance, approximated
      // by the difference between fragment and sample depth
      float compared_depth = getLinearizedDepth(color_and_depth.w);
      float occluder_distance = abs(compared_depth - linearized_fragment_depth);
      float distance_factor = saturate(mix(1.0, 0.0, occluder_distance / max_illumination_distance));

      global_illumination += color_and_depth.rgb * incidence_factor * distance_factor;
    }

    global_illumination /= float(TOTAL_SAMPLES);

    indirect_light += global_illumination;
  #endif

  // @todo sample from temporally reprojected screen coordinates to fix ghosting
  // @todo we may need one more pass to reduce noise further in extreme cases
  vec3 indirect_light_color_t1 = texture(indirectLightT1, fragUv).rgb;
  vec3 indirect_light_color_t2 = texture(indirectLightT2, fragUv).rgb;

  vec3 final_indirect_light = (indirect_light + indirect_light_color_t1 + indirect_light_color_t2) / 3.0;

  out_color_and_depth = vec4(final_indirect_light, frag_color_and_depth.w);
}