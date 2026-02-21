#version 460 core

in vec3 v_world_pos;
in vec2 v_uv;
flat in float v_material_layer;

uniform vec3 u_camera_world;
uniform vec3 u_fog_color;
uniform float u_fog_near;
uniform float u_fog_far;
uniform float u_alpha;
uniform sampler2DArray u_albedo_array;

out vec4 FragColor;

void main() {
    vec3 base = texture(u_albedo_array, vec3(v_uv, v_material_layer)).rgb;

    float height_light = clamp(0.72 + v_world_pos.y * 0.0025, 0.55, 1.0);
    vec3 lit = base * height_light;

    float dist = distance(v_world_pos, u_camera_world);
    float fog_t = clamp((dist - u_fog_near) / max(u_fog_far - u_fog_near, 0.001), 0.0, 1.0);
    vec3 final_rgb = mix(lit, u_fog_color, fog_t);

    FragColor = vec4(final_rgb, u_alpha);
}
