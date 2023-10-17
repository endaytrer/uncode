#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 size;
layout(location = 2) in float ch;
layout(location = 3) in vec3 color;

uniform vec2 resolution;
out vec4 frag_color;

void main() {
    vec2 vert_position = position;
    if ((gl_VertexID & 1) != 0) {
        vert_position += vec2(size.x, 0.0);
    }
    if ((gl_VertexID & 2) != 0) {
        vert_position += vec2(0.0, size.y);
    }
    gl_Position = vec4(2.0 * vert_position.x / resolution.x - 1.0, 1.0 - 2.0 * vert_position.y / resolution.y, 0.0, 1.0);
    frag_color = vec4(color, 1.0);
}