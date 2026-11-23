#version 330 core
in vec2 vNdc;
out vec4 FragColor;

uniform mat4 uInvViewProj;
uniform mat4 uViewProj;
uniform vec3 uCamPos;
uniform int uShowGrid;

vec3 unprojectPoint(float x, float y, float z) {
    vec4 p = uInvViewProj * vec4(x, y, z, 1.0);
    return p.xyz / max(abs(p.w), 1e-6);
}

void main() {
    vec3 nearPt = unprojectPoint(vNdc.x, vNdc.y, -1.0);
    vec3 farPt  = unprojectPoint(vNdc.x, vNdc.y,  1.0);
    vec3 rayDir = normalize(farPt - nearPt);

    // Gradiente atmosférico de horizonte idêntico ao editor da Astron Engine
    float h = rayDir.y;
    vec3 zenCol     = vec3(0.115, 0.122, 0.140);
    vec3 horizonCol = vec3(0.420, 0.432, 0.455);
    vec3 groundBg   = vec3(0.055, 0.058, 0.066);

    vec3 sky;
    if (h >= 0.0) {
        float k = pow(1.0 - clamp(h * 2.6, 0.0, 1.0), 3.0);
        sky = mix(zenCol, horizonCol, k);
    } else {
        float k = pow(1.0 - clamp(-h * 5.0, 0.0, 1.0), 2.5);
        sky = mix(groundBg, horizonCol * 0.72, k);
    }

    // Interseção do raio com o plano y = 0 para grade 3D infinita + eixos X/Z
    if (uShowGrid != 0 && abs(rayDir.y) > 1e-4) {
        float t = -nearPt.y / rayDir.y;
        if (t > 0.0) {
            vec3 hit = nearPt + t * rayDir;
            vec4 clip = uViewProj * vec4(hit, 1.0);
            float ndcDepth = clip.z / clip.w;
            gl_FragDepth = clamp(ndcDepth * 0.5 + 0.5, 0.0, 0.9998);

            float dist = length(hit.xz - uCamPos.xz);
            float fade = exp(-dist * 0.022);

            vec2 coord = hit.xz;
            vec2 deriv = fwidth(coord);

            // Grade menor (1m) e maior (5m)
            vec2 grid1 = abs(fract(coord - 0.5) - 0.5) / max(deriv, vec2(1e-4));
            float line1 = 1.0 - min(min(grid1.x, grid1.y), 1.0);

            vec2 grid5 = abs(fract(coord * 0.2 - 0.5) - 0.5) / max(deriv * 0.2, vec2(1e-4));
            float line5 = 1.0 - min(min(grid5.x, grid5.y), 1.0);

            vec4 gridCol = vec4(0.26, 0.28, 0.31, max(line1 * 0.32, line5 * 0.58) * fade);

            // Eixo X (vermelho) quando Z ~= 0
            float axisX = 1.0 - min(abs(coord.y) / max(deriv.y * 1.8, 1e-4), 1.0);
            if (axisX > 0.01) {
                gridCol = mix(gridCol, vec4(0.88, 0.20, 0.22, 0.92 * fade), axisX);
            }

            // Eixo Z (azul) quando X ~= 0
            float axisZ = 1.0 - min(abs(coord.x) / max(deriv.x * 1.8, 1e-4), 1.0);
            if (axisZ > 0.01) {
                gridCol = mix(gridCol, vec4(0.22, 0.48, 0.95, 0.92 * fade), axisZ);
            }

            vec3 finalRgb = mix(sky, gridCol.rgb, gridCol.a);
            FragColor = vec4(finalRgb, 1.0);
            return;
        }
    }

    gl_FragDepth = 0.9999;
    FragColor = vec4(sky, 1.0);
}
