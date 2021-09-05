#version 460 core

#define MAX_LIGHTS 10

struct DirectionalLight {
  vec3 color;
  float power;
  vec3 direction;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform DirectionalLight lights[10];

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_colorAndDepth;

@include('utils/conversion.glsl');

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w, fragUv, inverseProjection, inverseView);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;
  vec3 accumulatedColor = vec3(0.0);

  for (int i = 0; i < MAX_LIGHTS; i++) {
    DirectionalLight light = lights[i];

    // Diffuse lighting
    vec3 n_surfaceToLight = normalize(light.direction) * -1.0;
    vec3 n_surfaceToCamera = normalize(cameraPosition - position);
    vec3 halfVector = normalize(n_surfaceToLight + n_surfaceToCamera);
    float incidence = max(dot(n_surfaceToLight, normal), 0.0);
    float specularity = pow(max(dot(halfVector, normal), 0.0), 50);

    // Loosely approximates ambient/indirect lighting
    vec3 hack_ambient_light = light.color * light.power * pow(max(1.0 - dot(n_surfaceToCamera, normal), 0.0), 2) * 0.2;

    vec3 diffuseTerm = light.color * light.power * incidence + hack_ambient_light;
    vec3 specularTerm = light.color * light.power * specularity;

    accumulatedColor += color * (diffuseTerm + specularTerm);
  }

  out_colorAndDepth = vec4(accumulatedColor, frag_colorAndDepth.w);
}