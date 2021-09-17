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

@include('utils/gl.glsl');

float particle_id = float(gl_InstanceID);

// @todo receive params from particle system config
int total_particles = 10000;
float median_particle_speed = 0.2;
float particle_speed_variation = 0.1;

vec3 particle_path[] = {
  vec3(10.0, 30.0, 50.0),
  vec3(2.0, 5.0, -30.0)
};

// @todo improve the degree of randomness here + move to utils
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

vec3 getInitialPosition() {
  float radius = 20.0 * sqrt(random(particle_id * 1.255673));

  float x = random(-1, 1, particle_id * 1.1);
  float y = random(-1, 1, particle_id * 1.2);
  float z = random(-1, 1, particle_id * 1.3);

  return radius * normalize(vec3(x, y, z));
}

vec3 getParticlePosition() {
  // @todo implement proper spline interpolation
  float particle_speed = median_particle_speed + random(-particle_speed_variation, particle_speed_variation, particle_id);
  float interpolation_factor = sin(particle_id + time * particle_speed) * 0.5 + 0.5;
  vec3 start = particle_path[0];
  vec3 end = particle_path[1];
  vec3 interpolated_position = mix(start, end, interpolation_factor);

  return getInitialPosition() + interpolated_position;
}

void main() {
  vec3 position = spawn + getParticlePosition();

  float r = particle_id / float(total_particles);
  float scale = 10.0 + 10.0 * sin(time * 3.0 + r * 500.0);

  gl_Position = projection * view * glVec4(position);
  gl_PointSize = scale;

  fragUv = vertexUv;
  // @todo make color configurable
  color = vec3(sin(r * 500.0) * 0.5 + 0.5, sin(time * 2.0) * 0.5 + 0.5, cos(r * 1000.0) * 0.5 + 0.5);
}