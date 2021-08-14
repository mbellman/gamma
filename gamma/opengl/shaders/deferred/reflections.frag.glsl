#version 460 core

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;
uniform vec3 cameraPosition;
uniform mat4 view;
uniform mat4 inverseView;
uniform mat4 projection;
uniform mat4 inverseProjection;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_ColorAndDepth;

struct Reflection {
  vec3 color;
  vec2 uv;
  float intensity;
};

// @todo pass these in as uniforms
const float Z_NEAR = 1.0;
const float Z_FAR = 10000.0;

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
  vec3 sunDirection = normalize(vec3(0.5, 1.0, -1.0));
  vec3 sunColor = vec3(1.0, 0.1, 0.2);
  float sunBrightness = 10;
  float altitude = 0.2;

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

// @todo move to a helpers file; allow shader imports
float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  // @todo pass in near/far terms as uniforms
  float near = Z_NEAR;
  float far = Z_FAR;

  return 2.0 * near * far / (far + near - clip_depth * (far - near));
}

vec2 viewToScreenCoordinates(vec3 view_position) {
  // @hack
  view_position.z *= -1;

  vec4 proj = projection * vec4(view_position, 1.0);
  vec3 clip = proj.xyz / proj.w;

  return clip.xy * 0.5 + 0.5;
}

bool isOffScreen(vec2 uv) {
  return uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
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

float random(float low, float high) {
  return low + (noise(1.0) * 0.5 + 0.5) * (high - low);
}

/**
 * Returns a reflected light intensity factor, reduced
 * by proximity of the sample to the screen edges.
 */
float getReflectionIntensity(vec2 uv) {
  const float X_TAPER = 0.05;
  const float Y_TAPER = 0.2;

  float intensity = 1.0;

  if (uv.x < X_TAPER) intensity *= (uv.x / X_TAPER);
  if (uv.x > (1.0 - X_TAPER)) intensity *= 1.0 - (uv.x - (1.0 - X_TAPER)) * (1.0 / X_TAPER);
  if (uv.y < Y_TAPER) intensity *= (uv.y / Y_TAPER);
  if (uv.y > (1.0 - Y_TAPER)) intensity *= 1.0 - (uv.y - (1.0 - Y_TAPER)) * (1.0 / Y_TAPER);

  return clamp(intensity, 0, 1);
}

/**
 * @todo description
 */
float getRayDistance(vec3 test_ray) {
  vec2 test_uv = viewToScreenCoordinates(test_ray);
  vec4 test_sample = texture(colorAndDepth, test_uv);
  float test_depth = getLinearizedDepth(test_sample.w);

  return test_depth - test_ray.z;
}

/**
 * Performs a 'contact test' to capture close-proximity
 * reflections which the predetermined ray march step
 * size might miss.
 */
bool hasContactReflection(vec3 view_ray, vec3 normalized_view_reflection_ray) {
  return getRayDistance(view_ray + normalized_view_reflection_ray * 10.0) < 0;
}

/**
 * Determines whether a reflecting surface might have a
 * reflection from a more distant object, where a preliminary
 * march along the reflection ray sees increasing distance
 * from the underlying geometry.
 */
bool mightHaveDistantReflection(vec3 view_ray, vec3 normalized_view_reflection_ray) {
  float r1 = getRayDistance(view_ray + normalized_view_reflection_ray * 20.0);
  float r2 = getRayDistance(view_ray + normalized_view_reflection_ray * 40.0);
  float r3 = getRayDistance(view_ray + normalized_view_reflection_ray * 60.0);
  float r4 = getRayDistance(view_ray + normalized_view_reflection_ray * 80.0);

  return r4 > r3 && r3 > r2 && r2 > r1;
}

/**
 * @todo description
 */
Reflection getRefinedReflection(
  vec3 view_reflecting_surface_position,
  vec3 normalized_view_reflection_ray,
  vec3 view_starting_ray,
  float march_step_size
) {
  const int REFINEMENT_STEPS = 6;

  vec3 ray = view_starting_ray;
  vec3 ray_step = normalized_view_reflection_ray * march_step_size;
  vec3 refined_color = vec3(0);
  vec2 refined_uv = vec2(0);

  for (int i = 0; i < REFINEMENT_STEPS; i++) {
    ray_step *= 0.5;
    ray += ray_step;

    refined_uv = viewToScreenCoordinates(ray);

    vec4 test = texture(colorAndDepth, refined_uv);
    float test_depth = getLinearizedDepth(test.w);

    refined_color = test.rgb;

    if (test_depth < ray.z && test_depth > view_reflecting_surface_position.z) {
      ray -= ray_step;
    }
  }

  return Reflection(refined_color, refined_uv, getReflectionIntensity(refined_uv));
}

/**
 * @todo description
 */
Reflection getReflection(
  vec3 view_reflecting_surface_position,
  vec3 normalized_view_reflection_ray,
  vec3 march_offset,
  float march_step_size
) {
  const int TOTAL_MARCH_STEPS = 16;

  vec3 ray = view_reflecting_surface_position + march_offset;
  vec3 previous_ray = ray;
  float adjusted_march_step_size = march_step_size;

  if (hasContactReflection(ray, normalized_view_reflection_ray)) {
    adjusted_march_step_size = 5.0;
  } else if (mightHaveDistantReflection(ray, normalized_view_reflection_ray)) {
    adjusted_march_step_size *= 2.0;
  }

  adjusted_march_step_size *= random(0.9, 1.1);

  float current_step_size = adjusted_march_step_size;

  for (int i = 0; i < TOTAL_MARCH_STEPS; i++) {
    vec3 ray_step = normalized_view_reflection_ray * current_step_size;

    ray += ray_step;

    vec2 uv = viewToScreenCoordinates(ray);

    if (isOffScreen(uv) || ray.z >= Z_FAR) {
      break;
    }

    vec4 test = texture(colorAndDepth, uv);
    float test_depth = getLinearizedDepth(test.w);
    float ray_to_geometry_distance = abs(ray.z - test_depth);

    // A reflection occurs when:
    if (
      // 1) The ray is behind the geometry
      ray.z > test_depth &&
      (normalized_view_reflection_ray.z > 0
        // 2) For outgoing rays, the previous ray
        // was 'in front' of the geometry, within
        // a thickness threshold
        ? (test_depth - previous_ray.z > -40.0)
        // 3) For incoming rays, the previous ray
        // was behind the geometry, within a thickness
        // threshold
        : (previous_ray.z - test_depth < 40)
      )
    ) {
      return getRefinedReflection(view_reflecting_surface_position, normalized_view_reflection_ray, ray - ray_step, adjusted_march_step_size);
    } else if (ray_to_geometry_distance < 150) {
      // If a reflection did not occur, but the ray is
      // within close proximity to geometry, reduce the
      // step size to mitigate undersampling
      current_step_size = adjusted_march_step_size * 0.75;
    } else {
      // If a reflection did not occur and the geometry behind
      // the ray is more distant, increase the step size to
      // mitigate oversampling
      current_step_size = adjusted_march_step_size * 1.5;
    }

    previous_ray = ray;
  }

  return Reflection(vec3(0), vec2(0), 0);
}

void main() {
  // @todo can we write to alternate rows each frame,
  // since the reflection buffer is preserved across frames?
  // if (int(fragUv.x * 1920.0) % 2 == 0) {
  //   discard;
  // }

  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
  vec4 frag_normal_and_specularity = texture(normalAndSpecularity, fragUv);
  vec3 frag_world_position = getWorldPosition(frag_color_and_depth.w);
  vec3 camera_to_fragment = frag_world_position - cameraPosition;
  vec3 normalized_camera_to_fragment = normalize(camera_to_fragment);
  vec3 frag_world_normal = frag_normal_and_specularity.rgb;

  // Screen-space reflections
  float min_step_size = 5.0;
  float max_step_size = 75.0;
  float jitter = 5.0;
  float base_color_factor = 0.0;
  float reflection_color_factor = 1.0;

  // @todo these -z hacks are ridiculous; determine
  // a better way to regularize the vectors to the
  // camera's left-handed coordinate system
  vec4 frag_view_position = view * vec4(frag_world_position * vec3(1, 1, -1), 1.0) * vec4(1, 1, -1, 1);
  vec4 frag_view_normal = transpose(inverseView) * vec4(frag_world_normal * vec3(1, 1, -1), 1.0) * vec4(1, 1, -1, 1);
  vec3 world_reflection_vector = reflect(normalized_camera_to_fragment, frag_world_normal);
  vec3 normalized_view_reflection_ray = reflect(normalize(frag_view_position.xyz), frag_view_normal.xyz);

  float glance = max(dot(normalized_camera_to_fragment, world_reflection_vector), 0.0);
  float march_step_size = mix(min_step_size, max_step_size, glance);
  vec3 march_offset = normalized_view_reflection_ray * random(0.0, jitter);

  Reflection reflection = getReflection(frag_view_position.xyz, normalized_view_reflection_ray, march_offset, march_step_size);
  vec3 baseColor = frag_color_and_depth.rgb * base_color_factor;
  vec3 reflectionColor = reflection.color * reflection.intensity * reflection_color_factor;
  vec3 skyColor = getSkyColor(world_reflection_vector) * reflection_color_factor * (1.0 - reflection.intensity);

  out_ColorAndDepth = vec4(baseColor + reflectionColor + skyColor, frag_color_and_depth.w);
}