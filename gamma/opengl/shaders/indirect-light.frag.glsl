#version 460 core

#define USE_INDIRECT_SKY_LIGHT 1
#define USE_SCREEN_SPACE_AMBIENT_OCCLUSION 1
#define USE_SCREEN_SPACE_GLOBAL_ILLUMINATION 1  // @todo

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

// @todo pass in as a uniform
const float indirect_sky_light_intensity = 0.15;

@include('utils/conversion.glsl');
@include('utils/random.glsl');

// @todo allow shader imports; import this function
// from skybox helpers or similar. track shader dependencies
// as part of hot reloading
vec3 getSkyColor(vec3 direction) {
  vec3 sunDirection = normalize(vec3(0.3, 0.5, -1.0));
  vec3 sunColor = vec3(1.0, 0.3, 0.1);
  float sunBrightness = 10;
  float altitude = 0.6;

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

const vec3 sky_sample_offsets[] = {
  vec3(0),
  vec3(1, 0, 0),
  vec3(-1, 0, 0),
  vec3(0, 1, 0),
  vec3(0, -1, 0),
  vec3(0, 0, 1),
  vec3(0, 0, -1)
};

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(1.0) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

vec3 getIndirectSkyLightContribution(vec3 fragment_color, vec3 fragment_normal) {
  vec3 contribution = vec3(0);

  for (int i = 0; i < 7; i++) {
    vec3 direction = normalize(1.1 * fragment_normal + sky_sample_offsets[i]);

    contribution += fragment_color * getSkyColor(direction) * indirect_sky_light_intensity;
  }

  return contribution / 7.0;
}

vec3 getScreenSpaceAmbientOcclusionContribution(float fragment_depth) {
  vec3 contribution = vec3(0);
  vec2 texel_size = 1.0 / vec2(1920.0, 1080.0);
  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float occlusion = 0.0;
  const float radius = 25.0;

  for (int i = 0; i < 10; i++) {
    vec2 offset = rotatedVogelDisc(10, i) * radius;
    float compared_depth = getLinearizedDepth(texture(colorAndDepth, fragUv + offset * texel_size).w);

    // @todo use a distance or normal check to avoid face self-occlusion
    if (compared_depth < linearized_fragment_depth - 0.2) {
      occlusion += 1.0;
    }
  }

  float occlusion_factor = occlusion / 10.0;
  
  return vec3(-occlusion_factor) * 0.1;
}

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
  vec4 frag_normal_and_specularity = texture(normalAndSpecularity, fragUv);
  vec3 position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 normal = frag_normal_and_specularity.xyz;
  vec3 color = frag_color_and_depth.rgb;

  vec3 indirect_light = vec3(0);

  #if USE_INDIRECT_SKY_LIGHT == 1
    indirect_light += getIndirectSkyLightContribution(color, normal);
  #endif

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    indirect_light += getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w);
  #endif

  out_color_and_depth = vec4(indirect_light, frag_color_and_depth.w);
}