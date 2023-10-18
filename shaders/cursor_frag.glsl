#version 330 core

uniform vec4 color;
uniform float time;

void main() {
    float T = 1;
    float PI = 3.14159265;
    float omega = 2.0 * PI / T;
    float x = cos(omega * time);
    x = (x > 0 ? 1 : -1) * pow(abs(x), 1.0 / 3.0);
    x = (x + 1.0) / 2.0;
    gl_FragColor = color * x + vec4(0.0, 0.0, 0.0, 1.0) * (1.0 - x);
}