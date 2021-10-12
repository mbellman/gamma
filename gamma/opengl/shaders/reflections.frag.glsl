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
  float screen_edge_visibility;
};

// @todo pass these in as uniforms
const float z_near = 1.0;
const float z_far = 10000.0;
const float min_ray_step_size = 2.0;
const float max_ray_step_size = 10.0;
const float jitter = 1.0;
const float reflection_factor = 0.5;
const float thickness_threshold = 5.0;
const float slowdown_distance_threshold = 30.0;
const float distant_reflection_test_size = 4.0;
const float contact_ray_step_size = 1.0;

const int TOTAL_MARCH_STEPS = 24;
const int TOTAL_REFINEMENT_STEPS = 6;

#include "utils/gl.glsl";
#include "utils/conversion.glsl";
#include "utils/random.glsl";

// @todo allow shader imports; import this function
// from skybox helpers or similar. track shader dependencies
// as part of hot reloading
vec3 getSkyColor(vec3 direction) {
  vec3 sunDirection = normalize(vec3(0.3, 0.5, -1.0));
  vec3 sunColor = vec3(1.0, 0.3, 0.1);
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

bool isOffScreen(vec2 uv) {
  return uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
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
  if (uv.y > (1.0 - Y_TAPER)) intensity *= 1.0 - (uv.y - (1.0 - Y_TAPER)) * (1.0 / Y_TAPER);

  return clamp(intensity, 0, 1);
}

/**
 * Returns the distance between the geometry at a point
 * defined by the tip of a ray in view space, and the
 * tip of the ray itself.
 */
float getRayDistance(vec3 test_ray) {
  vec2 test_uv = getScreenCoordinates(test_ray, projection);
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
  float rayDistance = getRayDistance(view_ray + normalized_view_reflection_ray * 10.0);

  return rayDistance < 0 && rayDistance > -20.0;
}

/**
 * Determines whether a reflecting surface might have a
 * reflection from a more distant object, where a preliminary
 * march along the reflection ray sees increasing distance
 * from the underlying geometry. Surfaces with potentially
 * distant reflections begin with a larger ray march step
 * to mitigate oversampling where intersections are unlikely
 * to occur.
 */
bool mightHaveDistantReflection(vec3 view_ray, vec3 normalized_view_reflection_ray) {
  float r1 = getRayDistance(view_ray + normalized_view_reflection_ray * distant_reflection_test_size);
  float r2 = getRayDistance(view_ray + normalized_view_reflection_ray * distant_reflection_test_size * 2.0);
  float r3 = getRayDistance(view_ray + normalized_view_reflection_ray * distant_reflection_test_size * 3.0);
  float r4 = getRayDistance(view_ray + normalized_view_reflection_ray * distant_reflection_test_size * 4.0);

  return r4 > r3 && r3 > r2 && r2 > r1;
}

/**
 * Returns a Reflection by converging on the likely point
 * that a ray intersects geometry, determined in screen space.
 * We march the ray forward or backward, depending on whether
 * an intersection does not or does occur, respectively, using
 * decreasing steps each time in a binary search.
 */
Reflection getRefinedReflection(
  vec3 view_reflecting_surface_position,
  vec3 normalized_view_reflection_ray,
  vec3 view_starting_ray,
  float march_step_size
) {
  vec3 ray = view_starting_ray;
  vec3 ray_step = normalized_view_reflection_ray * march_step_size;
  vec3 refined_color = vec3(0);
  vec2 refined_uv = vec2(0);
  float test_depth;

  for (int i = 0; i < TOTAL_REFINEMENT_STEPS; i++) {
    // Decrease step size by half and advance the ray
    ray_step *= 0.5;
    ray += ray_step;
    refined_uv = getScreenCoordinates(ray, projection);

    vec4 test = texture(colorAndDepth, refined_uv);

    test_depth = getLinearizedDepth(test.w);
    refined_color = test.rgb;

    // If the ray is still intersecting the geometry,
    // advance a full step back. On the next cycle
    // we'll halve the step size and advance forward
    // again, converging on our likely reflection point.
    if (test_depth < ray.z && test_depth > view_reflecting_surface_position.z) {
      ray -= ray_step;
    }
  }

  // Disable reflections of points at the far plane
  float intensity = test_depth >= z_far ? 0.0 : getReflectionIntensity(refined_uv);

  return Reflection(refined_color, refined_uv, intensity);
}

/**
 * Returns a Reflection at a given reflecting surface fragment,
 * which may or may not reflect actual screen-space geometry.
 * Surfaces with reflection rays which 'miss' geometry instead
 * reflect the sky.
 */
Reflection getReflection(
  vec3 view_reflecting_surface_position,
  vec3 normalized_view_reflection_ray,
  vec3 march_offset,
  float march_step_size
) {
  vec3 ray = view_reflecting_surface_position + march_offset;
  vec3 previous_ray = ray;
  float adjusted_march_step_size = march_step_size;

  if (hasContactReflection(ray, normalized_view_reflection_ray)) {
    adjusted_march_step_size = contact_ray_step_size;
  } else if (mightHaveDistantReflection(ray, normalized_view_reflection_ray)) {
    adjusted_march_step_size *= 2.0;
  }

  adjusted_march_step_size *= random(0.975, 1.025);

  float current_step_size = adjusted_march_step_size;

  for (int i = 0; i < TOTAL_MARCH_STEPS; i++) {
    vec3 ray_step = normalized_view_reflection_ray * current_step_size;

    ray += ray_step;

    vec2 uv = getScreenCoordinates(ray, projection);

    if (isOffScreen(uv) || ray.z >= z_far) {
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
        ? (test_depth - previous_ray.z > -thickness_threshold)
        // 3) For incoming rays, the previous ray
        // was behind the geometry, within a thickness
        // threshold
        : (previous_ray.z - test_depth < thickness_threshold)
      )
    ) {
      return getRefinedReflection(view_reflecting_surface_position, normalized_view_reflection_ray, ray - ray_step, adjusted_march_step_size);
    } else if (ray_to_geometry_distance < slowdown_distance_threshold) {
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
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
  vec4 frag_normal_and_specularity = texture(normalAndSpecularity, fragUv);
  vec3 frag_world_position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 camera_to_fragment = frag_world_position - cameraPosition;
  vec3 normalized_camera_to_fragment = normalize(camera_to_fragment);
  vec3 frag_world_normal = frag_normal_and_specularity.rgb;

  vec4 frag_view_position = glVec4(view * glVec4(frag_world_position));
  vec4 frag_view_normal = glVec4(transpose(inverseView) * glVec4(frag_world_normal));
  vec3 world_reflection_vector = reflect(normalized_camera_to_fragment, frag_world_normal);
  vec3 normalized_view_reflection_ray = reflect(normalize(frag_view_position.xyz), frag_view_normal.xyz);

  float glance = max(dot(normalized_camera_to_fragment, world_reflection_vector), 0.0);
  float march_step_size = mix(min_ray_step_size, max_ray_step_size, glance);
  vec3 march_offset = normalized_view_reflection_ray * random(0.0, jitter);

  Reflection reflection = getReflection(frag_view_position.xyz, normalized_view_reflection_ray, march_offset, march_step_size);
  vec3 baseColor = frag_color_and_depth.rgb * (1.0 - reflection_factor);
  vec3 reflectionColor = reflection.color * reflection.screen_edge_visibility * reflection_factor;
  vec3 skyColor = getSkyColor(world_reflection_vector) * reflection_factor * (1.0 - reflection.screen_edge_visibility);

  out_ColorAndDepth = vec4(baseColor + reflectionColor + skyColor, frag_color_and_depth.w);
}