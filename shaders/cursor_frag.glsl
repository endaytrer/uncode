#version 330 core

uniform vec4 fg_color;
uniform vec4 bg_color;
uniform float time;
out vec4 fragColor;

void main() {
    float T = 1.5;
    float PI = 3.14159265;
    float STRENGTH = 20;
    float omega = 2.0 * PI / T;
    float x = cos(omega * time);
    x = 1.0 / (1.0 + exp(-STRENGTH * x));
    fragColor = fg_color * x + bg_color * (1.0 - x);
}