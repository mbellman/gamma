#version 460 core

uniform sampler2D color;

in vec2 fragUv;

layout (location = 0) out vec3 out_color;

/**
 * Returns a value within the range -1.0 - 1.0, constant
 * in screen space, acting as a noise filter.
 */
float noise() {
  return fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.545312);
}

/**
 * Applies a noise filter to colors to reduce banding
 * artifacts, strengthening the effect in darker ranges
 * where banding is more apparent.
 */
vec3 deband(vec3 color) {
  float brightness = color.r + color.g + color.b;
  float divisor = brightness * 150.0;

  return color * (1.0 + noise() / divisor);
}

void main() {
  out_color = deband(texture(color, fragUv).rgb);
}