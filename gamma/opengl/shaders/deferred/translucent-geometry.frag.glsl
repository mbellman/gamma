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

  return world.xyz * vec3(1.0, 1.0, -1.0);
}

vec2 viewToScreenCoordinates(vec3 view_position) {
  // @hack
  view_position.z *= -1;

  vec4 proj = projection * vec4(view_position, 1.0);
  vec3 clip = proj.xyz / proj.w;

  return clip.xy * 0.5 + 0.5;
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

bool isOffScreen(vec2 uv) {
  return uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1;
}

void main() {
  const float REFRACTION_INTENSITY = 30.0;

  vec3 position = getWorldPosition(gl_FragCoord.z);
  vec3 color = vec3(1.0);
  vec3 normal = normalize(fragNormal);
  vec3 world_refraction_ray = position + normal * REFRACTION_INTENSITY;

  // @todo Fix these z hacks! We can't keep doing this!
  vec4 view_normal = transpose(inverseView) * vec4(normal * vec3(1, 1, -1), 1.0) * vec4(1, 1, -1, 1);
  vec3 view_refraction_ray = (view * vec4(world_refraction_ray * vec3(1, 1, -1), 1.0)).xyz * vec3(1, 1, -1);
  vec2 refracted_color_coords = viewToScreenCoordinates(view_refraction_ray);
  float frag_depth = texture(colorAndDepth, getPixelCoords()).w;

  if (frag_depth < 1.0 && isOffScreen(refracted_color_coords)) {
    // If the fragment has a depth closer than the far plane,
    // discard any attempts at offscreen color reading
    discard;
  }

  vec4 refracted_color_and_depth = texture(colorAndDepth, refracted_color_coords);

  if (refracted_color_and_depth.w == 1.0) {
    // @todo Write the skybox to the post buffer before copying back
    // into G-Buffer attachment 2 (used for reflections + refractions).
    // This way we won't need to recalculate it; we need to read the
    // texture anyway and in the renderer it should just include disabling
    // stencil testing after the skybox draw
    vec3 direction = normalize(world_refraction_ray - cameraPosition);

    refracted_color_and_depth.rgb = getSkyColor(direction);
  }

  out_color_and_depth = refracted_color_and_depth;
}