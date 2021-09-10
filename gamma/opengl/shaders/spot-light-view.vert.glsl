#version 460 core

uniform mat4 lightMatrix;

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

// @todo when adding support for transparent textures
// out vec2 fragUv;

@include('utils/gl.glsl');

void main() {
  gl_Position = lightMatrix * glMat4(modelMatrix) * vec4(vertexPosition, 1.0);
}