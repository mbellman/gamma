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
uniform samplerCube shadowMap;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;
flat in Light light;

layout (location = 0) out vec4 out_colorAndDepth;

#include "utils/gl.glsl";
#include "utils/conversion.glsl";
#include "utils/random.glsl";

vec3 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(1.0) * 3.141592;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec3(cos(theta), sin(theta), 0.0);
}

float saturate(float value) {
  return clamp(value, 0.0, 1.0);
}

float getLightFactor(vec3 light_to_surface, float light_distance, float incidence) {
  #if USE_VARIABLE_PENUMBRA_SIZE == 1
    float max_spread = 200.0;
    float spread = 1.0 + pow(light_distance / light.radius, 2) * max_spread;
  #else
    float spread = 0.5;
  #endif
  
  float factor = 0.0;

  for (int i = 0; i < 7; i++) {
    vec3 sample_offset = spread * rotatedVogelDisc(7, i);
    float shadow_map_depth = texture(shadowMap, glVec3(light_to_surface + sample_offset)).r * light.radius;
    float bias = spread + pow(1.0 - incidence, 3) * 5.0;

    factor += shadow_map_depth > light_distance - bias ? 1.0 : 0.0;
  }

  return factor / 7.0;
}

void main() {
  #include "inline/point-light.glsl";

  float light_factor = getLightFactor(surface_to_light * -1.0, light_distance, incidence);

  out_colorAndDepth = vec4(illuminated_color * light_factor, frag_color_and_depth.w);
}