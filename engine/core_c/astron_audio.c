#include "astron_audio.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void write_u16_le(uint8_t* dst, uint16_t v) {
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void write_u32_le(uint8_t* dst, uint32_t v) {
    dst[0] = (uint8_t)(v & 0xFFu);
    dst[1] = (uint8_t)((v >> 8) & 0xFFu);
    dst[2] = (uint8_t)((v >> 16) & 0xFFu);
    dst[3] = (uint8_t)((v >> 24) & 0xFFu);
}

static float lfsr_noise(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return ((float)(x & 0xFFFFu) / 32767.5f) - 1.0f;
}

const char* astron_audio_preset_name(AstronSfxPreset preset) {
    switch (preset) {
        case ASTRON_SFX_JUMP:      return "jump.wav";
        case ASTRON_SFX_COIN:      return "coin.wav";
        case ASTRON_SFX_LASER:     return "laser.wav";
        case ASTRON_SFX_EXPLOSION: return "explosion.wav";
        case ASTRON_SFX_POWERUP:   return "powerup.wav";
        case ASTRON_SFX_AMBIENT:   return "space_ambient.wav";
        default:                   return "sfx.wav";
    }
}

bool astron_audio_write_wav(
    const char* filePath,
    const AstronAudioParams* params,
    float* outPeaks,
    size_t peakBins
) {
    if (!filePath || !params) return false;

    uint32_t sr = params->sampleRate >= 8000u ? params->sampleRate : 44100u;
    float dur = params->durationSec;
    if (dur < 0.08f) dur = 0.08f;
    if (dur > 6.0f)  dur = 6.0f;

    float baseHz = params->baseFreqHz > 30.0f ? params->baseFreqHz : 440.0f;
    float vol = params->volume;
    if (vol < 0.01f) vol = 0.01f;
    if (vol > 1.0f)  vol = 1.0f;

    uint32_t totalSamples = (uint32_t)(dur * (float)sr);
    if (totalSamples < 64u) totalSamples = 64u;

    int16_t* pcm = (int16_t*)malloc((size_t)totalSamples * sizeof(int16_t));
    if (!pcm) return false;

    if (outPeaks && peakBins > 0) {
        memset(outPeaks, 0, peakBins * sizeof(float));
    }

    double phase1 = 0.0;
    double phase2 = 0.0;
    double phase3 = 0.0;
    float lpState = 0.0f;
    uint32_t rng = 0xA5720E11u;

    for (uint32_t i = 0; i < totalSamples; ++i) {
        float t = (float)i / (float)sr;
        float u = (float)i / (float)totalSamples; /* 0..1 */
        float env = 1.0f - u;
        float s = 0.0f;

        switch (params->preset) {
            case ASTRON_SFX_JUMP: {
                float freq = baseHz * (1.0f + 1.35f * u * u);
                phase1 += (2.0 * M_PI * (double)freq) / (double)sr;
                float sq = sin(phase1) >= 0.0 ? 0.65f : -0.65f;
                s = (0.65f * (float)sin(phase1) + 0.35f * sq) * powf(1.0f - u, 0.7f);
                break;
            }
            case ASTRON_SFX_COIN: {
                float freq = (u < 0.28f) ? baseHz : (baseHz * 1.3348f);
                float localEnv = (u < 0.28f) ? (1.0f - u * 1.4f) : powf(1.0f - (u - 0.28f) / 0.72f, 1.5f);
                phase1 += (2.0 * M_PI * (double)freq) / (double)sr;
                phase2 += (4.0 * M_PI * (double)freq) / (double)sr;
                s = (0.78f * (float)sin(phase1) + 0.22f * (float)sin(phase2)) * localEnv;
                break;
            }
            case ASTRON_SFX_LASER: {
                float freq = baseHz * 2.2f * expf(-4.2f * u) + 70.0f;
                phase1 += (2.0 * M_PI * (double)freq) / (double)sr;
                float fm = (float)sin(t * 95.0f) * 0.4f;
                s = (float)sin(phase1 + (double)fm) * powf(env, 0.85f);
                break;
            }
            case ASTRON_SFX_EXPLOSION: {
                float raw = lfsr_noise(&rng);
                float cutoff = 0.04f + 0.32f * powf(1.0f - u, 2.2f);
                lpState += cutoff * (raw - lpState);
                phase1 += (2.0 * M_PI * (double)(65.0f * (1.0f - 0.6f * u))) / (double)sr;
                s = (0.80f * lpState * 2.2f + 0.25f * (float)sin(phase1)) * powf(1.0f - u, 1.35f);
                break;
            }
            case ASTRON_SFX_POWERUP: {
                float step = floorf(u * 5.0f) / 5.0f;
                float mult = 1.0f + step * 0.9f;
                float freq = baseHz * mult;
                phase1 += (2.0 * M_PI * (double)freq) / (double)sr;
                phase2 += (2.0 * M_PI * (double)(freq * 1.25f)) / (double)sr;
                s = 0.5f * ((float)sin(phase1) + (float)sin(phase2)) * (1.0f - 0.55f * u);
                break;
            }
            case ASTRON_SFX_AMBIENT:
            default: {
                float lfo = 1.0f + 0.015f * sinf(2.0f * (float)M_PI * 1.8f * t);
                phase1 += (2.0 * M_PI * (double)(baseHz * 0.5f * lfo)) / (double)sr;
                phase2 += (2.0 * M_PI * (double)(baseHz * 0.749f)) / (double)sr;
                phase3 += (2.0 * M_PI * (double)(baseHz * 1.002f)) / (double)sr;
                float fade = (u < 0.12f) ? (u / 0.12f) : ((u > 0.85f) ? ((1.0f - u) / 0.15f) : 1.0f);
                s = 0.36f * ((float)sin(phase1) + 0.7f * (float)sin(phase2) + 0.5f * (float)sin(phase3)) * fade;
                break;
            }
        }

        s *= vol;
        if (s > 1.0f)  s = 1.0f;
        if (s < -1.0f) s = -1.0f;

        pcm[i] = (int16_t)(s * 32760.0f);

        if (outPeaks && peakBins > 0) {
            size_t bin = ((size_t)i * peakBins) / (size_t)totalSamples;
            if (bin >= peakBins) bin = peakBins - 1;
            float a = fabsf(s);
            if (a > outPeaks[bin]) outPeaks[bin] = a;
        }
    }

    uint32_t dataBytes = totalSamples * 2u;
    uint8_t hdr[44];
    memcpy(hdr + 0, "RIFF", 4);
    write_u32_le(hdr + 4, 36u + dataBytes);
    memcpy(hdr + 8, "WAVEfmt ", 8);
    write_u32_le(hdr + 16, 16u);
    write_u16_le(hdr + 20, 1u);       /* PCM */
    write_u16_le(hdr + 22, 1u);       /* Mono */
    write_u32_le(hdr + 24, sr);
    write_u32_le(hdr + 28, sr * 2u);  /* Byte rate */
    write_u16_le(hdr + 32, 2u);       /* Block align */
    write_u16_le(hdr + 34, 16u);      /* Bits per sample */
    memcpy(hdr + 36, "data", 4);
    write_u32_le(hdr + 40, dataBytes);

    FILE* fp = fopen(filePath, "wb");
    if (!fp) {
        free(pcm);
        return false;
    }

    bool ok = (fwrite(hdr, 1, 44, fp) == 44) &&
              (fwrite(pcm, sizeof(int16_t), (size_t)totalSamples, fp) == (size_t)totalSamples);
    fclose(fp);
    free(pcm);
    return ok;
}
