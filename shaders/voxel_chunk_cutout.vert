#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in float a_material_layer;

uniform mat4 u_mvp;

out vec3 v_world_pos;
out vec2 v_uv;
flat out float v_material_layer;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_world_pos = a_position;
    v_uv = a_uv;
    v_material_layer = a_material_layer;
}
