#version 460 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in mat4 modelMatrix;

uniform mat4 projection;
uniform mat4 view;

out vec3 fragNormal;
out vec3 fragTangent;
out vec3 fragBitangent;
out vec2 fragUv;

vec3 invertZ(vec3 vector) {
  return vector * vec3(1.0, 1.0, -1.0);
}

/**
 * Returns a bitangent from potentially non-orthonormal
 * normal/tangent vectors using the Gram-Schmidt process.
 */
vec3 getFragBitangent(vec3 normal, vec3 tangent) {
  vec3 n_normal = normalize(normal);
  vec3 n_tangent = normalize(tangent);

  // Redefine the tangent by using the projection of the tangent
  // onto the normal line and defining a vector from that to the
  // original tangent, orthonormalizing the normal/tangent
  n_tangent = normalize(n_tangent - dot(n_tangent, n_normal) * n_normal);

  return cross(n_tangent, n_normal);
}

void main() {
  mat4 model = mat4(modelMatrix);
  mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

  // Invert the model translation z component to accommodate
  // the game world's left-handed coordinate system
  model[3][2] *= -1.0;

  gl_Position = projection * view * model * vec4(vertexPosition, 1.0);

  // Make similar adjustments to the normal/tangent z components
  fragNormal = invertZ(normalMatrix * vertexNormal);
  fragTangent = invertZ(normalMatrix * vertexTangent);
  fragBitangent = getFragBitangent(fragNormal, fragTangent);
  fragUv = vertexUv;
}