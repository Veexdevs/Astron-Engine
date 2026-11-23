#version 330 core
// Shader customizável — Editor de Shader da Astron Engine (GLSL 3.30)
in vec3 vWorldPos;
in vec3 vNormal;
out vec4 FragColor;

uniform vec4 uColor;
uniform vec3 uCamPos;
uniform float uTime;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(vec3(sin(uTime), 0.85, cos(uTime)));
    float ndl = max(dot(N, L), 0.15);
    float pulse = 0.85 + 0.15 * sin(uTime * 3.0 + vWorldPos.y * 4.0);
    FragColor = vec4(uColor.rgb * ndl * pulse, uColor.a);
}
