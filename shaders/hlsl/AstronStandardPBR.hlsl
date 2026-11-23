// Astron Engine — DirectX 11 / DirectX 12 HLSL Standard PBR Shader (Shader Model 5.0)
// Usado no pipeline Direct3D nativo Windows (.exe) da Astron Engine.

cbuffer AstronPerFrameCB : register(b0)
{
    float4x4 uViewProj;
    float3   uCameraPos;
    float    uTime;
    float3   uSunDirection;
    float    uSunIntensity;
    float3   uSunColor;
    float    uExposure;
    float3   uAmbientColor;
    float    _pad0;
};

cbuffer AstronPerObjectCB : register(b1)
{
    float4x4 uWorld;
    float4x4 uNormalMatrix;
    float4   uAlbedo;
    float    uMetallic;
    float    uRoughness;
    float    uEmissive;
    uint     uSelected;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
};

struct PSInput
{
    float4 clipPos  : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : TEXCOORD1;
    float2 uv       : TEXCOORD2;
};

static const float ASTRON_PI = 3.14159265359f;

PSInput VSMain(VSInput input)
{
    PSInput o;
    float4 wp  = mul(uWorld, float4(input.position, 1.0f));
    o.worldPos = wp.xyz;
    o.clipPos  = mul(uViewProj, wp);
    o.normal   = normalize(mul((float3x3)uNormalMatrix, input.normal));
    o.uv       = input.uv;
    return o;
}

float DistributionGGX_HLSL(float3 N, float3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float nh = saturate(dot(N, H));
    float d  = (nh * nh) * (a2 - 1.0f) + 1.0f;
    return a2 / max(ASTRON_PI * d * d, 0.0001f);
}

float GeometrySchlickGGX_HLSL(float nv, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) * 0.125f;
    return nv / max(nv * (1.0f - k) + k, 0.0001f);
}

float3 FresnelSchlick_HLSL(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - saturate(cosTheta), 5.0f);
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 V = normalize(uCameraPos - input.worldPos);
    float3 L = normalize(-uSunDirection);
    float3 H = normalize(V + L);

    float3 albedo = pow(abs(uAlbedo.rgb), 2.2f);
    float3 F0     = lerp(float3(0.04f, 0.04f, 0.04f), albedo, saturate(uMetallic));

    float  D  = DistributionGGX_HLSL(N, H, max(uRoughness, 0.05f));
    float  G  = GeometrySchlickGGX_HLSL(saturate(dot(N, V)), uRoughness)
              * GeometrySchlickGGX_HLSL(saturate(dot(N, L)), uRoughness);
    float3 F  = FresnelSchlick_HLSL(saturate(dot(H, V)), F0);

    float  denom    = 4.0f * saturate(dot(N, V)) * saturate(dot(N, L)) + 0.0001f;
    float3 specular = (D * G * F) / denom;
    float3 kD       = (1.0f - F) * (1.0f - saturate(uMetallic));

    float  NdotL    = saturate(dot(N, L));
    float3 radiance = uSunColor * uSunIntensity * 2.2f;
    float3 direct   = (kD * albedo / ASTRON_PI + specular) * radiance * NdotL;
    float3 ambient  = uAmbientColor * albedo * 0.35f + albedo * uEmissive;

    float3 hdr = (ambient + direct) * max(uExposure, 0.1f);
    if (uSelected != 0u)
    {
        float rim = pow(1.0f - saturate(dot(N, V)), 3.0f);
        hdr += float3(0.24f, 0.58f, 1.0f) * rim * 0.85f;
    }

    float3 mapped = (hdr * (2.51f * hdr + 0.03f)) / (hdr * (2.43f * hdr + 0.59f) + 0.14f);
    return float4(pow(saturate(mapped), 1.0f / 2.2f), uAlbedo.a);
}
