#version 460 core

uniform sampler2D cascade0;
uniform sampler2D cascade1;
uniform sampler2D cascade2;

noperspective in vec2 fragUv;

out vec3 out_color;

void main() {
  if (fragUv.x < 0.334) {
    vec2 sampleUv = fragUv * vec2(3.0, 1.0);
    float depth = texture(cascade0, sampleUv).r;

    out_color = vec3(depth);
  } else if (fragUv.x < 0.667) {
    vec2 sampleUv = (fragUv - vec2(0.333, 0.0)) * vec2(3.0, 1.0);
    float depth = texture(cascade1, sampleUv).r;

    out_color = vec3(depth);
  } else {
    vec2 sampleUv = (fragUv - vec2(0.667, 0.0)) * vec2(3.0, 1.0);
    float depth = texture(cascade2, sampleUv).r;

    out_color = vec3(depth);
  }
}