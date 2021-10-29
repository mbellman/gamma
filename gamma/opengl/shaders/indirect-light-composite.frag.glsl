#version 460 core

// @todo use better names to differentiate these
#define USE_AVERAGE_INDIRECT_LIGHT 1
#define USE_INDIRECT_SKY_LIGHT 1

uniform vec2 screenSize;
uniform sampler2D colorAndDepth;
uniform sampler2D normalAndEmissivity;
uniform sampler2D indirectLight;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

#include "utils/random.glsl";
#include "utils/skybox.glsl";

const vec3 sky_sample_offsets[] = {
  vec3(0),
  vec3(1, 0, 0),
  vec3(-1, 0, 0),
  vec3(0, 1, 0),
  vec3(0, -1, 0),
  vec3(0, 0, 1),
  vec3(0, 0, -1)
};

// @todo pass in as a uniform
const float indirect_sky_light_intensity = 0.5;

vec3 getIndirectSkyLightContribution(vec3 fragment_normal) {
  vec3 contribution = vec3(0);

  for (int i = 0; i < 7; i++) {
    vec3 direction = normalize(fragment_normal + 2.0 * sky_sample_offsets[i]);

    contribution += getSkyColor(direction) * indirect_sky_light_intensity;
  }

  return contribution / 7.0;
}

void main() {
  vec4 frag_normal_and_emissivity = texture(normalAndEmissivity, fragUv);
  vec3 fragment_albedo = texture(colorAndDepth, fragUv).rgb;
  vec3 fragment_normal = frag_normal_and_emissivity.xyz;
  float emissivity = frag_normal_and_emissivity.w;
  vec3 average_indirect_light = vec3(0);
  vec3 indirect_sky_light = vec3(0);

  #if USE_AVERAGE_INDIRECT_LIGHT == 1
    // @todo extract into its own function
    const int range = 6;
    vec2 texel_size = 1.0 / screenSize;
    int total_samples = 0;

    for (int i = -range; i <= range; i += range) {
      for (int j = -range; j <= range; j += range) {
        vec2 sample_coords = fragUv + texel_size * vec2(i, j);
        vec3 sample_normal = texture(normalAndEmissivity, sample_coords).xyz;

        // Avoid blurring where the fragment and sample have
        // sufficiently different normals, which otherwise
        // causes unsightly color bleeding/ghosting
        //
        // @bug there are still edge cases; a luminance
        // comparison might be preferable here
        if (dot(fragment_normal, sample_normal) < 0.9) {
          continue;
        }

        // Avoid sampling outside of the screen edges
        if (
          sample_coords.x >= 0.001 &&
          sample_coords.x <= 0.999 &&
          sample_coords.y >= 0.001 &&
          sample_coords.y <= 0.999
        ) {
          average_indirect_light += texture(indirectLight, sample_coords).rgb;
          total_samples++;
        }
      }
    }

    average_indirect_light /= float(total_samples);

    if (fragUv.x >= 0.001 && fragUv.x <= 0.999 && fragUv.y >= 0.001 && fragUv.y <= 0.999) {
      average_indirect_light += texture(indirectLight, fragUv).w;
    }
  #endif

  #if USE_INDIRECT_SKY_LIGHT == 1
    indirect_sky_light = fragment_albedo * getIndirectSkyLightContribution(fragment_normal);
  #endif

  vec3 composite_color = fragment_albedo * emissivity + average_indirect_light + indirect_sky_light;

  out_color_and_depth = vec4(composite_color, 0);
}