#version 460 core

#define USE_SCREEN_SPACE_AMBIENT_OCCLUSION 1
#define USE_SCREEN_SPACE_GLOBAL_ILLUMINATION 1

uniform vec2 screenSize;
uniform sampler2D colorAndDepth;
uniform sampler2D normalAndEmissivity;
uniform sampler2D indirectLightT1;
uniform sampler2D indirectLightT2;
uniform vec3 cameraPosition;
uniform mat4 view;
uniform mat4 inverseView;
uniform mat4 projection;
uniform mat4 inverseProjection;
uniform mat4 viewT1;
uniform mat4 viewT2;
uniform float time;

noperspective in vec2 fragUv;

layout (location = 0) out vec4 out_gi_and_ao;

#include "utils/conversion.glsl";
#include "utils/random.glsl";
#include "utils/helpers.glsl";
#include "utils/gl.glsl";

const vec3[] ssao_sample_points = {
  vec3(-0.690662, 0.510105, 0.158674),
  vec3(0.590586, 0.315846, 0.680247),
  vec3(0.7465, 0.309499, -0.215765),
  vec3(-0.14527, 0.028615, -0.194656),
  vec3(-0.598687, -0.608664, 0.185548),
  vec3(0.106358, -0.104366, -0.032851),
  vec3(0.16197, -0.596914, -0.244301),
  vec3(0.275134, 0.778992, 0.253804),
  vec3(0.322689, 0.081919, -0.010093),
  vec3(0.489904, -0.034348, -0.178335),
  vec3(0.247347, -0.194624, 0.126782),
  vec3(-0.819231, -0.18382, 0.037931),
  vec3(0.412153, -0.291263, -0.328101),
  vec3(-0.532299, -0.621028, 0.240485),
  vec3(0.336204, -0.058378, -0.081821),
  vec3(-0.100673, -0.009447, 0.149728),
  vec3(-0.232848, 0.158023, 0.03827),
  vec3(0.183876, -0.47699, 0.712526),
  vec3(-0.04253, -0.229033, 0.342295),
  vec3(-0.56477, 0.128458, 0.559566),
  vec3(-0.080643, -0.114, -0.039208),
  vec3(0.424498, 0.445644, 0.452789),
  vec3(0.250005, -0.189523, 0.226624),
  vec3(0.038703, 0.01734, -0.237867),
  vec3(0.27446, -0.193014, -0.67947),
  vec3(-0.527069, -0.286647, -0.156616),
  vec3(-0.034006, 0.222777, -0.304521),
  vec3(-0.353954, -0.489028, 0.148017),
  vec3(0.388574, 0.502973, -0.1629),
  vec3(0.311415, 0.365883, -0.254277),
  vec3(-0.081484, -0.157306, -0.226523),
  vec3(0.110136, 0.112116, 0.070524)
};

const vec3[] rvecs = {
  vec3(-0.71521, 0.366644, 0),
  vec3(0.594004, 0.201843, 0),
  vec3(-0.532579, 0.951231, 0),
  vec3(0.519711, 0.641485, 0),
  vec3(0.517936, -0.99854, 0),
  vec3(-0.561533, -0.332276, 0),
  vec3(0.756026, -0.451731, 0),
  vec3(0.911829, -0.785381, 0),
  vec3(-0.146327, 0.749293, 0),
  vec3(-0.009636, -0.569109, 0),
  vec3(0.949664, -0.198098, 0),
  vec3(-0.082819, 0.212425, 0),
  vec3(0.132091, -0.349236, 0),
  vec3(0.394824, 0.162642, 0),
  vec3(-0.878207, -0.434712, 0),
  vec3(0.558003, 0.228045, 0)
};

vec2 rotatedVogelDisc(int samples, int index) {
  float rotation = noise(fract(1.0 + time)) * 3.141592 * 2.0;
  float theta = 2.4 * index + rotation;
  float radius = sqrt(float(index) + 0.5) / sqrt(float(samples));

  return radius * vec2(cos(theta), sin(theta));
}

// @todo denoise
// @todo fix distance occlusion issues due to depth mipmap sampling
float getScreenSpaceAmbientOcclusionContribution(float fragment_depth, vec3 fragment_position, vec3 fragment_normal) {
  const int TOTAL_SAMPLES = 32;
  const float radius = 30.0;
  vec3 contribution = vec3(0);
  vec2 texel_size = 1.0 / screenSize;
  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float occlusion = 0.0;

  vec3 view_normal = transpose(inverse(mat3(view))) * fragment_normal;

  // int x = int(mod(gl_FragCoord.x, 4.0));
  // int y = int(mod(gl_FragCoord.y, 4.0));
  // vec3 rvec = rvecs[x * y + x];

  vec3 rvec = vec3(noise(1.0), noise(2.0), noise(3.0));
  vec3 tangent = normalize(rvec - view_normal * dot(rvec, view_normal));
  vec3 bitangent = cross(view_normal, tangent);
  mat3 tbn = mat3(tangent, bitangent, view_normal);

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    vec3 sample_offset = tbn * ssao_sample_points[i];
    vec3 world_sample_position = fragment_position + sample_offset * 3.0;
    vec3 fragment_to_sample = normalize(world_sample_position - fragment_position);

    if (dot(fragment_normal, fragment_to_sample) < 0.1) {
      continue;
    }

    vec3 view_sample_position = glVec3(view * glVec4(world_sample_position));
    vec2 screen_sample_position = getScreenCoordinates(view_sample_position, projection);
    float sample_depth = textureLod(colorAndDepth, screen_sample_position, 3).w;
    float linear_sample_depth = getLinearizedDepth(sample_depth);

    if (linear_sample_depth < view_sample_position.z) {
      float occluder_distance = view_sample_position.z - linear_sample_depth;

      occlusion += 2.0 * mix(1.0, 0.0, saturate(occluder_distance / 3.0));
    }
  }

  return occlusion / float(TOTAL_SAMPLES);
}

vec3 getScreenSpaceGlobalIlluminationContribution(float fragment_depth, vec3 fragment_position, vec3 fragment_normal) {
  const int TOTAL_SAMPLES = 30;
  const float max_sample_radius = 1000.0;
  const float max_brightness = 100.0;
  vec2 texel_size = 1.0 / screenSize;
  vec3 global_illumination = vec3(0.0);

  float linearized_fragment_depth = getLinearizedDepth(fragment_depth);
  float radius = max_sample_radius * saturate(1.0 / (linearized_fragment_depth * 0.01));

  // Bounce a ray off the surface and sample points
  // around the bounce ray screen coordinates
  vec3 camera_to_fragment = normalize(fragment_position - cameraPosition);
  vec3 reflection_vector = reflect(camera_to_fragment, fragment_normal);
  vec3 world_bounce_ray = fragment_position + reflection_vector * 10.0;
  vec3 view_bounce_ray = glVec3(view * glVec4(world_bounce_ray));
  vec2 bounce_ray_coords = getScreenCoordinates(view_bounce_ray, projection);

  for (int i = 0; i < TOTAL_SAMPLES; i++) {
    vec2 offset = texel_size * radius * rotatedVogelDisc(TOTAL_SAMPLES, i);
    vec2 coords = bounce_ray_coords + offset;

    if (isOffScreen(coords, 0.0)) {
      continue;
    }

    vec4 sample_color_and_depth = textureLod(colorAndDepth, coords, 3);

    // @todo why is this necessary? why are certain sample
    // color components < 0? note: this is to do with
    // probe reflectors
    sample_color_and_depth.r = saturate(sample_color_and_depth.r);
    sample_color_and_depth.g = saturate(sample_color_and_depth.g);
    sample_color_and_depth.b = saturate(sample_color_and_depth.b);

    // Diminish illumination where the sample emits
    // less incident bounce light onto the fragment
    vec3 sample_position = getWorldPosition(sample_color_and_depth.w, fragUv + offset, inverseProjection, inverseView);
    float sample_distance = distance(fragment_position, sample_position);
    vec3 normalized_fragment_to_sample = (sample_position - fragment_position) / sample_distance;
    float incidence_factor = max(0.0, dot(fragment_normal, normalized_fragment_to_sample));

    // Diminish illumination with distance
    float distance_factor = max_brightness * saturate(1.0 / sample_distance) * saturate(linearized_fragment_depth / 100.0);

    global_illumination += sample_color_and_depth.rgb * incidence_factor * distance_factor;
  }

  return global_illumination / float(TOTAL_SAMPLES);
}

/**
 * Determines whether a previous-frame temporal sample
 * has enough color to use for denoising. Fragments where
 * geometry was not rendered during the previous frame
 * will be black or near black, and we want to avoid
 * including these in the temporally averaged color. This
 * is mainly a problem where moving geometry edges meet
 * the sky, and reprojected samples are taken from points
 * where no geometry was rendered during an earlier frame,
 * and the sampled color would be invalid.
 */
bool isUsableTemporalSampleColor(vec3 color) {
  const float threshold = 0.04;

  return color.r >= threshold || color.g >= threshold || color.b >= threshold;
}

void main() {
  vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);

  if (frag_color_and_depth.w == 1.0) {
    discard;
  }

  vec3 fragment_position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);
  vec3 fragment_normal = texture(normalAndEmissivity, fragUv).xyz;
  vec3 global_illumination = vec3(0.0);
  float ambient_occlusion = 0.0;

  #if USE_SCREEN_SPACE_AMBIENT_OCCLUSION == 1
    ambient_occlusion = getScreenSpaceAmbientOcclusionContribution(frag_color_and_depth.w, fragment_position, fragment_normal);
  #endif

  #if USE_SCREEN_SPACE_GLOBAL_ILLUMINATION == 1
    global_illumination = getScreenSpaceGlobalIlluminationContribution(frag_color_and_depth.w, fragment_position, fragment_normal);
  #endif

  vec3 fragment_position_t1 = glVec3(viewT1 * glVec4(fragment_position));
  vec3 fragment_position_t2 = glVec3(viewT2 * glVec4(fragment_position));
  vec2 fragUv_t1 = getScreenCoordinates(fragment_position_t1.xyz, projection);
  vec2 fragUv_t2 = getScreenCoordinates(fragment_position_t2.xyz, projection);
  int total_temporal_samples = 1;

  out_gi_and_ao = vec4(global_illumination, ambient_occlusion);

  if (!isOffScreen(fragUv_t1, 0.001)) {
    vec4 sample_t1 = texture(indirectLightT1, fragUv_t1);

    if (isUsableTemporalSampleColor(sample_t1.rgb)) {
      out_gi_and_ao += sample_t1;
      total_temporal_samples++;
    }
  }

  if (!isOffScreen(fragUv_t2, 0.001)) {
    vec4 sample_t2 = texture(indirectLightT2, fragUv_t2);

    if (isUsableTemporalSampleColor(sample_t2.rgb)) {
      out_gi_and_ao += sample_t2;
      total_temporal_samples++;
    }
  }

  out_gi_and_ao /= float(total_temporal_samples);
}