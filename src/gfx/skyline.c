/* The seven city backdrops, ported from the mockup's BD array.  Fractions become fixed point
 * rounded to the nearest pixel; radians become 256-step angles through art_fsin/art_fcos. */
#include "gfx/art.h"

#define R(x, y, w, h, col) cv_rect(c, x, y, w, h, col)
#define P(x, y, col) cv_px(c, x, y, col)
#define L(x0, y0, x1, y1, col) cv_line(c, x0, y0, x1, y1, col)
#define D(x, y, r_, col) cv_disc(c, x, y, r_, col)
#define TRI(a, b, d, e, f, g, col) cv_tri(c, a, b, d, e, f, g, col)
#define RD art_rd

int art_rd(long long a, long long b)
{
	long long n = 2 * a + b, d = 2 * b, q = n / d;

	if (n % d != 0 && n < 0)
		q--;
	return (int)q;
}

/* 1 radian = 256 / (2 pi) = 40.742 steps */
int art_fsin(long long num, long long den)
{
	return isin(art_rd(num * 10430, den * 256));
}

int art_fcos(long long num, long long den)
{
	return icos(art_rd(num * 10430, den * 256));
}

void art_bldg(Canvas *c, int x, int top, int w, int gy, uint32_t body, uint32_t win, int lit_pct, ArtRng *r)
{
	int xx, yy;

	R(x, top, w, gy - top, body);
	for (yy = top + 2; yy < gy - 1; yy += 3)
		for (xx = x + 1; xx < x + w - 1; xx += 3)
			if (art_below(r, 100) < lit_pct)
				P(xx, yy, win);
}

#define BLD(x, top, w, gy, body, win, lit) art_bldg(c, x, top, w, gy, body, win, lit, r)

static void palm(Canvas *c, int x, int base, int ht, uint32_t trunk, uint32_t leaf)
{
	static const int lf[6][2] = {{-9, 3}, {-7, -2}, {-2, -4}, {4, -3}, {8, 1}, {9, 4}};
	int k, tx = x, ty = base;

	for (k = 0; k < ht; k++) {
		tx = x + RD(2LL * art_fsin(k, 7), 1024);
		ty = base - k;
		P(tx, ty, trunk);
		P(tx + 1, ty, trunk);
	}
	for (k = 0; k < 6; k++) {
		L(tx, ty, tx + lf[k][0], ty + lf[k][1], leaf);
		L(tx, ty + 1, tx + lf[k][0], ty + lf[k][1] + 1, leaf);
	}
}

static void pine(Canvas *c, int x, int base, int ht, uint32_t c1, uint32_t c2)
{
	int k;

	for (k = 0; k < 3; k++) {
		long long a100 = (long long)(base - ht) * 100 + (long long)k * ht * 28;
		int a = RD(a100, 100), b = RD(a100 + (long long)ht * 42, 100);
		int hw = RD((long long)ht * 42 * (45 + 30 * k), 10000);

		TRI(x - hw, b, x + hw, b, x, a, k % 2 ? c2 : c1);
	}
	R(x, base - 2, 1, 2, 0x3a2a1eu);
}

static void turbine(Canvas *c, int x, int base, int ht, uint32_t t, uint32_t col)
{
	int k;

	L(x, base, x, base - ht, col);
	for (k = 0; k < 3; k++) {
		long long num = (long long)t * 120 + k * 2094;	/* t * 0.12 + k * 2.094 radians */
		int ex = RD((long long)x * 102400 + 45LL * ht * art_fcos(num, 1000), 102400);
		int ey = RD((long long)(base - ht) * 102400 + 45LL * ht * art_fsin(num, 1000), 102400);

		L(x, base - ht, ex, ey, col);
	}
	P(x, base - ht, 0xffffffu);
}

static void okc(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x1b1238u, 0x34205au, 0x683768u, 0xc4614au};
	long long T = t;
	int x, k, dx = 34, pj = 62;
	uint32_t dk = 0x130b24u;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	for (x = 0; x < CV_W; x++) {
		int s1 = art_fsin(5LL * x + 3 * T, 55), s2 = art_fsin(10LL * x, 53);
		int b = y0 + 7 + RD(3LL * s1 + 2LL * s2, 1024);

		R(x, y0, 1, b - y0, 0x140d2cu);
		if (((x + b) & 1) == 0)
			P(x, b, 0x140d2cu);
	}
	if (t % 110 < 3 && t > 0) {
		int lx = 210 + (int)(((T / 110) * 37) % 90), ly = y0 + 9;
		ArtRng lr;

		art_seed(&lr, t);
		while (ly < gy - 22) {
			int nx = lx + art_below(&lr, 9) - 4, ny = ly + 4 + art_below(&lr, 4);

			L(lx, ly, nx, ny, 0xfff6c8u);
			lx = nx;
			ly = ny;
		}
	}
	R(0, gy - 10, CV_W, 6, 0x2c1a3eu);
	L(dx - 8, gy - 4, dx, gy - 38, dk);
	L(dx + 8, gy - 4, dx, gy - 38, dk);
	for (k = 0; k < 5; k++) {
		int ya2 = 2 * (gy - 4) - 13 * k, yb2 = ya2 - 13;	/* half pixels */
		int ha2 = ya2 - 2 * (gy - 38), hb2 = yb2 - 2 * (gy - 38);

		L(RD(dx * 68LL - 8LL * ha2, 68), RD(ya2, 2), RD(dx * 68LL + 8LL * hb2, 68), RD(yb2, 2), dk);
		L(RD(dx * 68LL + 8LL * ha2, 68), RD(ya2, 2), RD(dx * 68LL - 8LL * hb2, 68), RD(yb2, 2), dk);
	}
	R(dx - 1, gy - 41, 3, 3, dk);
	{
		int s = art_fsin(T, 6);			/* a = sin(t / 6) * 0.35 radians */
		int bx = art_fcos(35LL * s, 102400) * 12, by = art_fsin(35LL * s, 102400) * 12;	/* q10 */

		TRI(pj - 4, gy - 4, pj + 4, gy - 4, pj, gy - 13, dk);
		L(RD(pj * 1024LL - bx, 1024), RD((gy - 13) * 1024LL - by, 1024),
		  RD(pj * 1024LL + bx, 1024), RD((gy - 13) * 1024LL + by, 1024), dk);
		L(RD(pj * 1024LL - bx, 1024), RD((gy - 12) * 1024LL - by, 1024),
		  RD(pj * 1024LL + bx, 1024), RD((gy - 12) * 1024LL + by, 1024), dk);
		R(RD(pj * 1024LL + bx - 1024, 1024), RD((gy - 15) * 1024LL + by, 1024), 3, 6, dk);
	}
	BLD(92, gy - 16, 14, gy, 0x22173fu, 0xf2c46au, 30);
	BLD(108, gy - 25, 11, gy, 0x22173fu, 0xf2c46au, 30);
	BLD(121, gy - 31, 13, gy, 0x22173fu, 0xf2c46au, 30);
	R(138, gy - 44, 16, 44, 0x2f437eu);
	D(146, gy - 44, 8, 0x2f437eu);
	for (k = gy - 40; k < gy - 4; k += 4)
		R(139, k, 14, 1, 0x3d5594u);
	R(141, gy - 50, 1, 46, 0x6c86cfu);
	R(149, gy - 48, 1, 44, 0x4e67abu);
	BLD(157, gy - 35, 12, gy, 0x22173fu, 0xf2c46au, 30);
	BLD(171, gy - 23, 16, gy, 0x22173fu, 0xf2c46au, 30);
	BLD(189, gy - 29, 10, gy, 0x22173fu, 0xf2c46au, 30);
	BLD(201, gy - 17, 18, gy, 0x22173fu, 0xf2c46au, 30);
	BLD(221, gy - 12, 22, gy, 0x22173fu, 0xf2c46au, 25);
	turbine(c, 264, gy - 4, 36, t, 0xcfc8e4u);
	turbine(c, 298, gy - 4, 27, t + 9, 0xa9a0c8u);
	R(0, gy - 4, CV_W, 4, 0x120c22u);
	for (x = 4; x < CV_W; x += 16)
		P(x, gy - 3, 0xffcf6au);
}

static void hou(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0xe9813au, 0xf39a45u, 0xf7bb5eu, 0xfbdc93u};
	static const int far[8][3] = {{96, 22, 12}, {110, 30, 10}, {122, 38, 14}, {138, 26, 10},
				      {150, 42, 12}, {164, 34, 14}, {180, 28, 10}, {192, 20, 16}};
	long long T = t;
	int i, k, xx, yy, rk = 22, fl = (int)((t >> 1) & 1);
	uint32_t nc = 0x7a3b28u, nw = 0xffd58au, lt = 0x6b3a2au, rf = 0x6d4232u;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(64, gy - 30, 12, 0xfff0b0u);
	for (i = 0; i < 8; i++)
		R(far[i][0], gy - far[i][1], far[i][2], far[i][1], 0xd7895cu);
	BLD(100, gy - 24, 12, gy, nc, nw, 25);
	BLD(126, gy - 30, 10, gy, nc, nw, 25);
	R(144, gy - 36, 16, 36, 0x5e2c1fu);
	TRI(144, gy - 36, 152, gy - 36, 148, gy - 42, 0x5e2c1fu);
	TRI(152, gy - 36, 160, gy - 36, 156, gy - 42, 0x5e2c1fu);
	TRI(147, gy - 40, 157, gy - 40, 152, gy - 49, 0x5e2c1fu);
	for (yy = gy - 32; yy < gy - 6; yy += 3)
		for (xx = 146; xx < 158; xx += 3)
			if (art_below(r, 100) < 30)
				P(xx, yy, nw);
	BLD(166, gy - 28, 14, gy, nc, nw, 25);
	BLD(184, gy - 20, 12, gy, nc, nw, 25);
	R(rk, gy - 34, 6, 30, 0xf4efe6u);
	TRI(rk, gy - 34, rk + 6, gy - 34, rk + 3, gy - 43, 0xd9483au);
	TRI(rk - 3, gy - 4, rk, gy - 4, rk, gy - 12, 0xd9483au);
	TRI(rk + 6, gy - 4, rk + 9, gy - 4, rk + 6, gy - 12, 0xd9483au);
	R(rk + 1, gy - 22, 4, 1, 0x2a2a3au);
	R(rk + 12, gy - 38, 2, 34, lt);
	R(rk + 18, gy - 38, 2, 34, lt);
	for (yy = gy - 36; yy < gy - 6; yy += 5)
		L(rk + 12, yy, rk + 19, yy + 5, lt);
	R(rk + 6, gy - 30, 6, 1, lt);
	D(262, gy - 8, 6, rf);
	D(278, gy - 7, 5, rf);
	R(288, gy - 30, 3, 26, rf);
	R(298, gy - 24, 3, 20, rf);
	R(306, gy - 18, 6, 14, rf);
	P(289, gy - 31, fl ? 0xfff3a0u : 0xff8a2au);
	P(289, gy - 32, 0xff8a2au);
	P(290, gy - 33 - fl, 0xffd04au);
	R(0, gy - 13, CV_W, 3, 0x4a2a20u);
	R(0, gy - 14, CV_W, 1, 0x8a5a40u);
	for (xx = 6; xx < CV_W; xx += 22)
		R(xx, gy - 10, 3, 6, 0x4a2a20u);
	for (k = 0; k < 6; k++) {
		int cx0 = (int)((k * 57 + T * 2) % (CV_W + 20)) - 10;

		R(cx0, gy - 16, 4, 2, k % 2 ? 0xffe08au : 0xff5f6du);
	}
	R(0, gy - 4, CV_W, 4, 0x3e2219u);
}

static void sjc(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x3a8bd6u, 0x5ea9e8u, 0x94cef4u, 0xcdebfau};
	static const int bl[8][3] = {{60, 14, 18}, {82, 22, 14}, {100, 18, 20}, {124, 28, 16},
				     {144, 16, 22}, {170, 24, 14}, {188, 12, 26}, {218, 20, 16}};
	long long T = t;
	int x, i, xx, yy, k, tx2, tx;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	for (x = 0; x < CV_W; x++) {
		int s1 = art_fsin(100LL * x + 5330, 4100), s2 = art_fsin(x, 13);
		int h = gy - 16 - RD(9216LL + 9LL * s1 + 6LL * s2, 2048);

		R(x, h, 1, gy - h, 0xc79a45u);
		if (x % 3 == 0)
			P(x, h + 2, 0xa97c33u);
	}
	for (i = 0; i < 8; i++) {
		int bx = bl[i][0], h = bl[i][1], w = bl[i][2];

		R(bx, gy - h, w, h, 0x2b6b88u);
		for (yy = gy - h; yy < gy; yy++)
			for (xx = bx; xx < bx + w; xx++)
				if (((xx - bx) + (yy - (gy - h))) % 9 == 0)
					P(xx, yy, 0x86d8e6u);
		R(bx, gy - h, w, 1, 0xa8e6f0u);
	}
	R(240, gy - 18, 30, 18, 0x356f7fu);
	for (k = 0; k < 30; k++) {
		int hh = RD(5LL * isin(RD(256LL * k, 58)), 1024);

		R(240 + k, gy - 18 - hh, 1, hh, 0x356f7fu);
	}
	palm(c, 40, gy - 4, 22, 0x4a3a2au, 0x2e6a3au);
	palm(c, 206, gy - 4, 18, 0x4a3a2au, 0x2e6a3au);
	palm(c, 292, gy - 4, 24, 0x4a3a2au, 0x2e6a3au);
	R(0, gy - 15, CV_W, 1, 0x384450u);
	tx2 = (int)((3 * T) % 800) - 120;	/* train x in half pixels: (t * 1.5) % (W + 80) - 60 */
	tx = RD(tx2, 2);
	R(tx, gy - 12, 46, 8, 0xeef0f2u);
	R(tx, gy - 8, 46, 1, 0x2f6fd0u);
	for (k = 0; k < 6; k++)
		R(RD(tx2 + 2 * (3 + k * 7), 2), gy - 11, 4, 2, 0x3a4a5cu);
	L(RD(tx2 + 40, 2), gy - 12, RD(tx2 + 48, 2), gy - 15, 0x384450u);
	L(RD(tx2 + 56, 2), gy - 12, RD(tx2 + 48, 2), gy - 15, 0x384450u);
	R(0, gy - 4, CV_W, 4, 0x3b3d46u);
	R(0, gy - 4, CV_W, 1, 0x55586au);
}

static void nyc(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[3] = {0x04071au, 0x0a1334u, 0x16265au};
	int k, x, xx, yy, w, h;
	uint32_t ds = 0x0d1430u;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 3);
	for (k = 0; k < 26; k++) {
		int sx = art_below(r, CV_W), sy = y0 + art_below(r, 18);

		if (((k * 7 + (int)((t >> 3) % 9)) % 9) != 0)
			P(sx, sy, 0x93a6ffu);
	}
	x = 0;
	while (x < 240) {
		w = 8 + art_below(r, 10);
		h = 22 + art_below(r, 22);
		BLD(x, gy - h, w, gy, 0x121a3eu, 0xb08d3au, 12);
		x += w + 1;
	}
	R(226, gy - 7, 94, 3, 0x0e1a40u);
	x = -4;
	while (x < 226) {
		w = 10 + art_below(r, 12);
		h = 12 + art_below(r, 20);
		BLD(x, gy - h, w < 226 - x ? w : 226 - x, gy, 0x0a0f28u, 0xffd76au, 35);
		if (art_below(r, 100) < 30) {
			int wx = x + 3, wt = gy - h;

			R(wx, wt - 6, 5, 4, 0x4a3324u);
			TRI(wx - 1, wt - 6, wx + 5, wt - 6, wx + 2, wt - 9, 0x3a2618u);
			R(wx, wt - 2, 1, 2, 0x3a2618u);
			R(wx + 4, wt - 2, 1, 2, 0x3a2618u);
		}
		x += w;
	}
	R(194, gy - 30, 22, 30, ds);
	R(197, gy - 38, 16, 8, ds);
	R(200, gy - 44, 10, 6, ds);
	R(203, gy - 49, 4, 5, ds);
	R(204, gy - 55, 2, 6, 0xc9d2ffu);
	for (k = 0; k < 4; k++) {
		P(198 + k * 4, gy - 37, 0xffe9a8u);
		P(201 + k * 2, gy - 43, 0xffe9a8u);
	}
	for (yy = gy - 28; yy < gy - 2; yy += 3)
		for (xx = 196; xx < 214; xx += 3)
			if (art_below(r, 100) < 40)
				P(xx, yy, 0xffd76au);
	R(270, gy - 30, 3, 26, 0x26345fu);
	R(306, gy - 30, 3, 26, 0x26345fu);
	for (xx = 232; xx < 320; xx++) {
		if (xx < 271)
			yy = RD((gy - 8) * 39LL - 22LL * (xx - 232), 39);
		else if (xx <= 307)
			yy = RD((gy - 12) * 18LL - (long long)(xx - 289) * (xx - 289), 18);	/* the cable parabola */
		else
			yy = RD((gy - 30) * 5LL + 7LL * (xx - 307), 5);
		P(xx, yy, xx % 6 == 0 ? 0xffe8a0u : 0x4a5ea0u);
	}
	R(228, gy - 8, 92, 2, 0x26345fu);
	R(0, gy - 4, CV_W, 4, 0x060a1cu);
}

static void mia_hotel(Canvas *c, int gy, int x, int top, int w, uint32_t col, uint32_t sh)
{
	int xx, yy;

	R(x, top, w, gy - 4 - top, col);
	R(x - 1, top, w + 2, 2, sh);
	for (yy = top + 6; yy < gy - 6; yy += 5) {
		R(x + 2, yy, w - 4, 1, sh);
		for (xx = x + 3; xx < x + w - 3; xx += 4)
			R(xx, yy - 3, 2, 2, 0x2d5d6eu);
	}
	R(x + (w >> 1) - 1, top - 7, 3, 9, 0xff3f8eu);
	P(x + (w >> 1), top - 8, 0xffd6eau);
}

static void mia(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[5] = {0x4f2580u, 0xb23c8au, 0xf0606eu, 0xff9a58u, 0xffcf66u};
	long long T = t;
	int k;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 5);
	D(84, gy - 16, 13, 0xfff0a6u);
	R(0, gy - 16, 176, 12, 0x1a8b99u);
	R(0, gy - 16, 176, 1, 0x7fe0dcu);
	for (k = 0; k < 4; k++)
		R(78 - k * 2, gy - 13 + k * 3, 12 + 4 * k, 1, 0xffd08au);
	for (k = 0; k < 14; k++)
		R((int)((k * 29 + T) % 176), gy - 14 + (k % 4) * 3, 3, 1, 0xbff6efu);
	R(0, gy - 4, CV_W, 4, 0xf0d39au);
	mia_hotel(c, gy, 150, gy - 24, 34, 0xff9ec4u, 0xd96f9eu);
	mia_hotel(c, gy, 188, gy - 34, 32, 0x97eccbu, 0x5fb894u);
	mia_hotel(c, gy, 224, gy - 22, 28, 0xc7a6ffu, 0x9a78d6u);
	mia_hotel(c, gy, 256, gy - 30, 36, 0xffe08au, 0xd8b45cu);
	mia_hotel(c, gy, 296, gy - 20, 28, 0x9fdcffu, 0x6cb3dcu);
	palm(c, 140, gy - 4, 24, 0x3a2a3au, 0x1f4a3au);
	palm(c, 220, gy - 4, 16, 0x3a2a3au, 0x1f4a3au);
	palm(c, 300, gy - 4, 22, 0x3a2a3au, 0x1f4a3au);
}

static void nola(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x1c0e35u, 0x3c1d5cu, 0x743875u, 0xb25c78u};
	static const uint32_t hc[7] = {0xe8d5a2u, 0x3f9f98u, 0xdf795eu, 0x86a85au, 0xd39ec4u, 0xefbf58u, 0x7fb3c8u};
	static const int lamp[3] = {58, 170, 250};
	uint32_t ca = 0x24133cu;
	int x = 0, i = 0, f, xx, k;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(282, y0 + 12, 6, 0xfff3c8u);
	R(132, gy - 24, 46, 20, ca);
	R(150, gy - 38, 10, 14, ca);
	TRI(149, gy - 38, 161, gy - 38, 155, gy - 52, ca);
	R(136, gy - 30, 8, 6, ca);
	TRI(135, gy - 30, 145, gy - 30, 140, gy - 40, ca);
	R(166, gy - 30, 8, 6, ca);
	TRI(165, gy - 30, 175, gy - 30, 170, gy - 40, ca);
	while (x < CV_W) {
		int w = 26 + art_below(r, 14), h = art_below(r, 100) < 50 ? 22 : 30, mid;

		R(x, gy - h, w, h, hc[i % 7]);
		i++;
		R(x, gy - h, w, 2, 0x2a1a2eu);
		mid = gy - RD(h * 55LL, 100);
		for (f = 0; f < 2; f++) {
			int wy = f == 0 ? gy - h + 5 : mid + 3;

			for (xx = x + 3; xx < x + w - 4; xx += 7) {
				R(xx, wy, 3, 6, 0x2b2238u);
				P(xx - 1, wy + 1, 0x2f6b4au);
				P(xx + 3, wy + 1, 0x2f6b4au);
			}
		}
		R(x, mid, w, 1, 0x141018u);
		R(x, mid - 4, w, 1, 0x141018u);
		for (xx = x; xx < x + w; xx += 2)
			R(xx, mid - 3, 1, 3, 0x141018u);
		x += w;
	}
	for (k = 0; k < 3; k++) {
		int lx = lamp[k], on = (int)(((t >> 2) + (uint32_t)lx) % 11) != 0;

		R(lx, gy - 15, 1, 11, 0x141018u);
		R(lx - 1, gy - 17, 3, 2, on ? 0xffd36au : 0xa8843au);
	}
	R(0, gy - 4, CV_W, 4, 0x1e1426u);
}

static void pdx(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x55626eu, 0x73808cu, 0x939fa8u, 0xb2bbc1u};
	static const int pines[10][2] = {{8, 24}, {20, 30}, {34, 20}, {128, 22}, {142, 28},
					 {256, 20}, {270, 30}, {286, 24}, {302, 32}, {314, 22}};
	long long T = t;
	uint32_t tc = 0x3b4957u, tw = 0xdfe8eeu, tr = 0x2f5f80u;
	int k, xx;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	TRI(36, gy - 6, 156, gy - 6, 96, gy - 46, 0x677a8cu);
	TRI(96, gy - 46, 156, gy - 6, 118, gy - 6, 0x596d80u);
	TRI(80, gy - 35, 112, gy - 35, 96, gy - 46, 0xeef3f7u);
	TRI(80, gy - 35, 87, gy - 35, 83, gy - 30, 0xeef3f7u);
	TRI(90, gy - 35, 98, gy - 35, 94, gy - 29, 0xeef3f7u);
	TRI(103, gy - 35, 111, gy - 35, 107, gy - 31, 0xeef3f7u);
	BLD(170, gy - 30, 12, gy, tc, tw, 15);
	BLD(184, gy - 22, 14, gy, tc, tw, 15);
	BLD(200, gy - 34, 10, gy, tc, tw, 15);
	TRI(200, gy - 34, 210, gy - 34, 205, gy - 41, tc);
	BLD(212, gy - 18, 16, gy, tc, tw, 15);
	BLD(230, gy - 26, 12, gy, tc, tw, 15);
	R(150, gy - 8, 170, 4, 0x3e5a6eu);
	R(150, gy - 11, 170, 2, tr);
	R(150, gy - 19, 170, 1, tr);
	for (xx = 150; xx < 320; xx += 10) {
		L(xx, gy - 10, xx + 5, gy - 19, tr);
		L(xx + 5, gy - 19, xx + 10, gy - 10, tr);
	}
	for (k = 0; k < 10; k++)
		pine(c, pines[k][0], gy - 4, pines[k][1], 0x1d4632u, 0x28603fu);
	R(0, gy - 4, CV_W, 4, 0x2a3238u);
	for (k = 0; k < 46; k++) {
		int rx = (int)((k * 41 + T * 2) % CV_W), ry = y0 + (int)((k * 17 + T * 6) % (gy - y0));

		L(rx, ry, rx - 1, ry + 3, 0xcad4ddu);
	}
}

void art_backdrop(Canvas *c, int city, int y0, int h, uint32_t t)
{
	static void (*const bd[7])(Canvas *, int, int, ArtRng *, uint32_t) = {okc, hou, sjc, nyc, mia, nola, pdx};
	int sx0 = c->cx0, sy0 = c->cy0, sx1 = c->cx1, sy1 = c->cy1;
	ArtRng r;

	if (city < 0 || city > 6 || h <= 0)
		return;
	cv_clip(c, 0, y0, CV_W, y0 + h);
	if (c->cx0 < sx0) c->cx0 = sx0;
	if (c->cy0 < sy0) c->cy0 = sy0;
	if (c->cx1 > sx1) c->cx1 = sx1;
	if (c->cy1 > sy1) c->cy1 = sy1;
	art_seed(&r, 911 + (uint32_t)city * 31);
	bd[city](c, y0, y0 + h, &r, t);
	c->cx0 = sx0; c->cy0 = sy0; c->cx1 = sx1; c->cy1 = sy1;
}
