#version 460 core

uniform sampler2D colorAndDepth;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_ColorAndDepth;

/**
 * Returns a value within the range -1.0 - 1.0, constant
 * in screen space, acting as a noise filter.
 *
 * @todo move to helpers/allow shader imports
 */
float noise(float seed) {
  return 2.0 * (fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * seed * 43758.545312) - 0.5);
}

float luminance(vec3 color) {
  return length(color * vec3(0.2126, 0.7152, 0.0722));
}

void main() {
  vec2 texel = 1.0 / vec2(1920.0, 1080.0);

  vec4 frag = texture(colorAndDepth, fragUv);
  vec4 top = texture(colorAndDepth, fragUv + texel * vec2(0.0, 1.0));
  vec4 bottom = texture(colorAndDepth, fragUv + texel * vec2(0.0, -1.0));
  vec4 left = texture(colorAndDepth, fragUv + texel * vec2(-1.0, 0.0));
  vec4 right = texture(colorAndDepth, fragUv + texel * vec2(1.0, 0.0));

  float frag_luminance = luminance(frag.rgb);
  float top_luminance = luminance(top.rgb);
  float bottom_luminance = luminance(bottom.rgb);
  float left_luminance = luminance(left.rgb);
  float right_luminance = luminance(right.rgb);

  if (
    length(frag_luminance - top_luminance) > 0.05 ||
    length(frag_luminance - left_luminance) > 0.05 ||
    length(frag_luminance - right_luminance) > 0.05 ||
    length(frag_luminance - bottom_luminance) > 0.05
  ) {
    vec2 o = vec2(noise(1.0), noise(2.0)) * 4.0;
    vec2 o2 = vec2(noise(3.0), noise(4.0)) * 4.0;
    vec2 o3 = vec2(noise(5.0), noise(6.0)) * 4.0;

    vec3 rc = texture(colorAndDepth, fragUv + texel * o).rgb;
    vec3 rc2 = texture(colorAndDepth, fragUv + texel * o2).rgb;
    vec3 rc3 = texture(colorAndDepth, fragUv + texel * o3).rgb;

    vec3 av = (rc + rc2 + rc3 + top.rgb + bottom.rgb + left.rgb + right.rgb) / 7.0;

    out_ColorAndDepth = vec4(av, 1.0);
  } else {
    out_ColorAndDepth = vec4(frag.rgb, frag.w);
  }
}