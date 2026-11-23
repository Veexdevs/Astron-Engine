// Astron Engine — DirectX 11 / DirectX 12 HLSL Compute Particle Solver (cs_5_0)
// Simulação paralela em GPU de partículas 3D para efeitos espaciais e explosões.

struct GPU_Particle
{
    float3 position;
    float  life;
    float3 velocity;
    float  maxLife;
    float4 color;
};

RWStructuredBuffer<GPU_Particle> gParticles : register(u0);

cbuffer ParticleSimParams : register(b0)
{
    float3 uGravity;
    float  uDeltaTime;
    float3 uAttractorPos;
    float  uAttractorStrength;
    uint   uParticleCount;
    float  uDrag;
    float2 _reserved;
};

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint idx = dispatchThreadID.x;
    if (idx >= uParticleCount)
        return;

    GPU_Particle p = gParticles[idx];
    if (p.life <= 0.0f)
        return;

    float3 toAttractor = uAttractorPos - p.position;
    float  distSq      = max(dot(toAttractor, toAttractor), 0.25f);
    float3 pull        = normalize(toAttractor) * (uAttractorStrength / distSq);

    p.velocity += (uGravity + pull) * uDeltaTime;
    p.velocity *= saturate(1.0f - uDrag * uDeltaTime);
    p.position += p.velocity * uDeltaTime;
    p.life     -= uDeltaTime;

    float lifeRatio = saturate(p.life / max(p.maxLife, 0.001f));
    p.color.a = smoothstep(0.0f, 0.2f, lifeRatio);

    gParticles[idx] = p;
}
