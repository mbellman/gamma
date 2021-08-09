#version 460 core

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 view;
uniform mat4 inverseView;
uniform mat4 projection;
uniform mat4 inverseProjection;

noperspective in vec2 fragUv;

layout (location = 0) out vec3 out_color;

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

// @todo allow shader imports; import this function
// from skybox helpers or similar. track shader dependencies
// as part of hot reloading
vec3 getSkyColor(vec3 direction) {
  vec3 sunDirection = normalize(vec3(0.5, 0.3, 1.0));
  vec3 sunColor = vec3(1.0, 0.1, 0.2);
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

float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  float near = 1.0;
  float far = 10000.0;

  return 2.0 * near * far / (far + near - clip_depth * (far - near));
}

void main() {
  vec4 frag_colorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_NormalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 position = getWorldPosition(frag_colorAndDepth.w);
  vec3 cameraToSurface = position - cameraPosition;
  vec3 n_cameraToSurface = normalize(position - cameraPosition);
  vec3 normal = frag_NormalAndSpecularity.rgb;
  vec3 reflectionVector = reflect(n_cameraToSurface, normal);

  // Screen-space reflections
  // @todo make up better variable names, see about using
  // a hit test function
  int steps = 5;
  float stepSize = 100.0;
  vec3 ray = position;
  float fragDepth = frag_colorAndDepth.w;

  for (int i = 0; i < steps; i++) {
    ray += reflectionVector * stepSize;

    vec4 clip_ray = projection * view * vec4(ray * vec3(1, 1, -1), 1.0);
    vec3 ndc_ray = clip_ray.xyz / clip_ray.w;
    vec2 ray_uv = vec2(ndc_ray) * 0.5 + 0.5;

    if (ray_uv.x < 0.0 || ray_uv.x > 1.0 || ray_uv.y < 0.0 || ray_uv.y > 1.0) {
      break;
    }

    vec4 sample_colorAndDepth = texture(colorAndDepth, ray_uv);

    if (
      // If the sampled surface is further away than the reflecting surface...
      sample_colorAndDepth.w > frag_colorAndDepth.w &&
      // ...and the sampled surface depth is closer than the ray
      getLinearizedDepth(sample_colorAndDepth.w) < clip_ray.w
    ) {
      out_color = sample_colorAndDepth.rgb * 0.3;

      return;
    }
  }

  out_color = frag_colorAndDepth.rgb * getSkyColor(reflectionVector);
}