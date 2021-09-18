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
const int total_path_points = 8;
int total_particles = 10000;
float particle_spread = 5.0;
float median_particle_speed = 0.2;
float particle_speed_variation = 0.1;

vec3 particle_path[total_path_points] = {
  vec3(0.0, 20.0, 0.0),
  vec3(20.0, -10.0, -40.0),
  vec3(50.0, 40.0, 10.0),
  vec3(0, 30, 10),
  vec3(-20, 40, 35),
  vec3(-40, 15, 25),
  vec3(-60, 20, -30),
  vec3(-5, 20, -5)
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

/**
 * Adapted from http://paulbourke.net/miscellaneous/interpolation/
 *
 * Performs cubic spline interpolation.
 */
float interpolateCubic(float a, float b, float c, float d, float alpha) {
  float m = alpha * alpha;

  float a0 = d - c - a + b;
  float a1 = a - b - a0;
  float a2 = c - a;
  float a3 = b;
  
  return (a0 * alpha * m) + (a1 * m) + (a2 * alpha) + a3;
}

/**
 * Performs cubic spline interpolation between two points
 * p2 and p3, using the neighboring points p1 and p4.
 */
vec3 interpolatePoints(vec3 p1, vec3 p2, vec3 p3, vec3 p4, float alpha) {
  return vec3(
    interpolateCubic(p1.x, p2.x, p3.x, p4.x, alpha),
    interpolateCubic(p1.y, p2.y, p3.y, p4.y, alpha),
    interpolateCubic(p1.z, p2.z, p3.z, p4.z, alpha)
  );
}

/**
 * Returns an adjusted path control point index, wrapping
 * around at the beginning and ending indices to avoid
 * out-of-bounds access.
 */
int getWrappedPathIndex(int index) {
  if (index < 0) {
    return total_path_points + index;
  }

  if (index >= total_path_points) {
    return index - total_path_points;
  }

  return index;
}

/**
 * Returns the initial particle position, pseudo-randomly
 * determined by its ID.
 */
vec3 getInitialPosition() {
  float radius = particle_spread * sqrt(random(particle_id * 1.255673));

  float x = random(-1, 1, particle_id * 1.1);
  float y = random(-1, 1, particle_id * 1.2);
  float z = random(-1, 1, particle_id * 1.3);

  return radius * normalize(vec3(x, y, z));
}

/**
 * Returns the particle's current position as a function of time.
 */
vec3 getParticlePosition() {
  float particle_speed = median_particle_speed + random(-particle_speed_variation, particle_speed_variation, particle_id);
  float path_progress = fract(random(0, 1, particle_id * 1.1) + time * (particle_speed / total_path_points));
  int path_index = int(floor(path_progress * total_path_points));

  vec3 p1 = particle_path[getWrappedPathIndex(path_index - 1)];
  vec3 p2 = particle_path[path_index];
  vec3 p3 = particle_path[getWrappedPathIndex(path_index + 1)];
  vec3 p4 = particle_path[getWrappedPathIndex(path_index + 2)];

  float interpolation_factor = fract(path_progress * total_path_points);
  vec3 stream_position = interpolatePoints(p1, p2, p3, p4, interpolation_factor);

  // @todo make oscillation configurable
  float oscillation = sin(particle_id + time) * 2.0;
  return spawn + getInitialPosition() + stream_position + oscillation;
}

void main() {
  vec3 position = getParticlePosition();

  float r = particle_id / float(total_particles);
  float scale = 10.0 + 10.0 * sin(time * 3.0 + r * 500.0);

  gl_Position = projection * view * glVec4(position);
  gl_PointSize = scale;

  fragUv = vertexUv;
  // @todo make color configurable
  color = vec3(sin(r * 500.0) * 0.5 + 0.5, sin(time * 2.0) * 0.5 + 0.5, cos(r * 1000.0) * 0.5 + 0.5);
}