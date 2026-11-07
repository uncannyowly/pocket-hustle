/* Press Start 2P 8 x 8 bitmap font and text drawing.  Integers only. */
#ifndef PH_GFX_FONT_H
#define PH_GFX_FONT_H

#include <stdint.h>
#include "gfx/canvas.h"

enum { GLYPH_W = 8, GLYPH_H = 8, LINE_H = 10, TX_LINE_MAX = 48 };
enum { TX_LEFT, TX_CENTER, TX_RIGHT };

extern const uint8_t FONT8[95][8];	/* 0x20..0x7e, MSB = leftmost pixel */

int tx_width(const char *s, int scale);
/* Bytes outside 0x20..0x7e draw as '?'.  Every glyph cell that falls even partly outside the
 * canvas (not the clip) adds 1 to c->overflow.  Returns the width. */
int tx(Canvas *c, const char *s, int x, int y, uint32_t col, int scale, int align);
int tx_outline(Canvas *c, const char *s, int x, int y, uint32_t col, uint32_t outline, int scale, int align);
/* Greedy word wrap into out[][TX_LINE_MAX]; words longer than cols are split.
 * cols is clamped to 1..TX_LINE_MAX-1.  Returns the line count (at most max_lines). */
int tx_wrap(const char *s, int cols, char out[][TX_LINE_MAX], int max_lines);

#endif
