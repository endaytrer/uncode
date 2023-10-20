#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 size;
layout(location = 2) in vec4 color;

uniform vec2 viewport_size;
uniform vec2 viewport_pos;
uniform int viewport_scale;

out vec4 frag_color;

void main() {

    vec2 vert_position = position;
    if ((gl_VertexID & 1) != 0) {
        vert_position += vec2(size.x, 0.0);
    }
    if ((gl_VertexID & 2) != 0) {
        vert_position += vec2(0.0, size.y);
    }
    
    gl_Position = vec4(
        2.0 * (vert_position.x - viewport_pos.x * viewport_scale) / (viewport_size.x * viewport_scale) - 1.0,
        1.0 - 2.0 * (vert_position.y - viewport_pos.y * viewport_scale) / (viewport_size.y * viewport_scale),
        0.0,
        1.0
    );

    frag_color = color;
}