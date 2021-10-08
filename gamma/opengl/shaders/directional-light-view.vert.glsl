#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in uint modelColor;
layout (location = 5) in mat4 modelMatrix;

uniform mat4 lightView;

// @todo once mesh textures are checked for alpha
// out vec2 fragUv;

#include "utils/gl.glsl";

void main() {
  gl_Position = lightView * glMat4(modelMatrix) * vec4(vertexPosition, 1.0);

  // @todo once mesh textures are checked for alpha
  // fragUv = vertexUv;
}