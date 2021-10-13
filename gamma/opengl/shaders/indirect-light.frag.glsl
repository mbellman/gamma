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

layout (location = 0) out vec4 out_gi_and_ao;

#include "utils/conversion.glsl";
#include "utils/random.glsl";
#include "utils/helpers.glsl";

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(fract(1.0 + time)) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

// @todo revise to use a sphere or normal-aligned hemisphere with precalculated sample points
// @todo distance-dependent sampling radius
// @todo @bug fix self-occluding surfaces
float getScreenSpaceAmbientOcclusionContribution(float fragment_depth) {
  const float radius = 15.0;
  vec3 contribution = vec3(0);
  vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float occlusion = 0.0;

  for (int i = 0; i < 10; i++) {
    vec2 offset = texel_size * radius * rotatedVogelDisc(10, i);
    float compared_depth = getLinearizedDepth(texture(colorAndDepth, fragUv + offset).w);

    if (compared_depth < linearized_fragment_depth) {
      float occluder_distance = linearized_fragment_depth - compared_depth;

      occlusion += saturate(mix(1.0, 0.0, occluder_distance / 30.0));
    }
  }

  float average_occlusion = occlusion / 10.0;

  return -average_occlusion * 0.1;
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
  vec3 global_illumination = vec3(0.0);
  float ambient_occlusion = 0.0;

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    ambient_occlusion = getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w);
  #endif

  #if USE_SCREEN_SPACE_GLOBAL_ILLUMINATION == 1
    // @todo move to its own function
    const int TOTAL_SAMPLES = 20;
    const float max_sample_radius = 250.0;
    const float min_sample_radius = 50.0;
    const float max_illumination_distance = 100.0;

    vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
    float linearized_fragment_depth = getLinearizedDepth(frag_color_and_depth.w);
    // @todo define and use easeOut()
    float radius_distance_factor = sqrt(sqrt(linearized_fragment_depth / 500.0));
    float radius = mix(max_sample_radius, min_sample_radius, saturate(radius_distance_factor));

    for (int i = 0; i < TOTAL_SAMPLES; i++) {
      vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
      vec2 coords = fragUv + offset;

      if (isOffScreen(coords)) {
        continue;
      }

      vec4 sample_color_and_depth = textureLod(colorAndDepth, fragUv + offset, 3);

      // Diminish illumination where the sample emits
      // less incident bounce light onto the fragment
      vec3 sample_position = getWorldPosition(sample_color_and_depth.w, fragUv + offset, inverseProjection, inverseView);
      vec3 fragment_to_sample = normalize(sample_position - fragment_position);
      float incidence_factor = 0.25 + 0.75 * max(0.0, dot(fragment_normal, fragment_to_sample));

      // Diminish illumination with distance, approximated
      // by the difference between fragment and sample depth
      float compared_depth = getLinearizedDepth(sample_color_and_depth.w);
      float occluder_distance = abs(compared_depth - linearized_fragment_depth);
      // @todo define and use easeOut() for distance ratio
      float distance_factor = mix(2.0, 0.0, saturate(occluder_distance / max_illumination_distance));

      global_illumination += sample_color_and_depth.rgb * incidence_factor * distance_factor;
    }

    global_illumination /= float(TOTAL_SAMPLES);
  #endif

  // @todo sample from temporally reprojected screen coordinates to fix ghosting
  // @todo we may need one more pass to reduce noise further in extreme cases
  vec3 global_illumination_t1 = texture(indirectLightT1, fragUv).rgb;
  vec3 global_illumination_t2 = texture(indirectLightT2, fragUv).rgb;

  vec3 final_global_illumination = (
    global_illumination +
    global_illumination_t1 +
    global_illumination_t2
  ) / 3.0;

  out_gi_and_ao = vec4(final_global_illumination, ambient_occlusion);
}