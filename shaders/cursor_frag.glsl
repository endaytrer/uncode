#version 310 es
precision highp float;

uniform vec3 fg_color;
uniform float time;
out vec4 fragColor;

void main() {
    float T = 1.5;
    float PI = 3.14159265;
    float STRENGTH = 20.0;
    float omega = 2.0 * PI / T;
    float x = cos(omega * time);
    x = 1.0 / (1.0 + exp(-STRENGTH * x));
    fragColor = vec4(fg_color, x);
}
