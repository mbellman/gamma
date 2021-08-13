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
  float intensity;
};

// struct ReflectionTest {
//   vec4 sample;
//   vec2 uv;
// };

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

// @todo move to a helpers file; allow shader imports
float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  // @todo pass in near/far terms as uniforms
  float near = Z_NEAR;
  float far = Z_FAR;

  return 2.0 * near * far / (far + near - clip_depth * (far - near));
}

vec2 viewToScreenCoords(vec4 viewCoords) {
  vec4 proj = projection * viewCoords;
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
float noise() {
  return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.545312);
}

float random(float low, float high) {
  return low + (noise() * 0.5 + 0.5) * (high - low);
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

  return intensity;
}

/**
 * @todo description
 */
Reflection getRefinedReflection(
  vec3 view_reflecting_surface_position,
  vec3 normalized_view_reflection_ray,
  vec3 view_starting_position,
  float march_step_size
) {
  const int REFINEMENT_STEPS = 6;

  vec3 ray = view_starting_position;
  vec3 ray_step = normalized_view_reflection_ray * march_step_size;
  vec3 final_color = vec3(0);
  vec2 uv = vec2(0);

  for (int i = 0; i < REFINEMENT_STEPS; i++) {
    ray_step *= 0.5;
    ray += ray_step;

    // @todo fix negative z hack
    uv = viewToScreenCoords(vec4(ray, 1.0) * vec4(1, 1, -1, 1));

    vec4 test = texture(colorAndDepth, uv);
    float test_depth = getLinearizedDepth(test.w);

    final_color = test.rgb;

    if (test_depth < ray.z && test_depth > view_reflecting_surface_position.z) {
      ray -= ray_step;
    }
  }

  return Reflection(final_color, getReflectionIntensity(uv));
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
  const float STRIDE_TEST_STEP_SIZES[4] = { 5.0, 20.0, 50.0, 75.0 };
  const int REFLECTION_TEST_STEPS = 16;

  vec3 ray = view_reflecting_surface_position + march_offset;
  vec3 previous_ray = ray;
  int total_stride_test_hits = 0;

  // First, cast a couple close-proximity rays to determine
  // what the ideal ray stride should be
  for (int i = 0; i < 4; i++) {
    vec3 test_ray = ray + normalized_view_reflection_ray * STRIDE_TEST_STEP_SIZES[i];

    // @todo fix negative z hack
    vec2 uv = viewToScreenCoords(vec4(test_ray * vec3(1, 1, -1), 1.0));

    if (isOffScreen(uv)) {
      break;
    }

    vec4 test = texture(colorAndDepth, uv);
    float test_depth = getLinearizedDepth(test.w);

    if (test_ray.z > test_depth) {
      march_step_size *= 0.6;
    } else {
      march_step_size *= 1.2;
    }
  }

  if (total_stride_test_hits == 0) {
    march_step_size *= 1.2;
  }

  vec3 ray_step = normalized_view_reflection_ray * march_step_size;

  for (int i = 0; i < REFLECTION_TEST_STEPS; i++) {
    ray += ray_step;

    // @todo fix negative z hack
    vec2 uv = viewToScreenCoords(vec4(ray, 1.0) * vec4(1, 1, -1, 1));

    if (isOffScreen(uv) || ray.z > Z_FAR) {
      break;
    }

    vec4 test = texture(colorAndDepth, uv);
    float test_depth = getLinearizedDepth(test.w);

    if (test_depth < ray.z && (test_depth - previous_ray.z) > -20.0) {
      return getRefinedReflection(view_reflecting_surface_position, normalized_view_reflection_ray, ray - ray_step, march_step_size);
    }

    previous_ray = ray;
  }

  return Reflection(vec3(0.0), 0.0);
}

void main() {
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
  vec3 normalized_normalized_view_reflection_ray = reflect(normalize(frag_view_position.xyz), frag_view_normal.xyz);

  float glance = max(dot(normalized_camera_to_fragment, world_reflection_vector), 0.0);
  float march_step_size = mix(min_step_size, max_step_size, glance);
  vec3 march_offset = normalized_normalized_view_reflection_ray * jitter * random(-2.0, 2.0);

  Reflection reflection = getReflection(frag_view_position.xyz, normalized_normalized_view_reflection_ray, march_offset, march_step_size);
  vec3 baseColor = frag_color_and_depth.rgb * base_color_factor;
  vec3 reflectionColor = reflection.color * reflection.intensity;
  vec3 skyColor = getSkyColor(world_reflection_vector) * reflection_color_factor * (1.0 - reflection.intensity);

  out_ColorAndDepth = vec4(baseColor + reflectionColor + skyColor, frag_color_and_depth.w);
}