#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;

out vec4 frag_color;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    frag_color = vec4(color, 1.0);
}