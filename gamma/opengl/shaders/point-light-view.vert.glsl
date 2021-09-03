#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

mat4 glMat4(mat4 matrix) {
  matrix[3][2] *= -1;

  return matrix;
}

void main() {
  gl_Position = glMat4(modelMatrix) * vec4(vertexPosition, 1.0);
}