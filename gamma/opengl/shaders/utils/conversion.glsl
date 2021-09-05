@include('utils/gl.glsl');

/**
 * Reconstructs a fragment's world position from depth,
 * using the inverse projection/view matrices to transform
 * the fragment coordinates back into world space.
 *
 * @todo can we replace frag_uv with gl_FragCoord.xy?
 */
vec3 getWorldPosition(float depth, vec2 frag_uv, mat4 inverse_projection, mat4 inverse_view) {
  float z = depth * 2.0 - 1.0;
  vec4 clip = vec4(frag_uv * 2.0 - 1.0, z, 1.0);
  vec4 view = inverse_projection * clip;

  view /= view.w;

  vec4 world = inverse_view * view;

  return glVec3(world.xyz);
}

/**
 * Maps a nonlinear [0, 1] depth value to a linearized
 * depth between the near and far planes.
 */
float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  // @todo import from a 'utils/constants.glsl' file; use uniforms
  float near_plane = 1.0;
  float far_plane = 10000.0;

  return 2.0 * near_plane * far_plane / (far_plane + near_plane - clip_depth * (far_plane - near_plane));
}