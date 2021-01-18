#version 460 core

struct Light {
  vec3 position;
  vec3 color;
  float radius;
  // @TODO int type;
};

layout (location = 0) vec2 vertexPosition;
layout (location = 1) vec2 discOffset;
layout (location = 2) vec2 discScale;
layout (location = 3) Light discLight;

flat out Light light;

void main() {
  gl_Position = vec4(vertexPosition * discScale + discOffset, 0.0, 1.0);

  light = discLight;
}