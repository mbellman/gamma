#version 460 core

#define MAX_DIRECTIONAL_LIGHTS 10

struct DirectionalLight {
  vec3 color;
  float power;
  vec3 direction;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndEmissivity;
uniform vec3 cameraPosition;
uniform mat4 matInverseProjection;
uniform mat4 matInverseView;
uniform DirectionalLight lights[10];

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_colorAndDepth;

#include "utils/conversion.glsl";

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_normalAndEmissivity = texture(normalAndEmissivity, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w, fragUv, matInverseProjection, matInverseView);
  vec3 normal = frag_normalAndEmissivity.xyz;
  vec3 color = frag_colorAndDepth.rgb;
  float emissivity = frag_normalAndEmissivity.w;
  vec3 accumulatedColor = vec3(0.0);

  for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; i++) {
    DirectionalLight light = lights[i];

    #include "inline/directional-light.glsl";

    accumulatedColor += illuminated_color;
  }

  out_colorAndDepth = vec4(accumulatedColor * (1.0 - emissivity), frag_colorAndDepth.w);
}