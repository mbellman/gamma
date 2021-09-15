#version 460 core

uniform mat4 projection;
uniform mat4 view;
uniform vec3 spawn;
uniform float time;

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

out vec2 fragUv;
flat out vec3 color;

vec3 getBasePosition() {
  const int sq = 21 * 21;
  const float PI = 3.141592;
  const float TAU = 2.0 * PI;
  int i = gl_InstanceID;

  float r = float(i) / 10000.0;

  // @todo adjust base position; see if it's preferable
  // to precompute base positions on the CPU
  float slice = floor(float(i) / 100.0) / 100.0;
  float radius = 10.0 + sin(time + r * 500.0) * 5.0;
  float radius_factor = sqrt(sin(slice * PI));

  float x = radius * cos(float(i) / 100.0 * TAU) * radius_factor;
  float z = radius * sin(float(i) / 100.0 * TAU) * radius_factor;
  float y = (radius / 50.0) * floor(float(i) / 100.0) - radius;

  return vec3(x, y, z);
}

void main() {
  vec3 basePosition = spawn + getBasePosition();

  float r = float(gl_InstanceID) / 10000.0;
  float scale = 10.0 + 10.0 * sin(time * 3.0 + r * 500.0);

  mat4 model = mat4(
    scale, 0, 0, 0,
    0, scale, 0, 0,
    0, 0, scale, 0,
    basePosition.x, basePosition.y, -basePosition.z, 1
  );

  gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
  gl_PointSize = scale;

  fragUv = vertexUv;

  color = vec3(sin(r * 500.0) * 0.5 + 0.5, sin(time * 2.0) * 0.5 + 0.5, cos(r * 1000.0) * 0.5 + 0.5);
}