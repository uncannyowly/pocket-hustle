#include "gfx/font.h"

#include <string.h>

static const uint8_t *glyph(unsigned char ch)
{
	if (ch < 0x20 || ch > 0x7e)
		ch = '?';
	return FONT8[ch - 0x20];
}

int tx_width(const char *s, int scale)
{
	return (int)strlen(s) * GLYPH_W * scale;
}

int tx(Canvas *c, const char *s, int x, int y, uint32_t col, int scale, int align)
{
	int w = tx_width(s, scale), n, i, j, cell = GLYPH_W * scale;

	if (scale < 1)
		return w;
	if (align == TX_CENTER)
		x -= w >> 1;
	else if (align == TX_RIGHT)
		x -= w;
	for (n = 0; s[n]; n++) {
		const uint8_t *g = glyph((unsigned char)s[n]);
		int ox = x + n * cell;

		if (ox < 0 || y < 0 || ox + cell > CV_W || y + GLYPH_H * scale > CV_H)
			c->overflow++;
		for (j = 0; j < GLYPH_H; j++)
			for (i = 0; i < GLYPH_W; i++)
				if (g[j] & (0x80 >> i)) {
					if (scale == 1)
						cv_px(c, ox + i, y + j, col);
					else
						cv_rect(c, ox + i * scale, y + j * scale, scale, scale, col);
				}
	}
	return w;
}

int tx_outline(Canvas *c, const char *s, int x, int y, uint32_t col, uint32_t outline, int scale, int align)
{
	uint32_t saved = c->overflow;	/* the neighbor passes must not count; the main pass does */
	int dx, dy;

	for (dy = -1; dy <= 1; dy++)
		for (dx = -1; dx <= 1; dx++)
			if (dx || dy)
				tx(c, s, x + dx * scale, y + dy * scale, outline, scale, align);
	c->overflow = saved;
	return tx(c, s, x, y, col, scale, align);
}

int tx_wrap(const char *s, int cols, char out[][TX_LINE_MAX], int max_lines)
{
	char cur[TX_LINE_MAX];
	int len = 0, lines = 0;

	if (cols < 1)
		cols = 1;
	if (cols > TX_LINE_MAX - 1)
		cols = TX_LINE_MAX - 1;
	cur[0] = 0;
#define FLUSH() do { \
	if (len > 0) { \
		if (lines < max_lines) { \
			memcpy(out[lines], cur, (size_t)len); \
			out[lines][len] = 0; \
			lines++; \
		} \
		len = 0; \
		cur[0] = 0; \
	} \
} while (0)
	while (*s && lines < max_lines) {
		const char *w;
		int wl = 0;

		while (*s == ' ')
			s++;
		if (!*s)
			break;
		w = s;
		while (s[wl] && s[wl] != ' ')
			wl++;
		s += wl;
		while (wl > cols && lines < max_lines) {
			FLUSH();
			if (lines >= max_lines)
				break;
			memcpy(out[lines], w, (size_t)cols);
			out[lines][cols] = 0;
			lines++;
			w += cols;
			wl -= cols;
		}
		if (lines >= max_lines)
			break;
		if (len > 0 && len + 1 + wl > cols)
			FLUSH();
		if (lines >= max_lines)
			break;
		if (len > 0)
			cur[len++] = ' ';
		memcpy(cur + len, w, (size_t)wl);
		len += wl;
		cur[len] = 0;
	}
	FLUSH();
#undef FLUSH
	return lines;
}
