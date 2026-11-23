#version 330 core
// Astron Engine — Cook-Torrance GGX Microfacet PBR Fragment Shader (GLSL 3.30 Core)

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

const float PI = 3.14159265359;

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 0.0001);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float ggx2 = geometrySchlickGGX(max(dot(N, V), 0.0), roughness);
    float ggx1 = geometrySchlickGGX(max(dot(N, L), 0.0), roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    float metallic  = 0.85;
    float roughness = 0.22;
    vec3 albedo     = pow(uBaseColor.rgb, vec3(2.2));

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float NDF = distributionGGX(N, H, roughness);
    float G   = geometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular    = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);

    vec3 radiance = uLightColor * uLightIntensity * 2.4;
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;

    // Reflexo hemisférico de céu/solo para metais
    vec3 R = reflect(-V, N);
    vec3 envSky = mix(vec3(0.05, 0.06, 0.09), vec3(0.32, 0.44, 0.68), clamp(R.y * 0.5 + 0.5, 0.0, 1.0));
    vec3 ambient = (uAmbientColor * albedo * 0.35) + envSky * F * (1.0 - roughness * 0.6);

    vec3 color = ambient + Lo;
    if (uSelected == 1) {
        float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
        color += vec3(0.22, 0.58, 1.0) * rim * 0.85;
    }

    // Tonemapping ACES + correção gama sRGB
    color = (color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14);
    color = pow(clamp(color, 0.0, 1.0), vec3(1.0 / 2.2));
    FragColor = vec4(color, uBaseColor.a);
}
