#include <stddef.h>
#include "sfx/halve.h"

void audio_halve(int16_t *dst, const int16_t *src, int out_frames)
{
	int i;

	for (i = 0; i < out_frames; i++) {
		const int16_t *a = src + (size_t)i * 4;	/* L0 R0 L1 R1 */

		dst[i * 2] = (int16_t)(((int)a[0] + (int)a[2] + 1) >> 1);
		dst[i * 2 + 1] = (int16_t)(((int)a[1] + (int)a[3] + 1) >> 1);
	}
}
