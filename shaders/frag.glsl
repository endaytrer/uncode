#version 330 core

in vec2 uv;
in vec4 frag_color;
uniform float time;
uniform sampler2D font;
void main() {
    // gl_FragColor = 0.5 * frag_color + 0.5 * vec4((sin(time) + 1.0) / 2.0, (cos(time) + 1.0) / 2.0, (cos(2 * time) + 1.0) / 2.0, 1);
    gl_FragColor = frag_color * vec4(texture(font, uv).r, texture(font, uv).r, texture(font, uv).r, 1.0);
}