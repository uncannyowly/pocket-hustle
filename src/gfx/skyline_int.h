/* Shared by skyline.c (the first seven cities) and skyline2.c (the thirteen North American ones). */
#ifndef PH_GFX_SKYLINE_INT_H
#define PH_GFX_SKYLINE_INT_H

#include "gfx/art.h"

#define R(x, y, w, h, col) cv_rect(c, x, y, w, h, col)
#define P(x, y, col) cv_px(c, x, y, col)
#define L(x0, y0, x1, y1, col) cv_line(c, x0, y0, x1, y1, col)
#define D(x, y, r_, col) cv_disc(c, x, y, r_, col)
#define TRI(a, b, d, e, f, g, col) cv_tri(c, a, b, d, e, f, g, col)
#define RD art_rd
#define BLD(x, top, w, gy, body, win, lit) art_bldg(c, x, top, w, gy, body, win, lit, r)

void art_palm(Canvas *c, int x, int base, int ht, uint32_t trunk, uint32_t leaf);
void art_pine(Canvas *c, int x, int base, int ht, uint32_t c1, uint32_t c2);
/* idx 0..12: Dallas, San Francisco, Los Angeles, Seattle, Cincinnati, Dayton, Destin, Tijuana,
 * Cozumel, Los Cabos, Sinaloa, Chicago, Detroit.  Same contract as the original seven: rows y0..gy-1 only, t = 0 is a still. */
void art_backdrop2(Canvas *c, int idx, int y0, int gy, ArtRng *r, uint32_t t);

#endif
