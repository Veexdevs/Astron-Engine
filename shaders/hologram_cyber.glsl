#version 330 core
// Astron Engine — Sci-Fi Cyber Hologram Shader (GLSL 3.30 Core)

in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform vec4  uBaseColor;
uniform vec3  uCamPos;
uniform vec3  uLightDir;
uniform vec3  uLightColor;
uniform float uLightIntensity;
uniform vec3  uAmbientColor;
uniform int   uMaterialType;
uniform float uTime;
uniform int   uSelected;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);

    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 2.3);
    float scanFine   = 0.5 + 0.5 * sin(vWorldPos.y * 42.0 - uTime * 7.5);
    float scanCoarse = smoothstep(0.75, 1.0, 0.5 + 0.5 * sin(vWorldPos.y * 4.5 - uTime * 2.1));
    float flicker    = 0.92 + 0.08 * sin(uTime * 23.0 + vWorldPos.x * 3.0);

    vec3 holoTint = mix(vec3(0.08, 0.72, 1.0), uBaseColor.rgb, 0.35);
    vec3 coreGlow = holoTint * (0.28 + 0.45 * scanFine) + vec3(0.45, 0.92, 1.0) * scanCoarse * 0.55;
    vec3 rimGlow  = vec3(0.35, 0.88, 1.0) * fresnel * 1.65;

    vec3 color = (coreGlow + rimGlow) * flicker;
    float alpha = clamp(0.38 + fresnel * 0.55 + scanCoarse * 0.25, 0.2, 0.96);

    if (uSelected == 1) {
        color += vec3(1.0, 0.85, 0.35) * fresnel * 0.45;
    }

    FragColor = vec4(color, alpha);
}
