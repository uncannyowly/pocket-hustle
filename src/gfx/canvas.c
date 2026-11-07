#include "gfx/canvas.h"

#include <string.h>

void cv_clear(Canvas *c, uint32_t col)
{
	int i;

	for (i = 0; i < CV_W * CV_H; i++)
		c->px[i] = col;
	cv_noclip(c);
	c->overflow = 0;
}

void cv_clip(Canvas *c, int x0, int y0, int x1, int y1)
{
	c->cx0 = x0 > 0 ? x0 : 0;
	c->cy0 = y0 > 0 ? y0 : 0;
	c->cx1 = x1 < CV_W ? x1 : CV_W;
	c->cy1 = y1 < CV_H ? y1 : CV_H;
}

void cv_noclip(Canvas *c)
{
	c->cx0 = 0;
	c->cy0 = 0;
	c->cx1 = CV_W;
	c->cy1 = CV_H;
}

void cv_px(Canvas *c, int x, int y, uint32_t col)
{
	if (x >= c->cx0 && y >= c->cy0 && x < c->cx1 && y < c->cy1)
		c->px[y * CV_W + x] = col;
}

void cv_rect(Canvas *c, int x, int y, int w, int h, uint32_t col)
{
	long long xe = (long long)x + w, ye = (long long)y + h;
	int x0 = x > c->cx0 ? x : c->cx0;
	int y0 = y > c->cy0 ? y : c->cy0;
	int x1 = xe < c->cx1 ? (xe > x0 ? (int)xe : x0) : c->cx1;
	int y1 = ye < c->cy1 ? (ye > y0 ? (int)ye : y0) : c->cy1;
	int i, j;

	if (x1 <= x0)
		return;
	for (j = y0; j < y1; j++)
		for (i = x0; i < x1; i++)
			c->px[j * CV_W + i] = col;
}

void cv_box(Canvas *c, int x, int y, int w, int h, uint32_t col)
{
	cv_rect(c, x, y, w, 1, col);
	cv_rect(c, x, y + h - 1, w, 1, col);
	cv_rect(c, x, y, 1, h, col);
	cv_rect(c, x + w - 1, y, 1, h, col);
}

/* Bresenham, with the mockup's 2000-step bound against wild endpoints. */
void cv_line(Canvas *c, int x0, int y0, int x1, int y1, uint32_t col)
{
	int dx = x1 > x0 ? x1 - x0 : x0 - x1;
	int dy = y1 > y0 ? y0 - y1 : y1 - y0;	/* minus the absolute value */
	int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
	int e = dx + dy, e2, n;

	for (n = 0; n < 2000; n++) {
		cv_px(c, x0, y0, col);
		if (x0 == x1 && y0 == y1)
			break;
		e2 = 2 * e;
		if (e2 >= dy) {
			e += dy;
			x0 += sx;
		}
		if (e2 <= dx) {
			e += dx;
			y0 += sy;
		}
	}
}

void cv_disc(Canvas *c, int cx, int cy, int r, uint32_t col)
{
	int y, w;

	for (y = -r; y <= r; y++) {
		/* floor(sqrt(v) + 0.5) in integers */
		w = (isqrt(4 * (r * r - y * y)) + 1) / 2;
		cv_rect(c, cx - w, cy + y, 2 * w + 1, 1, col);
	}
}

void cv_ring(Canvas *c, int cx, int cy, int r, uint32_t col)
{
	int a, t;

	for (a = 0; a < 72; a++) {
		t = a * 256 / 72;
		cv_px(c, cx + ((icos(t) * r + 512) >> 10),
		      cy + ((isin(t) * r + 512) >> 10), col);
	}
}

static int floor_div(int n, int d)	/* d > 0 */
{
	int q = n / d;

	return (n % d != 0 && n < 0) ? q - 1 : q;
}

/* Scanline fill sampled at y + 0.5, so every y and x here is doubled. */
void cv_tri(Canvas *c, int ax, int ay, int bx, int by, int qx, int qy, uint32_t col)
{
	int px_[3], py_[3];
	int ymin = ay, ymax = ay, y, i;

	px_[0] = ax; py_[0] = ay;
	px_[1] = bx; py_[1] = by;
	px_[2] = qx; py_[2] = qy;
	for (i = 1; i < 3; i++) {
		if (py_[i] < ymin) ymin = py_[i];
		if (py_[i] > ymax) ymax = py_[i];
	}
	for (y = ymin; y <= ymax; y++) {
		int yy = 2 * y + 1, lo = 0, hi = 0, have = 0;

		for (i = 0; i < 3; i++) {
			int x0 = px_[i], y0 = py_[i];
			int x1 = px_[(i + 1) % 3], y1 = py_[(i + 1) % 3];
			int dy, x;

			if (!((yy >= 2 * y0 && yy < 2 * y1) || (yy >= 2 * y1 && yy < 2 * y0)))
				continue;
			if (y1 < y0) {
				int t;
				t = x0; x0 = x1; x1 = t;
				t = y0; y0 = y1; y1 = t;
			}
			dy = y1 - y0;
			/* round(x0 + (yy/2 - y0) * (x1 - x0) / dy) */
			x = floor_div(2 * x0 * dy + (yy - 2 * y0) * (x1 - x0) + dy, 2 * dy);
			if (!have || x < lo) lo = x;
			if (!have || x > hi) hi = x;
			have = 1;
		}
		if (have)
			cv_rect(c, lo, y, hi - lo + 1, 1, col);
	}
}

static const uint8_t BAY[16] = { 0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5 };

void cv_grad(Canvas *c, int x, int y, int w, int h, const uint32_t *cols, int n)
{
	int seg = n - 1, den = h > 1 ? h - 1 : 1;
	int j, k;

	if (n < 1)
		return;
	for (j = 0; j < h; j++) {
		int num = j * seg, i = num / den, f, Y = y + j;

		if (i > seg - 1)
			i = seg - 1;
		if (i < 0)
			i = 0;
		f = num - i * den;	/* fraction is f / den, threshold (2b + 1) / 32 */
		for (k = 0; k < w; k++) {
			int X = x + k;
			int th = 2 * BAY[(Y & 3) * 4 + (X & 3)] + 1;

			if (seg > 0 && f * 32 > th * den)
				cv_px(c, X, Y, cols[i + 1]);
			else
				cv_px(c, X, Y, cols[i]);
		}
	}
}

void cv_checker(Canvas *c, int x, int y, int w, int h, uint32_t col)
{
	int i, j;

	for (j = 0; j < h; j++)
		for (i = 0; i < w; i++)
			if (((x + i + y + j) & 1) == 0)
				cv_px(c, x + i, y + j, col);
}

void cv_dim(Canvas *c)
{
	int i;

	for (i = 0; i < CV_W * CV_H; i++)
		c->px[i] = (c->px[i] >> 1) & 0x7f7f7fu;
}

void cv_scale2x(const Canvas *c, uint32_t *dst, int dst_pitch)
{
	int x, y;

	for (y = 0; y < CV_H; y++) {
		uint32_t *r0 = dst + (size_t)(2 * y) * dst_pitch;

		for (x = 0; x < CV_W; x++) {
			r0[2 * x] = c->px[y * CV_W + x];
			r0[2 * x + 1] = c->px[y * CV_W + x];
		}
		memcpy(r0 + dst_pitch, r0, CV_W * 2 * sizeof *r0);
	}
}

/* round(1024 * sin(i * pi / 128)), i = 0..64: a quarter turn of 256 steps */
static const int16_t SINQ[65] = {
	0, 25, 50, 75, 100, 125, 150, 175, 200, 224, 249, 273, 297,
	321, 345, 369, 392, 415, 438, 460, 483, 505, 526, 548, 569, 590,
	610, 630, 650, 669, 688, 706, 724, 742, 759, 775, 792, 807, 822,
	837, 851, 865, 878, 891, 903, 915, 926, 936, 946, 955, 964, 972,
	980, 987, 993, 999, 1004, 1009, 1013, 1016, 1019, 1021, 1023, 1024, 1024,
};

int isin(int a)
{
	int i = a & 63;

	switch ((a >> 6) & 3) {
	case 0: return SINQ[i];
	case 1: return SINQ[64 - i];
	case 2: return -SINQ[i];
	default: return -SINQ[64 - i];
	}
}

int icos(int a)
{
	return isin(a + 64);
}

int isqrt(int v)
{
	unsigned r = 0, bit = 1u << 30, n = (unsigned)v;

	while (bit > n)
		bit >>= 2;
	while (bit) {
		if (n >= r + bit) {
			n -= r + bit;
			r = (r >> 1) + bit;
		} else {
			r >>= 1;
		}
		bit >>= 2;
	}
	return (int)r;
}

void art_seed(ArtRng *r, uint32_t seed)
{
	r->s = seed;
}

uint32_t art_next(ArtRng *r)
{
	uint32_t t;

	r->s += 0x6D2B79F5u;
	t = r->s;
	t = (t ^ (t >> 15)) * (t | 1u);
	t ^= t + (t ^ (t >> 7)) * (t | 61u);
	return t ^ (t >> 14);
}

int art_below(ArtRng *r, int n)
{
	return (int)(((uint64_t)art_next(r) * (uint32_t)n) >> 32);
}
