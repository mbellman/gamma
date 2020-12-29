#version 460 core

uniform bool hasTexture = false;
uniform bool hasNormalMap = false;
uniform sampler2D modelTexture;
uniform sampler2D normalMap;

in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBitangent;
in vec2 fragUv;

layout (location = 0) out vec4 color_depth;
layout (location = 1) out vec4 normal_specularity;

vec3 getNormal() {
  vec3 n_fragNormal = normalize(fragNormal);

  if (hasNormalMap) {
    vec3 mappedNormal = texture(normalMap, fragUv).rgb * 2.0 - vec3(1.0);

    mat3 tangentMatrix = mat3(
      normalize(fragTangent),
      normalize(fragBitangent),
      n_fragNormal
    );

    return normalize(tangentMatrix * mappedNormal);
  } else {
    return n_fragNormal;
  }
}

void main() {
  vec3 color = vec3(1.0, 0.0, 0.0);
  float depth = gl_FragCoord.z / gl_FragCoord.w;

  color_depth = vec4(color, depth);
  normal_specularity = vec4(getNormal(), 1.0);
}