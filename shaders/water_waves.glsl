#version 330 core
// Astron Engine — Oceano Procedural Gerstner & Caustics Shader (GLSL 3.30 Core)

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

float waveHeight(vec2 p, float t) {
    float w1 = sin(dot(p, vec2(1.2, 0.7)) * 2.4 + t * 2.1) * 0.14;
    float w2 = sin(dot(p, vec2(-0.8, 1.3)) * 3.9 - t * 2.8) * 0.08;
    float w3 = cos(dot(p, vec2(0.5, -1.5)) * 6.2 + t * 3.4) * 0.04;
    return w1 + w2 + w3;
}

void main() {
    vec2 xz = vWorldPos.xz;
    float eps = 0.06;
    float hC = waveHeight(xz, uTime);
    float hX = waveHeight(xz + vec2(eps, 0.0), uTime);
    float hZ = waveHeight(xz + vec2(0.0, eps), uTime);

    vec3 waveN = normalize(vec3(hC - hX, eps * 2.4, hC - hZ));
    vec3 N = normalize(mix(normalize(vNormal), waveN, 0.75));
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(L + V);

    vec3 deepWater    = vec3(0.02, 0.16, 0.34);
    vec3 shallowWater = vec3(0.06, 0.56, 0.74);
    float crest = clamp((hC + 0.22) * 2.2, 0.0, 1.0);
    vec3 waterBase = mix(deepWater, shallowWater, crest);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 128.0);
    float fres = pow(1.0 - max(dot(N, V), 0.0), 4.0);

    float foam = smoothstep(0.16, 0.24, hC);
    vec3 skyReflect = vec3(0.42, 0.62, 0.88);

    vec3 color = waterBase * (uAmbientColor + uLightColor * diff * uLightIntensity)
               + skyReflect * fres * 0.65
               + uLightColor * spec * 1.4
               + vec3(0.92, 0.97, 1.0) * foam * 0.45;

    FragColor = vec4(color, clamp(uBaseColor.a * 0.88, 0.35, 0.95));
}
