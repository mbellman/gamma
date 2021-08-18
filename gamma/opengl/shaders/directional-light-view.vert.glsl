#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

uniform mat4 lightView;

// out vec2 fragUv;

void main() {
  mat4 model = mat4(modelMatrix);

  // Invert the model translation z component to accommodate
  // the game world's left-handed coordinate system
  //
  // @todo this is unacceptable. we'll have to fix the engine's
  // negative-z issues before proceeding.
  model[3][2] *= -1.0;

  gl_Position = lightView * model * vec4(vertexPosition, 1.0);

  // fragUv = vertexUv;
}