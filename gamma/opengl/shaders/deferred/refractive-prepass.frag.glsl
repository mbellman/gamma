#version 460 core

uniform mat4 projection;
uniform sampler2D color_and_depth;

layout (location = 2) out vec4 out_color_and_depth;

// @todo move to a helpers file; allow shader imports
float getLinearizedDepth(float depth) {
  float clip_depth = 2.0 * depth - 1.0;
  float z_near = 1.0;
  float z_far = 10000.0;

  return 2.0 * z_near * z_far / (z_far + z_near - clip_depth * (z_far - z_near));
}

float getFragDepth(mat4 projection, float linearized_depth) {
  float a = projection[2][2];
  float b = projection[3][2];

  return 0.5 * (-a * linearized_depth + b) / linearized_depth + 0.5;
}

vec2 getPixelCoords() {
  return gl_FragCoord.xy / vec2(1920.0, 1080.0);
}

void main() {
  float linearized_depth = getLinearizedDepth(gl_FragCoord.z);
  vec4 base_color = texture(color_and_depth, getPixelCoords());

  out_color_and_depth = vec4(vec3(1, 0, 1) * base_color.rgb, gl_FragCoord.z);
  gl_FragDepth = getFragDepth(projection, linearized_depth + 1.0);
}