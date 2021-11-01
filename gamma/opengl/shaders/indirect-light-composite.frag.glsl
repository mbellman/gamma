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
#include "utils/helpers.glsl";
#include "utils/conversion.glsl";

const vec3 sky_sample_offsets[] = {
  vec3(0),
  vec3(1, 0, 0),
  vec3(-1, 0, 0),
  vec3(0, 1, 0),
  vec3(0, -1, 0),
  vec3(0, 0, 1),
  vec3(0, 0, -1)
};

vec3 getIndirectSkyLightContribution(vec3 fragment_normal) {
  // @todo pass in as a uniform
  const float indirect_sky_light_intensity = 0.5;
  vec3 contribution = vec3(0);

  for (int i = 0; i < 7; i++) {
    vec3 direction = normalize(1.1 * fragment_normal + sky_sample_offsets[i]);

    contribution += getSkyColor(direction) * indirect_sky_light_intensity;
  }

  return contribution / 7.0;
}

void main() {
  vec4 frag_normal_and_emissivity = texture(normalAndEmissivity, fragUv);
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
  vec3 fragment_albedo = frag_color_and_depth.rgb;
  float linear_fragment_depth = getLinearizedDepth(frag_color_and_depth.w);
  vec3 fragment_normal = frag_normal_and_emissivity.xyz;
  float emissivity = frag_normal_and_emissivity.w;
  vec3 average_indirect_light = vec3(0);
  vec3 indirect_sky_light = vec3(0);

  #if USE_AVERAGE_INDIRECT_LIGHT == 1
    // @todo extract into its own function
    const int range = 6;
    vec2 texel_size = 1.0 / screenSize;
    int total_samples = 0;

    // @todo it might be preferable to blur at half-resolution,
    // since we at least have to jump two pixels each time here
    // to read a different half-res indirect light buffer sample.
    for (int i = -range; i <= range; i += range) {
      for (int j = -range; j <= range; j += range) {
        vec2 sample_coords = fragUv + texel_size * vec2(i, j);
        float linear_sample_depth = getLinearizedDepth(texture(colorAndDepth, sample_coords).w);

        if (distance(linear_fragment_depth, linear_sample_depth) > 1.0) {
          // Avoid blurring across unconnected surfaces
          continue;
        }

        // Avoid sampling outside of the screen edges
        if (!isOffScreen(sample_coords, 0.001)) {
          vec4 indirect_light = texture(indirectLight, sample_coords);

          // Add global illumination term
          average_indirect_light += indirect_light.rgb;
          average_indirect_light -= indirect_light.w;
          total_samples++;
        }
      }
    }

    average_indirect_light /= float(total_samples);

    // if (!isOffScreen(fragUv, 0.001)) {
    //   // Subtract ambient occlusion term
    //   average_indirect_light -= texture(indirectLight, fragUv).w;
    // }
  #endif

  #if USE_INDIRECT_SKY_LIGHT == 1
    indirect_sky_light = fragment_albedo * getIndirectSkyLightContribution(fragment_normal);
  #endif

  vec3 composite_color = fragment_albedo * emissivity + average_indirect_light + indirect_sky_light;

  out_color_and_depth = vec4(composite_color, 0);
}