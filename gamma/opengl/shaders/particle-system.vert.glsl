#version 460 core

uniform mat4 projection;
uniform mat4 view;
uniform vec3 spawn;

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

out vec2 fragUv;

vec3 getBasePosition() {
  const int sq = 21 * 21;
  const float PI = 3.141592;
  const float TAU = 2.0 * PI;
  int i = gl_InstanceID;

  // @todo adjust base position; see if it's preferable
  // to precompute base positions on the CPU
  float slice = floor(float(i) / 100.0) / 100.0;
  float radius = 10.0;
  float radius_factor = sqrt(sin(slice * PI));

  float x = radius * cos(float(i) / 100.0 * TAU) * radius_factor;
  float z = radius * sin(float(i) / 100.0 * TAU) * radius_factor;
  float y = (radius / 50.0) * floor(float(i) / 100.0) - radius;

  return vec3(x, y, z);
}

void main() {
  vec3 basePosition = spawn + getBasePosition();

  mat4 model = mat4(
    0.05, 0, 0, 0,
    0, 0.05, 0, 0,
    0, 0, 0.05, 0,
    basePosition.x, basePosition.y, -basePosition.z, 1
  );

  gl_Position = projection * view * model * vec4(vertexPosition, 1.0);

  fragUv = vertexUv;
}