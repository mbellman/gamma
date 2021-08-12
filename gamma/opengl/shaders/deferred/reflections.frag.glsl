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

struct IntersectionTest {
  bool hit;
  vec3 color;
  float intensity;
};

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
  float near = 1.0;
  float far = 10000.0;

  return 2.0 * near * far / (far + near - clip_depth * (far - near));
}

vec2 ViewToScreenCoords(vec4 viewSpace) {
  vec4 proj = projection * viewSpace;
  vec3 clip = proj.xyz / proj.w;

  return clip.xy * 0.5 + 0.5;
}

bool isOutOfBounds(vec2 uv) {
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

/**
 * Returns a reflected light intensity factor, reduced
 * by proximity of the sample to the screen edges.
 */
float GetSampleIntensity(vec2 uv) {
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
IntersectionTest GetRefinedIntersection(float stepSize, vec3 view_RayBounce, vec3 view_RayContinuation, vec3 view_RayDirection) {
  const int REFINEMENT_STEPS = 6;

  // @todo march in screen space instead of view space
  vec3 ray = view_RayContinuation;
  vec3 rayStep = view_RayDirection * stepSize;
  vec3 finalColor = vec3(0);
  vec2 uv = vec2(0);

  for (int i = 0; i < REFINEMENT_STEPS; i++) {
    rayStep *= 0.5;
    ray += rayStep;

    // @todo fix negative z hack
    uv = ViewToScreenCoords(vec4(ray, 1.0) * vec4(1, 1, -1, 1));

    vec4 sampled = texture(colorAndDepth, uv);
    float sampleDepth = getLinearizedDepth(sampled.w);

    finalColor = sampled.rgb;

    if (sampleDepth < ray.z && sampleDepth > view_RayBounce.z) {
      ray -= rayStep;
    }
  }

  return IntersectionTest(true, finalColor, GetSampleIntensity(uv));
}

/**
 * @todo description
 */
IntersectionTest TestRayIntersection(
  vec3 view_RayBounce,
  vec3 view_RayDirection,
  vec3 rayOffset,
  float stepSize
) {
  const int TEST_STEPS = 16;

  // @todo march in screen space instead of view space
  vec3 ray = view_RayBounce + rayOffset;
  vec3 rayStep = view_RayDirection * stepSize;
  vec3 previousRay = ray;

  for (int i = 0; i < TEST_STEPS; i++) {
    ray += rayStep;

    // @todo fix negative z hack (note: will go away with screen-space ray marching)
    vec2 uv = ViewToScreenCoords(vec4(ray, 1.0) * vec4(1, 1, -1, 1));

    if (isOutOfBounds(uv) || ray.z > 10000.0) {
      break;
    }

    vec4 sampled = texture(colorAndDepth, uv);
    float sampleDepth = getLinearizedDepth(sampled.w);

    if (sampleDepth < ray.z && (sampleDepth - previousRay.z) > -20.0) {
      return GetRefinedIntersection(stepSize, view_RayBounce, ray - rayStep, view_RayDirection);
    }

    previousRay = ray;
  }

  return IntersectionTest(false, vec3(0.0), 0.0);
}

void main() {
  vec4 frag_ColorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_NormalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 frag_Position = getWorldPosition(frag_ColorAndDepth.w);
  vec3 cameraToFragment = frag_Position - cameraPosition;
  vec3 normal = frag_NormalAndSpecularity.rgb;

  // Screen-space reflections
  float minStepSize = 5.0;
  float maxStepSize = 100.0;
  float jitter = 5.0;
  float baseColorFactor = 0.0;
  float reflectivity = 1.0;

  // @todo these -z hacks are ridiculous; determine
  // a better way to regularize the vectors to the
  // camera's left-handed coordinate system
  vec4 frag_ViewPosition = view * vec4(frag_Position * vec3(1, 1, -1), 1.0) * vec4(1, 1, -1, 1);
  vec4 viewNormal = transpose(inverseView) * vec4(normal * vec3(1, 1, -1), 1.0) * vec4(1, 1, -1, 1);
  vec3 worldReflectionVector = reflect(normalize(cameraToFragment), normal);
  vec3 viewReflectionVector = reflect(normalize(frag_ViewPosition.xyz), viewNormal.xyz);

  float glance = max(dot(normalize(cameraToFragment), worldReflectionVector), 0.0);
  float stepSize = mix(minStepSize, maxStepSize, glance);
  vec3 rayOffset = viewReflectionVector * jitter * noise();

  IntersectionTest result = TestRayIntersection(frag_ViewPosition.xyz, viewReflectionVector, rayOffset, stepSize);
  vec3 baseColor = frag_ColorAndDepth.rgb * baseColorFactor;
  vec3 reflectionColor = result.color * result.intensity;
  vec3 skyColor = getSkyColor(worldReflectionVector) * reflectivity * (1.0 - result.intensity);

  out_ColorAndDepth = vec4(baseColor + reflectionColor + skyColor, frag_ColorAndDepth.w);
}