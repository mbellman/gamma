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
  bool hit;
  vec3 color;
  vec2 uv;
};

/**
 * Reconstructs a world position from pixel depth.
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
  float near = 1.0;
  float far = 10000.0;

  return 2.0 * near * far / (far + near - clip_depth * (far - near));
}

bool isRayOutOfBounds(vec3 ray) {
  return ray.x < 0.0 || ray.x > 1.0 || ray.y < 0.0 || ray.y > 1.0 || ray.z >= 1.0 || ray.z <= 0.0;
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
  float n = (noise() * 0.5 + 0.5);

  return low + n * (high - low);
}

float mixClamped(float low, float high, float alpha) {
  return clamp(mix(low, high, alpha), low, high);
}

/**
 * Returns a reflected light intensity factor, reduced
 * by proximity of the sample to the screen edges.
 */
float getReflectionIntensity(Reflection reflection) {
  if (!reflection.hit) {
    return 0.0;
  }

  const float X_TAPER = 0.05;
  const float Y_TAPER = 0.2;

  vec2 uv = reflection.uv - 0.5;
  float factor = sqrt(uv.x * uv.x + uv.y * uv.y);

  return (1.0 - factor);
}

/**
 * @todo description
 */
Reflection getRefinedReflection(float stepSize, vec3 rayContinuation, vec3 stepCoords) {
  const int REFINEMENT_STEPS = 6;

  vec3 ray = rayContinuation;
  vec3 rayStep = stepCoords * stepSize;
  vec3 finalColor = vec3(0);
  vec2 uv = vec2(0);

  for (int i = 0; i < REFINEMENT_STEPS; i++) {
    rayStep *= 0.5;
    ray += rayStep;

    vec4 sampled = texture(colorAndDepth, ray.xy);

    finalColor = sampled.rgb;

    if (sampled.w < ray.z) {
      ray -= rayStep;
    }
  }

  return Reflection(true, finalColor, ray.xy);
}

/**
 * @todo description
 */
Reflection getReflection(vec3 startCoords, vec3 stepCoords, float fragDistance) {
  const int TEST_STEPS = 16;

  float stepSize = 0.05;
  vec3 rayStep = stepCoords;
  vec3 ray = startCoords;
  vec3 previousRay = ray;
  float linearPreviousRayDepth = getLinearizedDepth(ray.z);

  // ray += rayStep * random(-stepSize * 0.1, stepSize * 0.1);

  for (int i = 0; i < TEST_STEPS; i++) {
    ray += rayStep * stepSize;

    if (isRayOutOfBounds(ray)) {
      break;
    }

    vec4 sampled = texture(colorAndDepth, ray.xy);
    float linearSampleDepth = getLinearizedDepth(sampled.w);
    float linearRayDepth = getLinearizedDepth(ray.z);

    if (
      // 1) The sample point is not at the far plane
      sampled.w < 1.0 &&
      // 2) The ray is behind/'intersecting' the sample
      ray.z > sampled.w &&
      (stepCoords.z > 0
        // 3) An outgoing ray intersected the object from the front
        ? (linearSampleDepth - linearPreviousRayDepth > -20)
        // 4) An incoming ray intersected the object from the back
        : (linearRayDepth - linearSampleDepth < 50)
      )
    ) {
      return getRefinedReflection(stepSize, ray - rayStep * stepSize, stepCoords);
    }

    previousRay = ray;
    linearPreviousRayDepth = linearRayDepth;
  }

  return Reflection(false, vec3(0), vec2(0));
}

void main() {
  vec4 frag_ColorAndDepth = texture(colorAndDepth, fragUv);
  vec4 frag_NormalAndSpecularity = texture(normalAndSpecularity, fragUv);
  vec3 frag_Position = getWorldPosition(frag_ColorAndDepth.w);
  float frag_Depth = getLinearizedDepth(frag_ColorAndDepth.w);
  vec3 cameraToFragment = frag_Position - cameraPosition;
  vec3 normal = frag_NormalAndSpecularity.rgb;

  // Screen-space reflections
  float baseColorFactor = 0.0;
  float reflectivity = 1.0;

  vec3 n_cameraToFragment = normalize(cameraToFragment);
  vec3 reflectionRay = reflect(n_cameraToFragment, normal);
  vec4 startCoords = vec4(fragUv.x, fragUv.y, frag_ColorAndDepth.w, 1.0);

  vec3 firstBounce = frag_Position + reflectionRay * 50.0;
  vec4 firstBounceCoords = projection * view * vec4(firstBounce * vec3(1, 1, -1), 1.0);
  firstBounceCoords /= firstBounceCoords.w;
  firstBounceCoords.xy *= 0.5;
  firstBounceCoords.xy += 0.5;
  // @todo why do we have to do this? is there something
  // wrong with the projection/perspective divide?
  firstBounceCoords.z = sqrt(firstBounceCoords.z);

  vec3 stepCoords = normalize(firstBounceCoords.xyz - startCoords.xyz);

  Reflection reflection = getReflection(startCoords.xyz, stepCoords, frag_Depth);
  float intensity = getReflectionIntensity(reflection);

  vec3 baseColor = frag_ColorAndDepth.rgb * baseColorFactor;
  vec3 reflectionColor = reflection.color * intensity;
  vec3 skyColor = getSkyColor(reflectionRay) * reflectivity * (1.0 - intensity);

  out_ColorAndDepth = vec4(baseColor + reflectionColor + skyColor, frag_ColorAndDepth.w);
}