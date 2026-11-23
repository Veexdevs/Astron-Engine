#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
out vec4 FragColor;

uniform vec4 uColor;
uniform vec3 uCamPos;
uniform int uMaterialType; // 0=Plastic, 1=Metal, 2=Wood, 3=Glass, 4=Neon, 5=Matte

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 L1 = normalize(vec3(0.55, 0.85, 0.48));
    vec3 L2 = normalize(vec3(-0.45, 0.30, -0.60));

    // Material Neon (emissivo)
    if (uMaterialType == 4) {
        float rim = pow(1.0 - max(dot(N, V), 0.0), 2.0);
        vec3 neon = uColor.rgb * (1.25 + 0.65 * rim);
        FragColor = vec4(neon, uColor.a);
        return;
    }

    float diff1 = max(dot(N, L1), 0.0);
    float diff2 = max(dot(N, L2), 0.0) * 0.28;
    float hemi  = 0.28 + 0.16 * (N.y * 0.5 + 0.5);

    float shininess = 36.0;
    float specStrength = 0.32;
    vec3 base = uColor.rgb;

    if (uMaterialType == 1) {          // Metal
        shininess = 96.0;
        specStrength = 0.85;
        base *= 0.90;
    } else if (uMaterialType == 2) {   // Wood
        shininess = 14.0;
        specStrength = 0.12;
        float grain = 0.92 + 0.08 * sin(vWorldPos.x * 14.0 + vWorldPos.z * 9.0);
        base *= vec3(0.78, 0.54, 0.34) * grain;
    } else if (uMaterialType == 3) {   // Glass
        shininess = 128.0;
        specStrength = 0.95;
    } else if (uMaterialType == 5) {   // Matte
        shininess = 6.0;
        specStrength = 0.04;
    }

    vec3 H = normalize(L1 + V);
    float spec = pow(max(dot(N, H), 0.0), shininess) * specStrength;

    vec3 col = base * (hemi + diff1 * 0.72 + diff2) + vec3(spec);
    float alpha = (uMaterialType == 3) ? min(uColor.a, 0.48) : uColor.a;
    FragColor = vec4(col, alpha);
}
