#ifndef VML_AUDIO_ANALYSIS_H
#define VML_AUDIO_ANALYSIS_H
#include <stdint.h>
typedef struct { float bass, mids, highs, rms, transient; } AudioLevels;
void audio_reset(void);
void audio_push(const int16_t *mono, int count, int rate);
AudioLevels audio_analyze(float gain);
#endif
