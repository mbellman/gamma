#version 460 core

#define USE_VARIABLE_PENUMBRA_SIZE 1

struct Light {
  vec3 position;
  float radius;
  vec3 color;
  float power;
  vec3 direction;
  float fov;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform sampler2D shadowMap;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform mat4 lightMatrix;

// @todo pass in as a uniform
const float indirect_light_factor = 0.01;

noperspective in vec2 fragUv;
flat in Light light;

layout (location = 0) out vec4 out_color_and_depth;

@include('utils/gl.glsl');
@include('utils/conversion.glsl');
@include('utils/random.glsl');

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(1.0) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

float getLightFactor(vec3 position, float incidence, float light_distance) {
  vec4 transform = lightMatrix * glVec4(position);

  transform.xyz /= transform.w;
  transform.xyz *= 0.5;
  transform.xyz += 0.5;

  vec2 shadow_map_texel_size = 1.0 / vec2(1024.0);

  #if USE_VARIABLE_PENUMBRA_SIZE == 1
    // @todo max_spread based on light.radius
    float max_spread = 500.0;
    float spread = 1.0 + pow(light_distance / light.radius, 2) * max_spread;
  #else
    float spread = 3.0;
  #endif

  float factor = 0.0;

  for (int i = 0; i < 12; i++) {
    vec2 texel_offset = spread * rotatedVogelDisc(12, i) * shadow_map_texel_size;
    vec2 texel_coords = transform.xy + texel_offset;
    float occluder_distance = texture(shadowMap, texel_coords).r;
    float bias = max(0.00075 + (1.0 - incidence) * 0.001 - (light_distance / light.radius) * 0.01, 0.0);

    if (occluder_distance > transform.z - bias) {
      factor += 1.0;
    }
  }

  return factor / 12.0;
}

void main() {
  @include('inline/spot-light.glsl');

  float light_factor = getLightFactor(position, incidence, light_distance);

  out_color_and_depth = vec4(illuminated_color * light_factor, frag_color_and_depth.w);
}