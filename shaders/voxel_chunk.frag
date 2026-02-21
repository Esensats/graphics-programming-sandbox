#version 460 core

in vec3 v_world_pos;
in vec3 v_color;

uniform vec3 u_camera_world;
uniform vec3 u_fog_color;
uniform float u_fog_near;
uniform float u_fog_far;
uniform float u_alpha;

out vec4 FragColor;

void main() {
    float height_light = clamp(0.72 + v_world_pos.y * 0.0025, 0.55, 1.0);
    vec3 lit = v_color * height_light;

    float dist = distance(v_world_pos, u_camera_world);
    float fog_t = clamp((dist - u_fog_near) / max(u_fog_far - u_fog_near, 0.001), 0.0, 1.0);
    vec3 final_rgb = mix(lit, u_fog_color, fog_t);

    FragColor = vec4(final_rgb, u_alpha);
}
