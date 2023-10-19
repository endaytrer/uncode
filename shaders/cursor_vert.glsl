#version 330 core

in vec2 position;

uniform vec2 viewport_size;
uniform vec2 viewport_pos;
uniform int viewport_scale;
uniform vec2 size;

void main() {

    vec2 vert_position = position;
    if ((gl_VertexID & 1) != 0) {
        vert_position += vec2(size.x, 0.0);
    }
    if ((gl_VertexID & 2) != 0) {
        vert_position += vec2(0.0, size.y);
    }

    gl_Position = vec4(2.0 * (vert_position.x - viewport_pos.x * viewport_scale) / (viewport_size.x * viewport_scale) - 1.0, 1.0 - 2.0 * (vert_position.y - viewport_pos.y * viewport_scale) / (viewport_size.y * viewport_scale) , 0.0, 1.0);
}