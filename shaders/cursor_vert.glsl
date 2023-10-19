#version 310 es
precision mediump float;

in vec2 position;

uniform vec2 resolution;
uniform vec2 viewport_pos;
uniform vec2 size;

void main() {

    vec2 vert_position = position;
    if ((gl_VertexID & 1) != 0) {
        vert_position += vec2(size.x, 0.0);
    }
    if ((gl_VertexID & 2) != 0) {
        vert_position += vec2(0.0, size.y);
    }

    gl_Position = vec4(2.0 * (vert_position.x - viewport_pos.x) / resolution.x - 1.0, 1.0 - 2.0 * (vert_position.y - viewport_pos.y) / resolution.y, 0.0, 1.0);
}
