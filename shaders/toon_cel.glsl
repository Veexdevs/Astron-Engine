#version 330 core
// Astron Engine — Stylized Toon / Cel-Shading + Rim Light Shader (GLSL 3.30 Core)

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
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(L + V);

    float ndotl = dot(N, L) * 0.5 + 0.5;
    float band;
    if (ndotl > 0.82)      band = 1.05;
    else if (ndotl > 0.52) band = 0.78;
    else if (ndotl > 0.28) band = 0.48;
    else                   band = 0.26;

    float specBand = step(0.92, pow(max(dot(N, H), 0.0), 32.0));
    float rimBand  = smoothstep(0.68, 0.76, 1.0 - max(dot(N, V), 0.0));

    vec3 lit = uBaseColor.rgb * (uAmbientColor * 0.45 + uLightColor * band * uLightIntensity)
             + uLightColor * specBand * 0.35
             + uBaseColor.rgb * rimBand * 0.32;

    if (uSelected == 1) {
        lit = mix(lit, vec3(0.32, 0.68, 1.0), rimBand * 0.65);
    }

    FragColor = vec4(lit, uBaseColor.a);
}
