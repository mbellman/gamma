#version 460 core

#define MAX_LIGHTS 1000

struct Light {
  vec3 position;
  float radius;
  vec3 color;
  float power;
};

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform int totalLights;

layout (std140, binding = 0) uniform Lights {
  Light lights[MAX_LIGHTS];
};

noperspective in vec2 fragUv;

layout (location = 0) out vec3 out_color;

vec3 getWorldPosition(float depth) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(fragUv * 2.0 - 1.0, z, 1.0);
  vec4 view = inverseProjection * clip;

  view /= view.w;

  vec4 world = inverseView * view;

  return world.xyz * vec3(1.0, 1.0, -1.0);
}

vec3 getIlluminationColor(Light light, vec3 position, vec3 normal, vec3 color) {
  vec3 surfaceToLight = light.position - position;
  float lightDistance = length(surfaceToLight);
  vec3 n_surfaceToLight = surfaceToLight / lightDistance;
  float incidence = max(dot(n_surfaceToLight, normal), 0.0);
  float attenuation = pow(1.0 / lightDistance, 2);
  vec3 diffuseTerm = light.color * light.radius * incidence * attenuation;

  vec3 n_surfaceToCamera = normalize(cameraPosition - position);
  vec3 halfVector = normalize(n_surfaceToLight + n_surfaceToCamera);
  float specularity = pow(max(dot(halfVector, normal), 0.0), 50);
  vec3 specularTerm = light.color * light.radius * attenuation * specularity;

  return color * (diffuseTerm + specularTerm);
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);

  vec3 position = getWorldPosition(frag_colorAndDepth.w);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  vec3 illuminationColor = vec3(0.0);

  for (int i = 0; i < totalLights; i++) {
    illuminationColor += getIlluminationColor(lights[i], position, normal, color);
  }

  out_color = illuminationColor;
}