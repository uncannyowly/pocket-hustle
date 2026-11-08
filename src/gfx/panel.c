#include "gfx/panel.h"
#include <stddef.h>

void panel_blit_rot180(uint32_t *dst, int dpitch, int dw, int dh,
		       const uint32_t *src, int sw, int sh, int spitch)
{
	int ox = (dw - sw) / 2, oy = (dh - sh) / 2;	/* negative when the source is larger: crop */
	int x, y;

	for (y = 0; y < dh; y++) {
		/* panel (x, y) shows the unrotated panel at (dw-1-x, dh-1-y) */
		int ly = dh - 1 - y - oy;
		uint32_t *row = dst + (size_t)y * dpitch;

		for (x = 0; x < dw; x++) {
			int lx = dw - 1 - x - ox;

			if (lx >= 0 && lx < sw && ly >= 0 && ly < sh)
				row[x] = src[(size_t)ly * spitch + lx] | 0xFF000000u;
			else
				row[x] = 0xFF000000u;
		}
	}
}
