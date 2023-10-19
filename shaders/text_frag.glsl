#version 310 es
precision mediump float;

in vec2 uv;
in vec3 frag_fg_color;
in vec3 frag_bg_color;
uniform float time;
uniform sampler2D font;
out vec4 fragColor;

vec3 hsv2rgb(vec3 hsv) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(hsv.xxx + K.xyz) * 6.0 - K.www);
    return hsv.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsv.y);
}
void main() {
    float offset = gl_FragCoord.x - gl_FragCoord.y;
    float omega = 1.0, k = 0.002;
    vec3 color = hsv2rgb(vec3((sin(omega * time - k * offset) + 1.0) / 2.0, 0.4, 1.0));
    float alpha = texture(font, uv).r;
    fragColor = vec4(frag_fg_color * color * alpha + frag_bg_color * (1.0 - alpha), 1.0);
}
