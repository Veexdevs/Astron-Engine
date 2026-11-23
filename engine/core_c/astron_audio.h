#ifndef ASTRON_AUDIO_H
#define ASTRON_AUDIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AstronSfxPreset {
    ASTRON_SFX_JUMP      = 0,
    ASTRON_SFX_COIN      = 1,
    ASTRON_SFX_LASER     = 2,
    ASTRON_SFX_EXPLOSION = 3,
    ASTRON_SFX_POWERUP   = 4,
    ASTRON_SFX_AMBIENT   = 5
} AstronSfxPreset;

typedef struct AstronAudioParams {
    AstronSfxPreset preset;
    uint32_t sampleRate;   /* e.g. 44100 */
    float    durationSec;  /* 0.10 .. 4.00 */
    float    baseFreqHz;   /* 80.0 .. 1760.0 */
    float    volume;       /* 0.05 .. 1.00 */
} AstronAudioParams;

/* Synthesizes a 16-bit mono RIFF/WAVE file directly to disk and optionally
 * outputs normalized waveform peaks [0..1] for UI visualization. */
bool astron_audio_write_wav(
    const char* filePath,
    const AstronAudioParams* params,
    float* outPeaks,
    size_t peakBins
);

const char* astron_audio_preset_name(AstronSfxPreset preset);

#ifdef __cplusplus
}
#endif

#endif /* ASTRON_AUDIO_H */
