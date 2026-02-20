#version 460 core

out vec4 FragColor;
uniform vec2 u_resolution;
uniform float u_time;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    vec2 centered = uv * 2.0 - 1.0;
    centered.x *= u_resolution.x / u_resolution.y;

    float radial = length(centered);
    float waves = sin(10.0 * radial - u_time * 2.0);
    vec3 base = vec3(0.1, 0.2, 0.8);
    vec3 accent = vec3(0.9, 0.2, 0.6);
    vec3 color = mix(base, accent, 0.5 + 0.5 * waves);
    color *= 1.0 - smoothstep(0.2, 1.2, radial);

    FragColor = vec4(color, 1.0);
}
