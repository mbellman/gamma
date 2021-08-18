#version 460 core

// in vec2 fragUv;

layout (location = 0) out float depth;

void main() {
  depth = gl_FragCoord.z;
}