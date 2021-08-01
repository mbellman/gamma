#version 460 core

uniform bool hasTexture = false;
uniform bool hasNormalMap = false;
uniform sampler2D meshTexture;
uniform sampler2D meshNormalMap;

in vec3 fragNormal;
in vec3 fragTangent;
in vec3 fragBitangent;
in vec2 fragUv;

layout (location = 0) out vec4 out_colorAndDepth;
layout (location = 1) out vec4 out_normalAndSpecularity;

vec3 getNormal() {
  vec3 n_fragNormal = normalize(fragNormal);

  if (hasNormalMap) {
    vec3 mappedNormal = texture(meshNormalMap, fragUv).rgb * 2.0 - vec3(1.0);

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
  vec3 color = hasTexture ? texture(meshTexture, fragUv).rgb : vec3(1.0);
  float depth = gl_FragCoord.z;

  out_colorAndDepth = vec4(color, depth);
  out_normalAndSpecularity = vec4(getNormal(), 1.0);
}