/* 2:1 sample rate reduction for the Mini's audio device. Pure. */
#ifndef PH_HALVE_H
#define PH_HALVE_H

#include <stdint.h>

/* Writes out_frames stereo frames to dst, each the per-channel average of the next two stereo
 * frames of src (2 * out_frames frames). Rounds half up; the result always fits int16. */
void audio_halve(int16_t *dst, const int16_t *src, int out_frames);

#endif
