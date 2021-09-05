#version 460 core

uniform sampler2D color;

in vec2 fragUv;

layout (location = 0) out vec3 out_color;

@include('utils/random.glsl');

/**
 * Applies a noise filter to colors to reduce banding
 * artifacts, strengthening the effect in darker ranges
 * where banding is more apparent.
 */
vec3 deband(vec3 color) {
  float brightness = color.r + color.g + color.b;
  float divisor = brightness * 150.0;

  return color * (1.0 + random(0.0, 1.0) / divisor);
}

void main() {
  out_color = deband(texture(color, fragUv).rgb);
}