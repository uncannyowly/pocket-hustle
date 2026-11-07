/* 320 x 240 XRGB8888 canvas and integer drawing primitives.  No SDL, integers only:
 * every screen renders pixel-identical on the PC and on the Miyoo. */
#ifndef PH_GFX_CANVAS_H
#define PH_GFX_CANVAS_H

#include <stdint.h>

enum { CV_W = 320, CV_H = 240 };

typedef struct {
	uint32_t px[CV_W * CV_H];
	int cx0, cy0, cx1, cy1;		/* clip window, half-open */
	uint32_t overflow;		/* spare counter for layers above (text overflow); cv_clear zeroes it */
} Canvas;

void cv_clear(Canvas *c, uint32_t col);			/* also resets clip and overflow */
void cv_clip(Canvas *c, int x0, int y0, int x1, int y1);	/* half-open, intersected with the canvas */
void cv_noclip(Canvas *c);
void cv_px(Canvas *c, int x, int y, uint32_t col);
void cv_rect(Canvas *c, int x, int y, int w, int h, uint32_t col);
void cv_box(Canvas *c, int x, int y, int w, int h, uint32_t col);	/* 1-pixel outline */
void cv_line(Canvas *c, int x0, int y0, int x1, int y1, uint32_t col);
void cv_disc(Canvas *c, int cx, int cy, int r, uint32_t col);
void cv_ring(Canvas *c, int cx, int cy, int r, uint32_t col);
void cv_tri(Canvas *c, int ax, int ay, int bx, int by, int qx, int qy, uint32_t col);
void cv_grad(Canvas *c, int x, int y, int w, int h, const uint32_t *cols, int n);	/* vertical, 4x4 Bayer */
void cv_checker(Canvas *c, int x, int y, int w, int h, uint32_t col);	/* every other pixel */
void cv_dim(Canvas *c);					/* each channel halved */
void cv_scale2x(const Canvas *c, uint32_t *dst, int dst_pitch);	/* to 640 x 480; pitch in pixels */

int isin(int a);	/* a: 256 steps a turn, any int; result -1024..1024 */
int icos(int a);
int isqrt(int v);	/* floor, v >= 0 */

typedef struct { uint32_t s; } ArtRng;	/* mulberry32, for stable window patterns */
void art_seed(ArtRng *r, uint32_t seed);
uint32_t art_next(ArtRng *r);
int art_below(ArtRng *r, int n);

#endif
