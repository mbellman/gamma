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

@include('utils/gl.glsl');
@include('utils/conversion.glsl');

vec2 getPixelCoords() {
  return gl_FragCoord.xy / vec2(1920.0, 1080.0);
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

  vec3 position = getWorldPosition(gl_FragCoord.z, getPixelCoords(), inverseProjection, inverseView);
  vec3 color = vec3(1.0);
  vec3 normal = normalize(fragNormal);

  // @todo The refraction ray should be either Snell's Law-based
  // or some approximation using angle of incidence vs. normal/
  // index of refraction - mix(normalized_fragment_to_camera, normal, ratio)
  vec3 world_refraction_ray = position - normal * REFRACTION_INTENSITY;

  vec3 view_refraction_ray = glVec4(view * glVec4(world_refraction_ray)).xyz;
  vec2 refracted_color_coords = getScreenCoordinates(view_refraction_ray, projection);
  float sample_depth = texture(colorAndDepth, getPixelCoords()).w;

  if (sample_depth < 1.0 && isOffScreen(refracted_color_coords)) {
    // If the fragment has a depth closer than the far plane,
    // discard any attempts at offscreen color reading
    discard;
  }

  if (gl_FragCoord.z > sample_depth) {
    // Accommodation for alpha-blended particles, which write to the
    // depth channel of the color and depth texture, but not to the
    // depth buffer, in order to properly blend against themselves.
    // Perform a 'manual' depth test to ensure that particles in front
    // of the refractive geometry aren't overwritten and incorrectly
    // rendered 'behind' it.
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