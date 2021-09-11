vec4 frag_color_and_depth = texture(colorAndDepth, fragUv);
vec3 position = getWorldPosition(frag_color_and_depth.w, fragUv, inverseProjection, inverseView);

if (length(light.position - position) > light.radius) {
  // Discard any fragments outside of the light radius
  discard;
}

vec3 surface_to_light = light.position - position;
float light_distance = length(surface_to_light);
vec3 normalized_surface_to_light = surface_to_light / light_distance;
vec3 normalized_surface_to_camera = normalize(cameraPosition - position);
vec3 half_vector = normalize(normalized_surface_to_light + normalized_surface_to_camera);

float direction_factor = dot(normalized_surface_to_light * -1, normalize(light.direction));
float max_factor = 1.0;
float min_factor = 1.0 - (light.fov / 180.0);

if (direction_factor < min_factor) {
  // Discard any fragments outside of the spot light cone
  discard;
}

float range = max_factor - min_factor;
float adjusted_factor = max(direction_factor - min_factor, 0.0);
float spot_factor = sqrt(adjusted_factor / range);

vec4 frag_normal_and_specularity = texture(normalAndSpecularity, fragUv);
vec3 normal = frag_normal_and_specularity.xyz;
vec3 color = frag_color_and_depth.rgb;

float incidence = max(dot(normalized_surface_to_light, normal), 0.0);
float attenuation = pow(1.0 / light_distance, 2);
float specularity = pow(max(dot(half_vector, normal), 0.0), 50);

// Have light intensity 'fall off' toward radius boundary
float hack_radial_influence = max(1.0 - light_distance / light.radius, 0.0);
// Taper light intensity more softly to preserve light with distance
float hack_soft_tapering = (20.0 * (light_distance / light.radius));
// Loosely approximates ambient/indirect lighting
vec3 hack_indirect_light = light.color * light.power * pow(max(1.0 - dot(normalized_surface_to_camera, normal), 0.0), 2) * indirect_light_factor;

vec3 radiant_flux = light.color * light.power * light.radius;
vec3 diffuse_term = radiant_flux * incidence * attenuation * hack_radial_influence * hack_soft_tapering + hack_indirect_light;
vec3 specular_term = radiant_flux * specularity * attenuation;

vec3 illuminated_color = color * spot_factor * (diffuse_term + specular_term);