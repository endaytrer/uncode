#version 330 core

in vec4 frag_color;
uniform float time;

void main() {
    gl_FragColor = 0.5 * frag_color + 0.5 * vec4((sin(time) + 1.0) / 2.0, (cos(time) + 1.0) / 2.0, (cos(2 * time) + 1.0) / 2.0, 1);
}