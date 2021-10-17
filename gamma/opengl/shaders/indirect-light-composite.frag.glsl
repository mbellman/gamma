#version 460 core

// @todo use better names to differentiate these
#define USE_AVERAGE_INDIRECT_LIGHT 1
#define USE_INDIRECT_SKY_LIGHT 1

uniform vec2 screenSize;
uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform sampler2D indirectLight;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

#include "utils/random.glsl";

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
const float indirect_sky_light_intensity = 0.15;

// @todo allow shader imports; import this function
// from skybox helpers or similar. track shader dependencies
// as part of hot reloading
vec3 getSkyColor(vec3 direction) {
  vec3 sunDirection = normalize(vec3(0.3, 0.5, -1.0));
  vec3 sunColor = vec3(1.0, 0.3, 0.1);
  float sunBrightness = 10;
  float altitude = -0.5;

  float y = direction.y + altitude;
  float z = direction.z;

  float base_r = pow(0.5 + 0.5 * cos(y) * 0.8, 6);
  float base_g = pow(0.5 + 0.5 * cos(y) * 0.9, 7);
  float base_b = pow(0.5 + 0.5 * cos(y), 5);

  vec3 skylight = vec3(2 * pow(0.5 * cos(y) + 0.5, 50));
  vec3 sunlight = sunColor * sunBrightness * pow(max(dot(direction, sunDirection), 0.0), 100);
  vec3 atmosphere = 0.2 * (skylight + sunlight);

  return vec3(
    max(base_r + atmosphere.r, 0),
    max(base_g + 0.7 * atmosphere.g, 0),
    max(base_b + 0.4 * atmosphere.b, 0)
  );
}

vec3 getIndirectSkyLightContribution(vec3 fragment_normal) {
  vec3 contribution = vec3(0);

  for (int i = 0; i < 7; i++) {
    vec3 direction = normalize(1.1 * fragment_normal + sky_sample_offsets[i]);

    contribution += getSkyColor(direction) * indirect_sky_light_intensity;
  }

  return contribution / 7.0;
}

void main() {
  vec3 average_indirect_light = vec3(0);
  vec3 indirect_sky_light = vec3(0);
  vec3 fragment_normal = texture(normalAndSpecularity, fragUv).xyz;

  #if USE_AVERAGE_INDIRECT_LIGHT == 1
    const int range = 6;
    vec2 texel_size = 1.0 / screenSize;
    int total_samples = 0;

    for (int i = -range; i <= range; i += range) {
      for (int j = -range; j <= range; j += range) {
        vec2 sample_coords = fragUv + texel_size * vec2(i, j);
        vec3 sample_normal = texture(normalAndSpecularity, sample_coords).xyz;

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
    vec3 color = texture(colorAndDepth, fragUv).rgb;

    indirect_sky_light = color * getIndirectSkyLightContribution(fragment_normal);
  #endif

  vec3 composite_color = average_indirect_light + indirect_sky_light;

  out_color_and_depth = vec4(composite_color, 0);
}