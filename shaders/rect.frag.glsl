#version 310 es
precision highp float;

in vec4 frag_color;
out vec4 fragColor;

void main() {
    fragColor = frag_color;
}