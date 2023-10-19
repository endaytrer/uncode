#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 size;
layout(location = 2) in float uv_offset_x;
layout(location = 3) in vec2 uv_size;
layout(location = 4) in vec3 fg_color;
layout(location = 5) in vec3 bg_color;

uniform vec2 viewport_size;
uniform vec2 viewport_pos;
uniform int viewport_scale;
out vec3 frag_fg_color;
out vec3 frag_bg_color;
out vec2 uv;
void main() {
    vec2 vert_position = position;
    uv = vec2(uv_offset_x, 0.0);
    if ((gl_VertexID & 1) != 0) {
        vert_position += vec2(size.x, 0.0);
        uv.x += uv_size.x;
    }
    if ((gl_VertexID & 2) != 0) {
        vert_position += vec2(0.0, size.y);
        uv.y += uv_size.y;
    }
    gl_Position = vec4(2.0 * (vert_position.x - viewport_pos.x * viewport_scale) / (viewport_size.x * viewport_scale) - 1.0, 1.0 - 2.0 * (vert_position.y - viewport_pos.y * viewport_scale) / (viewport_size.y * viewport_scale) , 0.0, 1.0);
    frag_fg_color = fg_color;
    frag_bg_color = bg_color;
}