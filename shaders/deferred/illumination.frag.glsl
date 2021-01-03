#version 460 core

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;

noperspective in vec2 fragUv;

layout (location = 0) out vec3 out_color;

struct Light {
  vec3 position;
  vec3 color;
  float radius;
};

vec3 getWorldPosition(float depth) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(fragUv * 2.0 - 1.0, z, 1.0);
  vec4 view = inverseProjection * clip;

  view /= view.w;

  vec4 world = inverseView * view;

  return world.xyz * vec3(1.0, 1.0, -1.0);
}

vec3 getIllumination(Light light, vec3 position, vec3 normal, vec3 color) {
  vec3 surfaceToLight = light.position - position;
  vec3 n_surfaceToLight = normalize(surfaceToLight);
  float lightDistance = length(surfaceToLight);
  float incidence = max(dot(n_surfaceToLight, normal), 0.0);
  float attenuation = pow(1.0 / lightDistance, 2);
  vec3 diffuseTerm = (light.color * light.radius * incidence * attenuation);

  vec3 n_surfaceToCamera = normalize(cameraPosition - position);
  vec3 halfVector = reflect(n_surfaceToLight, n_surfaceToCamera);
  float specularity = pow(max(dot(halfVector, normal), 0.0), 50);
  vec3 specularTerm = (light.color * light.radius * attenuation * specularity);

  return color * (diffuseTerm + specularTerm);
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);

  vec3 position = getWorldPosition(frag_colorAndDepth.w);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  Light light = Light(
    vec3(0.0, 50.0, 0.0),
    vec3(1.0, 0.5, 0.2),
    2000.0
  );

  out_color = getIllumination(light, position, normal, color);
}