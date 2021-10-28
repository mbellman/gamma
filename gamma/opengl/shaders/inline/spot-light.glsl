vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
vec3 position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);

if (length(light.position - position) > light.radius) {
  // Discard any fragments outside of the light radius
  discard;
}

vec3 surface_to_light = light.position - position;
float light_distance = length(surface_to_light);
vec3 normalized_surface_to_light = surface_to_light / light_distance;

// @optimize normalize light direction outside of the shader
float directional_alignment = dot(normalized_surface_to_light * -1, normalize(light.direction));
float cone_edge_alignment = 1.0 - (light.fov / 180.0);

if (directional_alignment < cone_edge_alignment) {
  // Discard any fragments outside of the light cone
  discard;
}

float alignment_range = 1.0 - cone_edge_alignment;
float edge_proximity = directional_alignment - cone_edge_alignment;
float spot_factor = sqrt(edge_proximity / alignment_range);

vec4 frag_normal_and_emissivity = texture(normalAndEmissivity, fragUv);
vec3 normal = frag_normal_and_emissivity.xyz;
vec3 color = frag_color_and_depth.rgb;

vec3 normalized_surface_to_camera = normalize(cameraPosition - position);
vec3 half_vector = normalize(normalized_surface_to_light + normalized_surface_to_camera);
float incidence = max(dot(normalized_surface_to_light, normal), 0.0);
float attenuation = pow(1.0 / light_distance, 2);
float specularity = pow(max(dot(half_vector, normal), 0.0), 50);

// Have light intensity 'fall off' toward radius boundary
float hack_radial_influence = max(1.0 - light_distance / light.radius, 0.0);
// Taper light intensity more softly to preserve light with distance
float hack_soft_tapering = (20.0 * (light_distance / light.radius));

vec3 radiant_flux = light.color * light.power * light.radius;
vec3 diffuse_term = color * radiant_flux * incidence * attenuation * hack_radial_influence * hack_soft_tapering * (1.0 - specularity);
vec3 specular_term = radiant_flux * specularity * attenuation;

float emissivity = frag_normal_and_emissivity.w;
vec3 illuminated_color = (diffuse_term + specular_term) * (1.0 - emissivity) * spot_factor;