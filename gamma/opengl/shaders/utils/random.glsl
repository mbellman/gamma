/**
 * Returns a value within the range -1.0 - 1.0, constant
 * in screen space, acting as a noise filter.
 */
float noise(float seed) {
  return 2.0 * (fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * seed * 43758.545312) - 0.5);
}