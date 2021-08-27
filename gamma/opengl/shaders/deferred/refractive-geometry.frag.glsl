#version 460 core

uniform sampler2D colorAndDepth;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform vec3 cameraPosition;

in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBitangent;
in vec2 fragUv;

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

vec2 getPixelCoords() {
  return gl_FragCoord.xy / vec2(1920.0, 1080.0);
}

/**
 * Reconstructs the world position from pixel depth.
 */
vec3 getWorldPosition(float depth) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(getPixelCoords() * 2.0 - 1.0, z, 1.0);
  vec4 view = inverseProjection * clip;

  view /= view.w;

  vec4 world = inverseView * view;

  return glVec3(world.xyz);
}

vec2 viewToScreenCoordinates(vec3 view_position) {
  vec4 proj = projection * glVec4(view_position);
  vec3 clip = proj.xyz / proj.w;

  return clip.xy * 0.5 + 0.5;
}

// @todo allow shader imports; import this function
// from skybox helpers or similar. track shader dependencies
// as part of hot reloading
vec3 getSkyColor(vec3 direction) {
  vec3 sunDirection = normalize(vec3(0.3, 0.5, -1.0));
  vec3 sunColor = vec3(1.0, 0.3, 0.1);
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

bool isOffScreen(vec2 uv) {
  return uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
}

void main() {
  const float REFRACTION_INTENSITY = 3.0;
  const vec3 GEOMETRY_COLOR = vec3(1, 0, 1);

  vec3 position = getWorldPosition(gl_FragCoord.z);
  vec3 color = vec3(1.0);
  vec3 normal = normalize(fragNormal);

  // @todo The refraction ray should be either Snell's Law-based
  // or some approximation using angle of incidence vs. normal/
  // index of refraction - mix(normalized_fragment_to_camera, normal, ratio)
  vec3 world_refraction_ray = position - normal * REFRACTION_INTENSITY;

  vec3 view_refraction_ray = glVec4(view * glVec4(world_refraction_ray)).xyz;
  vec2 refracted_color_coords = viewToScreenCoordinates(view_refraction_ray);
  float frag_depth = texture(colorAndDepth, getPixelCoords()).w;

  if (frag_depth < 1.0 && isOffScreen(refracted_color_coords)) {
    // If the fragment has a depth closer than the far plane,
    // discard any attempts at offscreen color reading
    discard;
  }

  vec4 refracted_color_and_depth = texture(colorAndDepth, refracted_color_coords);

  if (refracted_color_and_depth.w == 1.0) {
    // Skybox
    vec3 direction = normalize(world_refraction_ray - cameraPosition);

    refracted_color_and_depth.rgb = getSkyColor(direction);
  }

  refracted_color_and_depth.rgb *= GEOMETRY_COLOR;

  out_color_and_depth = vec4(refracted_color_and_depth.rgb, gl_FragCoord.z);
}