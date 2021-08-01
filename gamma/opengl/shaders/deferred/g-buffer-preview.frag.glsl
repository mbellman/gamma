#version 460 core

uniform sampler2D colorAndDepth;
uniform sampler2D normalAndSpecularity;

noperspective in vec2 fragUv;

out vec3 out_color;

void main() {
  if (fragUv.x < 0.25) {
    // Albedo
    vec2 uv = fragUv * vec2(4.0, 1.0);
    vec4 colorAndDepthSample = texture(colorAndDepth, uv);

    out_color = colorAndDepthSample.rgb;
  } else if (fragUv.x < 0.5) {
    // Depth (adjusted for clarity)
    vec2 uv = (fragUv - vec2(0.25, 0.0)) * vec2(4.0, 1.0);
    vec4 colorAndDepthSample = texture(colorAndDepth, uv);

    out_color = vec3(1.0 - pow(colorAndDepthSample.w, 50.0));
  } else if (fragUv.x < 0.75) {
    // Normal
    vec2 uv = (fragUv - vec2(0.5, 0.0)) * vec2(4.0, 1.0);
    vec4 normalAndSpecularitySample = texture(normalAndSpecularity, uv);

    out_color = normalAndSpecularitySample.rgb;
  } else {
    // Specularity
    vec2 uv = (fragUv - vec2(0.75, 0.0)) * vec2(4.0, 1.0);
    vec4 normalAndSpecularitySample = texture(normalAndSpecularity, uv);

    out_color = vec3(normalAndSpecularitySample.w);
  }
}