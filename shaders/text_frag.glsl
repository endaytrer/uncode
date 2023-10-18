#version 330 core

in vec2 uv;
in vec4 frag_fg_color;
in vec4 frag_bg_color;
uniform float time;
uniform sampler2D font;
void main() {
    // gl_FragColor = 0.5 * frag_color + 0.5 * vec4((sin(time) + 1.0) / 2.0, (cos(time) + 1.0) / 2.0, (cos(2 * time) + 1.0) / 2.0, 1);
    float alpha = texture(font, uv).r;
    gl_FragColor = frag_fg_color * alpha + frag_bg_color * (1.0 - alpha);
}