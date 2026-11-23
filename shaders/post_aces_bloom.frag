#version 330 core
// Astron Engine — ACES Filmic Tonemapping + Chromatic Bloom + Vignette (GLSL 3.30 Core)

in vec2 vUV;

uniform sampler2D uSceneTex;
uniform vec2      uTexelSize;
uniform float     uExposure;
uniform float     uBloomStrength;
uniform float     uVignette;

out vec4 FragColor;

vec3 acesTonemap(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 sampleBright(vec2 uv) {
    vec3 c = texture(uSceneTex, uv).rgb;
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    return c * smoothstep(0.72, 1.25, lum);
}

void main() {
    vec3 hdr = texture(uSceneTex, vUV).rgb;

    // 9-tap tent bloom kernel
    vec2 o = uTexelSize * 2.5;
    vec3 bloom = sampleBright(vUV) * 0.25
               + sampleBright(vUV + vec2( o.x,  0.0)) * 0.125
               + sampleBright(vUV + vec2(-o.x,  0.0)) * 0.125
               + sampleBright(vUV + vec2( 0.0,  o.y)) * 0.125
               + sampleBright(vUV + vec2( 0.0, -o.y)) * 0.125
               + sampleBright(vUV + vec2( o.x,  o.y)) * 0.0625
               + sampleBright(vUV + vec2(-o.x,  o.y)) * 0.0625
               + sampleBright(vUV + vec2( o.x, -o.y)) * 0.0625
               + sampleBright(vUV + vec2(-o.x, -o.y)) * 0.0625;

    vec3 combined = (hdr + bloom * uBloomStrength) * uExposure;
    vec3 mapped   = acesTonemap(combined);

    vec2 centered = vUV * 2.0 - 1.0;
    float vig = clamp(1.0 - dot(centered, centered) * uVignette * 0.35, 0.0, 1.0);

    FragColor = vec4(pow(mapped * vig, vec3(1.0 / 2.2)), 1.0);
}
