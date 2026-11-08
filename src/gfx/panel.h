/* Putting the 640 x 480 picture on a Mini panel: centered, turned 180 degrees. Pure. */
#ifndef PH_PANEL_H
#define PH_PANEL_H

#include <stdint.h>

/* Writes the panel image to dst (dw x dh pixels, pitch dpitch in pixels): src (sw x sh, pitch
 * spitch) centered, the whole panel then rotated 180 degrees, every pixel opaque, the margins
 * black. A source larger than the panel is cropped around its center. */
void panel_blit_rot180(uint32_t *dst, int dpitch, int dw, int dh,
		       const uint32_t *src, int sw, int sh, int spitch);

#endif
