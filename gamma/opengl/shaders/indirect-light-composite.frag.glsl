#version 460 core

// @todo use better names to differentiate these
#define USE_AVERAGE_INDIRECT_LIGHT 1
#define USE_INDIRECT_SKY_LIGHT 1

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

  #if USE_AVERAGE_INDIRECT_LIGHT == 1
    const int range = 2;
    vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
    int total_samples = 0;

    // average_indirect_light = texture(indirectLight, fragUv).rgb;

    for (int i = -range; i <= range; i += range) {
      for (int j = -range; j <= range; j += range) {
        vec2 coords = fragUv + texel_size * vec2(i, j);

        // Avoid sampling outside of the screen edges
        if (coords.x >= 0.001 && coords.x <= 0.999 && coords.y >= 0.001 && coords.y <= 0.999) {
          average_indirect_light += texture(indirectLight, coords).rgb;
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
    vec3 normal = texture(normalAndSpecularity, fragUv).xyz;

    indirect_sky_light = color * getIndirectSkyLightContribution(normal);
  #endif

  vec3 composite_color = average_indirect_light + indirect_sky_light;

  out_color_and_depth = vec4(composite_color, 0);
}