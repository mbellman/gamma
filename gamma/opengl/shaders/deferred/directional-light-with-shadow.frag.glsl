#version 460 core

struct DirectionalLight {
  vec3 color;
  float power;
  vec3 direction;
};

uniform sampler2D cascades[3];
uniform mat4 lightMatrices[3];

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform DirectionalLight light;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

// @todo move to gl helpers
vec3 glVec3(vec3 vector) {
  return vector * vec3(1, 1, -1);
}

vec4 glVec4(vec4 vector) {
  return vector * vec4(1, 1, -1, 1);
}

vec4 glVec4(vec3 vector) {
  return vec4(glVec3(vector), 1.0);
}

/**
 * Reconstructs the world position from pixel depth.
 */
vec3 getWorldPosition(float depth) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(fragUv * 2.0 - 1.0, z, 1.0);
  vec4 view = inverseProjection * clip;

  view /= view.w;

  vec4 world = inverseView * view;

  return world.xyz * vec3(1.0, 1.0, -1.0);
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  // @todo get appropriate light matrix + cascade
  vec4 shadow_map_position = lightMatrices[0] * glVec4(position);
  vec3 shadow_map_transform = shadow_map_position.xyz / shadow_map_position.w;

  shadow_map_transform.xyz *= 0.5;
  shadow_map_transform.xyz += 0.5;

  float shadow_map_depth = texture(cascades[0], shadow_map_transform.xy).r;

  // @todo define depth-appropriate bias, use filtering
  if (shadow_map_transform.z < 0.999 && shadow_map_depth < shadow_map_transform.z - 0.001) {
    discard;
  }

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

  out_color_and_depth = vec4(color * (diffuseTerm + specularTerm), frag_colorAndDepth.w);
}