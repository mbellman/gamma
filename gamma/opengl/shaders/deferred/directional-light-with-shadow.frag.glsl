#version 460 core

struct DirectionalLight {
  vec3 color;
  float power;
  vec3 direction;
};

struct Cascade {
  int index;
  mat4 matrix;
  float bias;
};

uniform sampler2D shadowMaps[3];
uniform mat4 lightMatrices[3];

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform DirectionalLight light;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_color_and_depth;

const float cascade_depth_1 = 200.0;
const float cascade_depth_2 = 600.0;

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

// @todo move to a helpers file; allow shader imports
float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  float z_near = 1.0;
  float z_far = 10000.0;

  return 2.0 * z_near * z_far / (z_far + z_near - clip_depth * (z_far - z_near));
}

/**
 * Returns a value within the range -1.0 - 1.0, constant
 * in screen space, acting as a noise filter.
 *
 * @todo move to helpers/allow shader imports
 */
float noise(float seed) {
  return 2.0 * (fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * seed * 43758.545312) - 0.5);
}

Cascade getCascadeByDepth(float linearized_depth) {
  if (linearized_depth < cascade_depth_1) {
    return Cascade(0, lightMatrices[0], 0.001);
  } else if (linearized_depth < cascade_depth_2) {
    return Cascade(1, lightMatrices[1], 0.001);
  } else {
    return Cascade(2, lightMatrices[2], 0.001);
  }
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_normalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w);
  vec3 normal = frag_normalAndSpecularity.xyz;
  vec3 color = frag_colorAndDepth.rgb;

  Cascade cascade = getCascadeByDepth(getLinearizedDepth(frag_colorAndDepth.w));
  vec4 shadow_map_transform = lightMatrices[cascade.index] * glVec4(position);
  
  shadow_map_transform.xyz /= shadow_map_transform.w;
  shadow_map_transform.xyz *= 0.5;
  shadow_map_transform.xyz += 0.5;

  // @todo do a close-proximity sweep to determine
  // whether we can do an early bail-out
  const vec2 sweep[9] = {
    vec2(0.0, 0.0),
    vec2(-1.0, 0.0),
    vec2(-1.0, 1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, -1.0),
    vec2(0.0, -1.0),
    vec2(-1.0, -1.0)
  };

  // @todo cleanup/factor into its own function
  vec2 shadow_map_size = 1.0 / textureSize(shadowMaps[cascade.index], 0);
  float light_intensity = 1.0;

  if (shadow_map_transform.z < 0.999) {
    for (int i = 0; i < 9; i++) {
      vec2 texel_offset = 1.0 * sweep[i] * shadow_map_size;
      vec2 random_offset = 0.3 * normalize(vec2(noise(1.0), noise(2.0))) * shadow_map_size;
      vec2 texel_coords = shadow_map_transform.xy + random_offset + texel_offset;
      float shadow_map_depth = texture(shadowMaps[cascade.index], texel_coords).r;

      if (shadow_map_depth > shadow_map_transform.z - cascade.bias) {
        light_intensity += 1.0;
      }
    }

    light_intensity /= 9.0;
  }

  // Diffuse lighting
  vec3 n_surfaceToLight = normalize(light.direction) * -1.0;
  vec3 n_surfaceToCamera = normalize(cameraPosition - position);
  vec3 halfVector = normalize(n_surfaceToLight + n_surfaceToCamera);
  float incidence = max(dot(n_surfaceToLight, normal), 0.0);
  float specularity = pow(max(dot(halfVector, normal), 0.0), 50);

  vec3 diffuseTerm = light.color * light.power * incidence;
  vec3 specularTerm = light.color * light.power * specularity;

  // Loosely approximates ambient/indirect lighting
  vec3 hack_ambient_light = color * light.color * light.power * pow(max(1.0 - dot(n_surfaceToCamera, normal), 0.0), 2) * 0.2;

  out_color_and_depth = vec4(color * (diffuseTerm + specularTerm) * light_intensity + hack_ambient_light, frag_colorAndDepth.w);
}