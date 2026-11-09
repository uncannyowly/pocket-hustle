/* Icons, lender portraits, the cop, the vault, the chase street and the seven city skylines,
 * all drawn in code from the approved mockup (docs/mockups/screens-round1.html).  Integers only. */
#ifndef PH_GFX_ART_H
#define PH_GFX_ART_H

#include <stdint.h>
#include "gfx/canvas.h"

typedef enum { ICON_HEART, ICON_COIN, ICON_BANK, ICON_FIN, ICON_BOLT, ICON_UP, ICON_DOWN, ICON_MIC,
	       ICON_BANG, ICON_TRAIN, ICON_SUN, ICON_CHART, ICON_CHECK, ICON_STAR, NUM_ICONS } IconId;

enum { ART_CITIES = 20 };	/* skylines and lender portraits, one per city (NUM_CITIES, checked by the tests) */

void art_icon(Canvas *c, IconId id, int x, int y, int scale, uint32_t tint);	/* tint used by UP/DOWN only */
void art_backdrop(Canvas *c, int city, int y0, int h, uint32_t t);	/* draws only inside rows y0..y0+h-1 */
void art_portrait(Canvas *c, int city, int x, int y);			/* 56 x 56, the city's lender */
void art_cop(Canvas *c, int x, int base, int scale, int boss);		/* 20 x 28 cells, feet on base */
void art_vault(Canvas *c, int cx, int cy, uint32_t t);
void art_chase_scene(Canvas *c, int y0, int cops_left, uint32_t t);	/* street, car, Buzzkill and posse */

/* Shared by skyline.c and sprites.c: fixed-point helpers. */
int art_rd(long long a, long long b);			/* a / b rounded to nearest, halves up (Math.round); b > 0 */
int art_fsin(long long num, long long den);		/* sin of num/den radians, -1024..1024 */
int art_fcos(long long num, long long den);
void art_bldg(Canvas *c, int x, int top, int w, int gy, uint32_t body, uint32_t win, int lit_pct, ArtRng *r);

#endif
