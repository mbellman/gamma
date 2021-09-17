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

// @todo improve the degree of randomness
float random(vec2 seed){
  return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

float random(float seed) {
  return random(vec2(seed));
}

float random(float low, float high, float seed) {
  float a = random(vec2(seed));

  return mix(low, high, a);
}

vec3 getParticlePosition() {
  float random_seed = float(gl_InstanceID);
  float radius = 20.0 * sqrt(random(random_seed * 1.255673));

  float x = random(-1, 1, random_seed * 1.1);
  float y = random(-1, 1, random_seed * 1.2);
  float z = random(-1, 1, random_seed * 1.3);

  return radius * normalize(vec3(x, y, z));
}

void main() {
  vec3 position = spawn + getParticlePosition();

  float r = float(gl_InstanceID) / 10000.0;
  float scale = 10.0 + 10.0 * sin(time * 3.0 + r * 500.0);

  gl_Position = projection * view * vec4(position, 1.0);
  gl_PointSize = 5.0;

  fragUv = vertexUv;
  color = vec3(sin(r * 500.0) * 0.5 + 0.5, sin(time * 2.0) * 0.5 + 0.5, cos(r * 1000.0) * 0.5 + 0.5);
}