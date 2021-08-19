#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

uniform mat4 lightView;

// out vec2 fragUv;

// @todo move to gl helpers
mat4 glMat4(mat4 matrix) {
  matrix[3][2] *= -1;

  return matrix;
}

void main() {
  gl_Position = lightView * glMat4(modelMatrix) * vec4(vertexPosition, 1.0);

  // fragUv = vertexUv;
}