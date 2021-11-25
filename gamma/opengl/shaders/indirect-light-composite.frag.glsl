#version 460 core

// @todo use better names to differentiate these
#define USE_AVERAGE_INDIRECT_LIGHT 1
#define USE_INDIRECT_SKY_LIGHT 1

uniform vec2 screenSize;
uniform sampler2D texColorAndDepth;
uniform sampler2D texNormalAndEmissivity;
uniform sampler2D texIndirectLight;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

#include "utils/random.glsl";
#include "utils/skybox.glsl";
#include "utils/helpers.glsl";
#include "utils/conversion.glsl";

const vec3 sky_sample_offsets[] = {
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

  for (int i = 0; i < 6; i++) {
    // @todo use roughness to determine sample offset range
    vec3 direction = normalize(0.2 * fragment_normal + sky_sample_offsets[i]);

    contribution += getSkyColor(direction) * indirect_sky_light_intensity;
  }

  return contribution / 6.0;
}

void main() {
  vec4 frag_normal_and_emissivity = texture(texNormalAndEmissivity, fragUv);
  vec4 frag_color_and_depth = texture(texColorAndDepth, fragUv);
  vec3 fragment_albedo = frag_color_and_depth.rgb;
  float linear_fragment_depth = getLinearizedDepth(frag_color_and_depth.w);
  vec3 fragment_normal = frag_normal_and_emissivity.xyz;
  float emissivity = frag_normal_and_emissivity.w;
  vec3 global_illumination = vec3(0);
  float ambient_occlusion = 0.0;
  vec3 indirect_sky_light = vec3(0);

  #if USE_AVERAGE_INDIRECT_LIGHT == 1
    vec4 indirect_light = texture(texIndirectLight, fragUv);

    global_illumination = indirect_light.rgb;
    ambient_occlusion = indirect_light.w;
  #endif

  #if USE_INDIRECT_SKY_LIGHT == 1
    // @todo if we apply indirect sky light in
    // the first available directional shadowcaster
    // shader, or in the directional light shader,
    // or in a pre-pass if no directional lights
    // exist, we can have sky-lit surfaces contribute
    // to global illumination.
    indirect_sky_light = fragment_albedo * getIndirectSkyLightContribution(fragment_normal);
  #endif

  vec3 composite_color = fragment_albedo * emissivity + global_illumination;

  // @bug this tints occluded regions correctly, but
  // produces excessive darkening in certain areas
  composite_color -= ambient_occlusion;

  out_color_and_depth = vec4(composite_color, frag_color_and_depth.w);
}