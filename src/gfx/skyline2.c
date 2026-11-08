/* The eleven North American backdrops: Dallas, San Francisco, Los Angeles, Seattle, Cincinnati,
 * Dayton, Destin, Tijuana, Cozumel, Los Cabos and Sinaloa.  Same recipe as skyline.c: a dithered
 * sky gradient, far and near layers, one or two landmarks, a ground strip, and motion from the
 * tick t (t = 0 is a still).  Every shape is a generic silhouette; no lettering, no marks.
 * Rows y0..gy-1 only; the last four rows are the ground strip.  Integers only. */
#include "gfx/skyline_int.h"

static int wrap(long long a, int m)
{
	long long q = a % m;

	return (int)(q < 0 ? q + m : q);
}

/* sin of a turn fraction: x / period of a full turn, -1024..1024 */
static int sn(long long x, int period)
{
	return isin((int)((x * 256) / period));
}

/* ---------------------------------------------------------------- Dallas */

static void dal(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x24164eu, 0x5e327au, 0xcf5a7au, 0xf79c5cu};
	const uint32_t body = 0x120f30u, neon = 0x39f08cu, win = 0xffd070u;
	long long T = t;
	int x = 0, w, h, k;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	while (x < CV_W) {					/* hazy far towers */
		w = 9 + art_below(r, 10);
		h = 14 + art_below(r, 14);
		R(x, gy - h, w, h, 0x3e2862u);
		x += w + 1;
	}
	x = 92;
	while (x < CV_W) {					/* near towers, green outlines */
		w = 10 + art_below(r, 8);
		h = 16 + art_below(r, 20);
		BLD(x, gy - h, w, gy, body, win, 22);
		cv_box(c, x, gy - h, w, h - 3, neon);
		x += w + 2;
	}
	/* angular tower with a cut roof */
	R(176, gy - 42, 15, 40, body);
	TRI(176, gy - 42, 191, gy - 42, 176, gy - 49, body);
	L(176, gy - 49, 191, gy - 42, neon);
	L(176, gy - 49, 176, gy - 4, neon);
	L(190, gy - 42, 190, gy - 4, neon);
	for (k = gy - 38; k < gy - 6; k += 4)
		R(178, k, 11, 1, 0x1f1b4cu);
	for (k = 0; k < 6; k++)
		R(179 + k * 2, gy - 40, 1, 33, 0x251f5cu);
	/* round-topped tower */
	R(236, gy - 36, 14, 32, body);
	D(243, gy - 36, 7, body);
	L(236, gy - 36, 236, gy - 4, neon);
	L(249, gy - 36, 249, gy - 4, neon);
	for (k = 0; k < 16; k++) {
		int a = 128 + k * 8;

		P(243 + RD(7LL * icos(a), 1024), gy - 36 + RD(7LL * isin(a), 1024), neon);
	}
	for (k = gy - 32; k < gy - 6; k += 4)
		R(238, k, 10, 1, 0x1f1b4cu);
	/* ball-topped observation tower */
	L(49, gy - 4, 55, gy - 26, 0x2a2450u);
	L(63, gy - 4, 57, gy - 26, 0x2a2450u);
	R(55, gy - 34, 3, 30, 0x2a2450u);
	R(56, gy - 34, 1, 30, 0x4a3f80u);
	R(54, gy - 24, 5, 1, 0x2a2450u);
	D(56, gy - 39, 7, 0x1c2a60u);
	cv_ring(c, 56, gy - 39, 7, 0xbfeaffu);
	L(49, gy - 39, 63, gy - 39, 0x7fa8d8u);
	L(56, gy - 46, 56, gy - 32, 0x7fa8d8u);
	L(51, gy - 44, 61, gy - 34, 0x7fa8d8u);
	L(61, gy - 44, 51, gy - 34, 0x7fa8d8u);
	for (k = 0; k < 8; k++) {
		int a = k * 32 + 16;

		if ((int)((T / 3 + k * 3) % 5) != 0)
			P(56 + RD(7LL * icos(a), 1024), gy - 39 + RD(7LL * isin(a), 1024), 0xffffffu);
	}
	L(56, gy - 46, 56, gy - 54, 0xdfe8ffu);
	if ((T / 6) % 2 == 0)
		P(56, gy - 55, 0xff4a4au);
	R(0, gy - 4, CV_W, 4, 0x0e0a20u);
	for (k = 0; k < 5; k++) {
		int cx0 = wrap(k * 71 + T * (k % 2 ? 2 : 3), CV_W + 10) - 5;

		R(cx0, gy - 3, 3, 1, k % 2 ? 0xfff0b0u : 0xff4a4au);
	}
}

/* ---------------------------------------------------------------- San Francisco */

static int sf_hill(int gy, int x)
{
	return x < 120 ? gy - 8 - RD(20LL * (120 - x), 120) : gy - 8;
}

static void sfo(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x7fb0d8u, 0xb9d0e0u, 0xe8d8d0u, 0xf8e0c0u};
	static const uint32_t pastel[5] = {0xf2c8d0u, 0xf6e6a8u, 0xa8d8c8u, 0xc8b8e8u, 0xf2a888u};
	const uint32_t red = 0xd64a2cu, dark = 0xa8341eu;
	long long T = t;
	int x, i, k, tx1 = 190, tx2 = 288, ct;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	for (x = 150; x < CV_W; x++) {			/* far headland */
		int h = gy - 30 - RD(5LL * sn(x * 3, 200) + 3LL * sn(x * 7 + 40, 90), 1024);

		R(x, h, 1, gy - h, 0x7a98a0u);
	}
	for (i = 0; i < 6; i++) {				/* downtown behind the hill */
		static const int bx[6] = {84, 96, 108, 120, 134, 146};
		static const int bh[6] = {18, 26, 20, 30, 16, 22};

		R(bx[i], gy - bh[i], 10, bh[i], i % 2 ? 0xd8b8b8u : 0xe8d0c8u);
		R(bx[i] + 7, gy - bh[i], 3, bh[i], 0xb89898u);
		for (k = gy - bh[i] + 2; k < gy - 8; k += 3)
			P(bx[i] + 3, k, 0x8a7a8au);
	}
	/* water and bridge */
	R(150, gy - 14, CV_W - 150, 10, 0x4a7a98u);
	for (k = 0; k < 18; k++)
		R(150 + wrap(k * 37 + T, 170), gy - 12 + (k % 4) * 2, 3, 1, 0x9ac4d8u);
	for (x = tx1; x <= tx2; x++) {			/* main cable */
		int d = x - (tx1 + tx2) / 2, hf = (tx2 - tx1) / 2;
		int y = gy - 44 + RD(26LL * (hf * hf - d * d), hf * hf);

		P(x, y, red);
		if (x % 4 == 0)
			L(x, y + 1, x, gy - 16, dark);
	}
	L(tx1 + 1, gy - 44, 150, gy - 16, red);
	L(tx2 + 5, gy - 44, CV_W - 1, gy - 30, red);
	for (x = 154; x < tx1; x += 5)
		L(x, gy - 16, x, gy - 16 - RD((x - 150) * 28LL, 40) , dark);
	R(150, gy - 15, CV_W - 150, 2, red);
	R(150, gy - 13, CV_W - 150, 1, 0x7a2a1au);
	for (i = 0; i < 2; i++) {
		int tx = i ? tx2 : tx1;

		R(tx, gy - 46, 2, 36, red);
		R(tx + 6, gy - 46, 2, 36, dark);
		for (k = 0; k < 5; k++)
			R(tx, gy - 42 + k * 7, 8, 1, red);
		R(tx - 1, gy - 47, 10, 2, red);
	}
	/* rolling fog: solid crest, then a dither that thins out; it drifts with t */
	for (x = 0; x < CV_W; x++) {
		int fy = gy - 26 + RD(3LL * sn(x * 2 + T, 150) + 2LL * sn(x * 5 - 2 * T, 110), 1024);

		R(x, fy, 1, 2, 0xeff3f2u);
		for (k = fy + 2; k < fy + 9; k++)
			if (((x + k) & 1) == 0 || (k < fy + 4))
				P(x, k, k < fy + 4 ? 0xe4eaeau : 0xd4dedeu);
	}
	/* the near hill: houses on a steep street, with a pointed tower and its wings */
	for (x = 0; x < 150; x++) {
		int hy = sf_hill(gy, x);

		R(x, hy, 1, gy - hy, 0x5c7a4cu);
	}
	for (x = 3; x < 118; x += 8) {
		int hy = sf_hill(gy, x);

		R(x, hy - 6, 7, 6, pastel[(x / 8) % 5]);
		R(x - 1, hy - 7, 9, 1, 0x7a4a3au);
		P(x + 2, hy - 4, 0x4a3a52u);
		P(x + 4, hy - 4, 0x4a3a52u);
	}
	for (x = 0; x < 150; x++) {
		int hy = sf_hill(gy, x);

		R(x, hy + 3, 1, 2, 0x3a3a44u);
	}
	TRI(133, gy - 8, 143, gy - 8, 138, gy - 50, 0xf2eee8u);
	TRI(138, gy - 50, 143, gy - 8, 138, gy - 8, 0xc4bcc0u);
	R(131, gy - 28, 3, 20, 0xe0d8d8u);
	R(142, gy - 28, 3, 20, 0xb4acb0u);
	R(138, gy - 56, 1, 6, 0xf2eee8u);
	for (k = gy - 44; k < gy - 10; k += 4)
		P(138, k, 0x6a6a7au);
	/* the cable car climbs and descends the hill street */
	ct = wrap(T, 220);
	x = 8 + (ct < 110 ? ct : 220 - ct);
	{
		int hy = sf_hill(gy, x + 4) + 1;

		R(x, hy - 5, 9, 4, 0xc83a2au);
		R(x - 1, hy - 6, 11, 1, 0xf0e0b0u);
		R(x + 1, hy - 4, 2, 2, 0xf6e6a8u);
		R(x + 5, hy - 4, 2, 2, 0xf6e6a8u);
		P(x + 1, hy - 1, 0x222222u);
		P(x + 7, hy - 1, 0x222222u);
	}
	R(0, gy - 4, CV_W, 4, 0x3a3a44u);
	R(0, gy - 4, CV_W, 1, 0x585868u);
}

/* ---------------------------------------------------------------- Los Angeles */

static void lax(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0xa65a86u, 0xe27a7cu, 0xf5a070u, 0xfcc888u};
	static const int pm[7][2] = {{14, 26}, {58, 20}, {96, 28}, {150, 22}, {196, 27}, {250, 21}, {298, 25}};
	long long T = t;
	int x, i, k;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(236, gy - 26, 12, 0xffe6aau);
	D(236, gy - 26, 8, 0xfff2c8u);
	for (i = 0; i < 9; i++) {				/* hazy downtown */
		static const int bx[9] = {150, 162, 174, 190, 206, 220, 252, 266, 284};
		static const int bh[9] = {22, 30, 24, 36, 26, 20, 28, 18, 24};

		R(bx[i], gy - bh[i], 11, bh[i], 0xc27a86u);
		if (i == 3) {
			R(192, gy - 41, 7, 5, 0xc27a86u);
			R(194, gy - 45, 3, 4, 0xc27a86u);
		}
	}
	for (x = 0; x < CV_W; x++) {				/* the hills */
		int h = gy - 20 - RD(9LL * sn(x * 2 + 30, 250) + 5LL * sn(x * 5, 120), 1024) + (x > 150 ? 8 : 0);

		R(x, h, 1, gy - h, 0x6e4466u);
	}
	TRI(22, gy - 12, 40, gy - 29, 40, gy - 12, 0x6e4466u);
	R(40, gy - 29, 36, 17, 0x6e4466u);
	TRI(76, gy - 29, 76, gy - 12, 96, gy - 12, 0x6e4466u);
	/* hilltop observatory: a main dome, two small ones, a slender needle */
	R(40, gy - 36, 34, 8, 0xe8d8c8u);
	for (k = 0; k < 6; k++)
		R(42 + k * 6, gy - 35, 2, 6, 0xc8b0a4u);
	for (k = 0; k < 26; k++) {
		int hh = isqrt(81 - (k - 13) * (k - 13) * 81 / 169);

		R(51 + k, gy - 36 - hh, 1, hh, 0x7ab0a0u);
	}
	D(44, gy - 37, 3, 0x7ab0a0u);
	D(70, gy - 37, 3, 0x7ab0a0u);
	R(34, gy - 38, 2, 12, 0xe8d8c8u);
	P(34, gy - 39, 0xfff2c8u);
	R(0, gy - 13, CV_W, 4, 0x4a2e44u);			/* the freeway */
	R(0, gy - 14, CV_W, 1, 0x8a5a60u);
	for (x = 4; x < CV_W; x += 24)
		R(x, gy - 9, 3, 5, 0x4a2e44u);
	for (k = 0; k < 8; k++) {
		int cx0 = wrap(k * 41 + T * (k % 2 ? -2 : 3), CV_W + 16) - 8;

		R(cx0, gy - (k % 2 ? 12 : 11), 4, 1, k % 2 ? 0xff5f6du : 0xfff0b0u);
	}
	for (i = 0; i < 7; i++)
		art_palm(c, pm[i][0], gy - 4, pm[i][1], 0x3a2438u, 0x24442eu);
	R(0, gy - 4, CV_W, 4, 0x3a2438u);
}

/* ---------------------------------------------------------------- Seattle */

static void sea1(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x5a6a7au, 0x7e8e9cu, 0xa4b2bcu, 0xc8d0d4u};
	static const int tw[7][2] = {{70, 22}, {82, 30}, {94, 20}, {106, 34}, {120, 24}, {134, 28}, {146, 18}};
	long long T = t;
	int i, k, fx;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	/* the snowy volcano in the distance, with a blunt crater */
	TRI(150, gy - 16, 320, gy - 16, 236, gy - 52, 0x7e92b0u);
	TRI(236, gy - 52, 320, gy - 16, 270, gy - 16, 0x6e82a2u);
	R(228, gy - 52, 16, 2, 0x7e92b0u);
	TRI(212, gy - 40, 260, gy - 40, 236, gy - 52, 0xeaf0f6u);
	R(228, gy - 53, 16, 1, 0xeaf0f6u);
	for (k = 0; k < 7; k++)
		TRI(212 + k * 7, gy - 40, 220 + k * 7, gy - 40, 216 + k * 7, gy - 34 + (k % 3) * 2, 0xeaf0f6u);
	TRI(236, gy - 52, 260, gy - 40, 250, gy - 40, 0xc6d2e0u);
	R(0, gy - 16, 150, 12, 0x6c7d86u);			/* far shore */
	R(150, gy - 14, 170, 10, 0x6c7d86u);
	/* the downtown towers and a slim saucer-topped landmark */
	for (i = 0; i < 7; i++) {
		BLD(tw[i][0], gy - tw[i][1], 11, gy - 12, 0x46566au, 0xe8e0a8u, 18);
		R(tw[i][0] + 8, gy - tw[i][1], 3, tw[i][1] - 12, 0x364458u);
	}
	L(60, gy - 12, 63, gy - 34, 0x8a98a6u);
	L(68, gy - 12, 65, gy - 34, 0x8a98a6u);
	R(62, gy - 24, 4, 1, 0x8a98a6u);
	R(57, gy - 39, 16, 3, 0xaab6c2u);
	R(60, gy - 41, 10, 2, 0xaab6c2u);
	R(64, gy - 49, 2, 8, 0x8a98a6u);
	/* port cranes */
	for (i = 0; i < 3; i++) {
		int cx0 = 214 + i * 38;

		R(cx0, gy - 28, 2, 22, 0x48606eu);
		R(cx0 + 12, gy - 28, 2, 22, 0x48606eu);
		R(cx0 - 6, gy - 30, 30, 2, 0x48606eu);
		L(cx0 + 22, gy - 30, cx0 + 28, gy - 38, 0x48606eu);
		R(cx0 - 1, gy - 32, 4, 2, 0xc8622au);
		R(cx0 + 4, gy - 16, 6, 4, 0xa84a3au);
	}
	/* the bay and the ferry */
	for (i = 0; i < 12; i++) {				/* evergreens along the front */
		static const int px[12] = {6, 18, 30, 82, 94, 120, 132, 168, 180, 296, 308, 314};
		static const int ph[12] = {26, 32, 24, 22, 28, 20, 26, 22, 28, 30, 24, 20};

		art_pine(c, px[i], gy - 12, ph[i] - 4, 0x1d4632u, 0x28603fu);
	}
	R(0, gy - 12, CV_W, 8, 0x3c5a6eu);
	for (k = 0; k < 20; k++)
		R(wrap(k * 33 + T, CV_W), gy - 11 + (k % 6), 3, 1, 0x6a8ca0u);
	fx = wrap(T * 3 / 2, CV_W + 90) - 50;
	R(fx, gy - 9, 32, 4, 0xe8ece8u);
	R(fx, gy - 6, 32, 2, 0x1f6a4au);
	R(fx + 3, gy - 13, 26, 4, 0xe8ece8u);
	R(fx + 12, gy - 17, 8, 4, 0xe8ece8u);
	R(fx + 14, gy - 21, 3, 4, 0xc8622au);
	for (k = 0; k < 6; k++)
		R(fx + 5 + k * 4, gy - 12, 2, 2, 0x35505cu);
	R(fx + 13, gy - 16, 6, 1, 0x35505cu);
	L(fx - 2, gy - 4, fx - 10, gy - 4, 0xa8c0cau);
	R(0, gy - 4, CV_W, 4, 0x2a3238u);
	(void)r;
	for (k = 0; k < 30; k++) {				/* a fine drizzle, shorter and slower than a downpour */
		int rx = wrap(k * 53 + T / 2, CV_W), ry = y0 + wrap(k * 29 + T * 3, gy - y0 - 4);

		P(rx, ry, 0xd6dee4u);
		P(rx, ry + 1, 0xaab6beu);
	}
}

/* ---------------------------------------------------------------- Cincinnati */

static void cvg(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x4a7cc0u, 0x7aa6d8u, 0xb8cce0u, 0xf0d8b8u};
	static const uint32_t roof[4] = {0xa84a3au, 0x7a4a3au, 0xb86a4au, 0x8a5a4au};
	const uint32_t blue = 0x2f5fa8u, stone = 0x8a7a68u;
	long long T = t;
	int x, i, k, tx1 = 128, tx2 = 262, bx;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	x = 100;
	while (x < CV_W) {					/* the skyline across the river */
		int w = 10 + art_below(r, 8), h = 14 + art_below(r, 16);

		BLD(x, gy - 14 - h, w, gy - 14, 0x4a5a78u, 0xf0e0a8u, 18);
		x += w + 1;
	}
	R(184, gy - 52, 12, 38, 0x56667eu);			/* the tall stepped tower */
	R(186, gy - 58, 8, 6, 0x56667eu);
	R(188, gy - 64, 4, 6, 0x56667eu);
	R(189, gy - 68, 2, 4, 0x56667eu);
	for (k = gy - 48; k < gy - 16; k += 3)
		P(190, k, 0xf0e0a8u);
	for (x = 0; x < 120; x++) {				/* the hillside and its houses */
		int hy = gy - 12 - RD(30LL * (120 - x), 120);

		R(x, hy, 1, gy - 14 - hy, 0x4f7048u);
	}
	for (i = 0; i < 12; i++) {
		static const int hx[12] = {4, 14, 26, 36, 48, 58, 70, 80, 90, 98, 106, 112};
		int hy = gy - 12 - RD(30LL * (120 - hx[i]), 120);

		R(hx[i], hy - 5, 7, 5, i % 3 == 0 ? 0xd8c4a0u : 0xc8a888u);
		TRI(hx[i] - 1, hy - 5, hx[i] + 8, hy - 5, hx[i] + 3, hy - 9, roof[i % 4]);
		P(hx[i] + 2, hy - 3, 0x3a2a30u);
		P(hx[i] + 4, hy - 3, 0x3a2a30u);
	}
	R(0, gy - 14, CV_W, 10, 0x2a4a68u);			/* the river */
	for (k = 0; k < 22; k++)
		R(wrap(k * 31 + T, CV_W), gy - 12 + (k % 4) * 2, 3, 1, 0x7ab0d0u);
	for (x = tx1; x <= tx2; x++) {				/* the blue suspension bridge */
		int d = x - (tx1 + tx2) / 2, hf = (tx2 - tx1) / 2;
		int y = gy - 40 + RD(20LL * (hf * hf - d * d), hf * hf);

		P(x, y, blue);
		if (x % 4 == 0)
			L(x, y + 1, x, gy - 17, blue);
	}
	L(tx1 + 2, gy - 40, 100, gy - 17, blue);
	L(tx2 + 2, gy - 40, 296, gy - 17, blue);
	for (k = 1; k < 5; k++) {				/* fan of diagonal stays */
		L(tx1 + 2, gy - 38, tx1 + 2 - k * 7, gy - 17, 0x4a78c0u);
		L(tx2 + 2, gy - 38, tx2 + 2 + k * 7, gy - 17, 0x4a78c0u);
	}
	R(96, gy - 17, 204, 3, blue);
	R(96, gy - 15, 204, 1, 0x1f3f78u);
	for (i = 0; i < 2; i++) {
		int tx = i ? tx2 : tx1;

		R(tx - 1, gy - 42, 8, 30, stone);
		R(tx + 5, gy - 42, 2, 30, 0x6a5a4au);
		R(tx + 1, gy - 28, 4, 12, 0x5a4a3eu);		/* the arched opening */
		R(tx + 2, gy - 30, 2, 2, 0x5a4a3eu);
		R(tx - 2, gy - 44, 10, 3, 0x9a8a76u);
		TRI(tx - 1, gy - 44, tx + 7, gy - 44, tx + 3, gy - 50, 0x6a5a4au);
	}
	/* the riverboat: tall chimneys, white decks, a turning paddle wheel at the stern */
	bx = 176 + RD(10LL * sn(T, 160), 1024);
	R(bx, gy - 9, 34, 3, 0xf4f0e8u);
	R(bx, gy - 6, 34, 2, 0x2a2a3au);
	R(bx + 4, gy - 13, 24, 4, 0xf4f0e8u);
	R(bx + 8, gy - 16, 16, 3, 0xf4f0e8u);
	for (k = 0; k < 6; k++)
		P(bx + 6 + k * 4, gy - 12, 0x35505cu);
	R(bx + 9, gy - 25, 2, 9, 0x222230u);
	R(bx + 19, gy - 25, 2, 9, 0x222230u);
	R(bx + 8, gy - 26, 4, 1, 0x222230u);
	R(bx + 18, gy - 26, 4, 1, 0x222230u);
	D(bx + 35, gy - 8, 5, 0xb02a2au);
	D(bx + 35, gy - 8, 3, 0x7ab0d0u);
	for (k = 0; k < 6; k++) {
		int a = k * 43 + (int)(T * 9 % 256);

		L(bx + 35, gy - 8, bx + 35 + RD(5LL * icos(a), 1024), gy - 8 + RD(5LL * isin(a), 1024), 0xf0e0c8u);
	}
	R(0, gy - 4, CV_W, 4, 0x35404cu);
}

/* ---------------------------------------------------------------- Dayton */

static void dtn(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x5d9ad0u, 0x8fbfe0u, 0xc8e0ecu, 0xf4ecd0u};
	static const uint32_t brick[4] = {0xa8584au, 0xb87a5au, 0x8a4a42u, 0xc89a72u};
	long long T = t;
	int x = 0, k, px, py, tx, rg = gy - 15;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(40, y0 + 12, 6, 0xfff6d0u);
	while (x < CV_W) {					/* a modest skyline */
		int w = 12 + art_below(r, 10), h = 10 + art_below(r, 12), tall = art_below(r, 100) < 18;

		if (tall)
			h += 14;
		BLD(x, rg - h, w, rg, brick[art_below(r, 4)], 0xf6e6a8u, 30);
		R(x - 1, rg - h, w + 2, 2, 0xd8c0a0u);
		if (tall)
			R(x + (w >> 1), rg - h - 4, 2, 4, 0x6a4a42u);
		x += w + 1;
	}
	R(96, rg - 20, 2, 20, 0x6a5a52u);			/* a water tower on legs */
	R(104, rg - 20, 2, 20, 0x6a5a52u);
	L(96, rg - 4, 105, rg - 14, 0x6a5a52u);
	L(105, rg - 4, 96, rg - 14, 0x6a5a52u);
	R(93, rg - 34, 16, 14, 0xc8b8a4u);
	R(93, rg - 34, 16, 2, 0x9a8a78u);
	TRI(93, rg - 34, 109, rg - 34, 101, rg - 40, 0x6a5a52u);
	R(0, rg, CV_W, 6, 0x3f6f8au);				/* the river */
	for (k = 0; k < 16; k++)
		R(wrap(k * 41 + T, CV_W), rg + 1 + (k % 4), 3, 1, 0x9ac8dcu);
	R(0, gy - 9, CV_W, 5, 0x5a5a62u);			/* the street */
	R(0, gy - 10, CV_W, 1, 0x8a8a8cu);
	for (x = 6; x < CV_W; x += 20)
		R(x, gy - 7, 8, 1, 0xe8d890u);
	R(0, gy - 24, CV_W, 1, 0x2a2a30u);			/* overhead wires and their poles */
	for (x = 24; x < CV_W; x += 70)
		R(x, gy - 25, 2, 21, 0x3a3a40u);
	tx = wrap(T, CV_W + 70) - 40;
	R(tx, gy - 17, 26, 10, 0x2f7a5au);
	R(tx, gy - 17, 26, 3, 0xf0e4b8u);
	for (k = 0; k < 5; k++)
		R(tx + 2 + k * 5, gy - 14, 3, 3, 0xa8d8eau);
	R(tx + 22, gy - 14, 3, 3, 0xe8d890u);
	L(tx + 8, gy - 17, tx + 14, gy - 24, 0x2a2a30u);
	D(tx + 5, gy - 6, 2, 0x161616u);
	D(tx + 20, gy - 6, 2, 0x161616u);
	px = wrap(T * 2, CV_W + 80) - 40;			/* the biplane */
	py = y0 + 12 + RD(3LL * sn(T, 50), 1024);
	R(px - 7, py - 1, 15, 3, 0xf0e6c8u);
	R(px - 6, py - 4, 13, 2, 0xd8482au);
	R(px - 5, py + 2, 11, 2, 0xb03a22u);
	L(px - 3, py - 2, px - 3, py + 2, 0x4a3a30u);
	L(px + 3, py - 2, px + 3, py + 2, 0x4a3a30u);
	TRI(px - 7, py - 1, px - 11, py - 4, px - 7, py + 1, 0xd8482au);
	D(px + 1, py - 1, 1, 0x4a3a30u);
	if (t & 1)
		L(px + 9, py - 2, px + 9, py + 3, 0x3a3a40u);
	else
		L(px + 8, py, px + 10, py, 0x3a3a40u);
	R(0, gy - 4, CV_W, 4, 0x3a3a42u);
}

/* ---------------------------------------------------------------- Destin */

static void dst(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x3aa0e8u, 0x6cc0f4u, 0xa8e0fau, 0xe4f6fcu};
	static const uint32_t sea[3] = {0x14946eu, 0x2cc08cu, 0x7be8c0u};
	long long T = t;
	int x, i, k, hz = gy - 26;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(272, y0 + 12, 7, 0xfffbe0u);
	D(60, y0 + 10, 5, 0xffffffu);				/* two puffy clouds */
	D(68, y0 + 11, 6, 0xffffffu);
	D(77, y0 + 10, 4, 0xffffffu);
	D(168, y0 + 18, 4, 0xf4fbffu);
	D(175, y0 + 18, 5, 0xf4fbffu);
	cv_grad(c, 0, hz, CV_W, gy - 4 - hz, sea, 3);
	for (k = 0; k < 26; k++)
		R(wrap(k * 27 + T, CV_W), hz + 2 + (k * 5) % 20, 4, 1, 0xc8f6e0u);
	R(0, hz, CV_W, 1, 0xa8e8d0u);
	/* the dunes, with a few tufts of sea oats */
	for (x = 196; x < CV_W; x++) {
		int h = gy - 16 - RD(5LL * sn(x * 2 + 20, 400) + 3LL * sn(x * 3, 180), 1024) - RD(14LL * (x - 196), 124);

		R(x, h, 1, gy - h, 0xf6efd8u);
		if ((x + h) % 5 == 0)
			P(x, h + 2, 0xe3d6b0u);
		if (x % 17 == 3) {
			L(x, h, x - 1, h - 3, 0x8aa050u);
			L(x, h, x + 1, h - 3, 0x8aa050u);
			L(x, h, x, h - 4, 0x8aa050u);
		}
	}
	R(0, gy - 9, 188, 2, 0x8a6a4au);			/* the harbor dock */
	for (x = 4; x < 188; x += 12)
		R(x, gy - 9, 2, 6, 0x5a4a3au);
	for (i = 0; i < 3; i++) {				/* charter fishing boats */
		static const int bx[3] = {18, 74, 130};
		int bob = RD(sn(T + i * 40, 80), 1024) + 0;
		int b0 = bx[i], by = gy - 11 + bob;

		R(b0, by, 34, 4, 0xf4f4f0u);
		R(b0, by + 3, 34, 1, 0x1f5f8au);
		TRI(b0 + 30, by, b0 + 38, by, b0 + 30, by + 4, 0xf4f4f0u);
		R(b0 + 8, by - 5, 14, 5, 0xe8e8e0u);
		R(b0 + 10, by - 4, 10, 2, 0x35505cu);
		R(b0 + 14, by - 12, 1, 7, 0xd8d8d0u);		/* tuna tower */
		R(b0 + 12, by - 12, 5, 1, 0xd8d8d0u);
		L(b0 + 24, by, b0 + 36, by - 14, 0x8a8a84u);	/* outrigger poles */
		L(b0 + 6, by, b0 - 6, by - 12, 0x8a8a84u);
	}
	for (i = 0; i < 3; i++) {				/* pelicans, gliding with a slow flap */
		int pxx = wrap(T * (i + 2) / 2 + i * 120, CV_W + 40) - 20;
		int pyy = y0 + 20 + i * 6 + RD(2LL * sn(T + i * 30, 60), 1024);
		int up = ((T / 5 + i) & 1) ? 3 : -2;

		L(pxx - 5, pyy + up, pxx, pyy, 0x4a4a52u);
		L(pxx + 5, pyy + up, pxx, pyy, 0x4a4a52u);
		R(pxx - 1, pyy, 3, 2, 0xe8e0d0u);
		R(pxx + 2, pyy + 1, 3, 1, 0xe8a83au);
	}
	R(0, gy - 4, CV_W, 4, 0xf6efd8u);
	for (x = 3; x < CV_W; x += 11)
		P(x, gy - 2, 0xd8c8a0u);
}

/* ---------------------------------------------------------------- Tijuana */

static void tij(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x6a2f6eu, 0xc04a6au, 0xf08a52u, 0xfcc86au};
	static const uint32_t far[6] = {0x9a5a6au, 0xb87a72u, 0x8a6a8au, 0xaa8a62u, 0x9a6a52u, 0x7a8a8au};
	static const uint32_t nr[8] = {0xe8545au, 0xf2c24au, 0x3fb3a8u, 0xf28ab0u, 0x6a8ae0u, 0xf2f0d8u, 0xa8d85au, 0xf08a3au};
	static const uint32_t car[5] = {0xe83a3au, 0xf2f2f0u, 0xf2c24au, 0x3a7ae0u, 0x3fc888u};
	long long T = t;
	int x, yy, k, cx = 210, cy = gy - 20;

	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(64, gy - 24, 11, 0xffe08au);
	for (x = 0; x < CV_W; x += 6) {			/* far hill, muted houses */
		int top = gy - 40 - RD(7LL * sn(x * 3 + 20, 330), 1024);

		for (yy = top; yy < gy - 10; yy += 5) {
			uint32_t col = far[art_below(r, 6)];

			R(x, yy + (x / 6 % 2) * 2, 6, 5, col);
			R(x, yy + (x / 6 % 2) * 2, 6, 1, 0x5a3a52u);
		}
	}
	for (x = 0; x < CV_W; x += 5) {			/* near hill, bright houses packed to the ridge */
		int top = gy - 30 - RD(9LL * sn(x * 4 + 90, 300) + 3LL * sn(x * 9, 100), 1024);

		for (yy = top; yy < gy - 9; yy += 5) {
			uint32_t col = nr[art_below(r, 8)];

			R(x, yy, 5, 5, col);
			R(x, yy, 5, 1, 0x4a2a3au);
			P(x + 2, yy + 3, 0x3a2438u);
			if (art_below(r, 100) < 22)
				P(x + 1, yy + 2, 0xffe08au);
		}
	}
	/* the monumental arch with a clock */
	R(cx - 15, cy - 16, 6, 16 + 12, 0xe6d2a8u);
	R(cx + 9, cy - 16, 6, 16 + 12, 0xe6d2a8u);
	R(cx + 12, cy - 16, 3, 28, 0xc4ac80u);
	for (k = 0; k <= 128; k++) {
		int rr;

		for (rr = 9; rr <= 15; rr++)
			P(cx + RD((long long)rr * icos(k), 1024), cy - RD((long long)rr * isin(k), 1024) - 1,
			  rr >= 14 ? 0xc4ac80u : 0xe6d2a8u);
	}
	R(cx - 7, cy - 30, 14, 10, 0xe6d2a8u);
	R(cx - 8, cy - 31, 16, 2, 0xc4ac80u);
	D(cx, cy - 25, 4, 0xfffbeau);
	L(cx, cy - 25, cx, cy - 28, 0x3a2438u);
	L(cx, cy - 25, cx + 2, cy - 24, 0x3a2438u);
	R(0, gy - 10, CV_W, 6, 0x3e3a46u);			/* the busy avenue */
	R(0, gy - 11, CV_W, 1, 0x8a7a6au);
	for (x = 4; x < CV_W; x += 18)
		R(x, gy - 7, 8, 1, 0xf2d86au);
	for (k = 0; k < 7; k++) {
		int bx = wrap(k * 53 + T * (k % 2 ? -2 : 2), CV_W + 20) - 10, ly = gy - (k % 2 ? 9 : 6);

		R(bx, ly, 8, 3, car[k % 5]);
		R(bx + 2, ly - 1, 4, 1, car[k % 5]);
		P(bx + 1, ly + 3, 0x161616u);
		P(bx + 6, ly + 3, 0x161616u);
	}
	for (x = 20; x < CV_W; x += 52) {
		R(x, gy - 22, 1, 12, 0x2a2430u);
		P(x, gy - 23, 0xffe08au);
	}
	R(0, gy - 4, CV_W, 4, 0x2e2a34u);
}

/* ---------------------------------------------------------------- Cozumel */

static void czm(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x2e9ae0u, 0x68c4f0u, 0xa0e0f8u, 0xd8f4fau};
	static const uint32_t sea[3] = {0x1ea8b8u, 0x4de0d0u, 0x9af2e4u};
	long long T = t;
	int x, i, k, hz = gy - 26, lx = 298, lb;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	D(40, y0 + 12, 6, 0xfffbe0u);
	D(150, y0 + 10, 5, 0xffffffu);
	D(158, y0 + 11, 6, 0xffffffu);
	D(167, y0 + 10, 4, 0xffffffu);
	cv_grad(c, 0, hz, CV_W, gy - 4 - hz, sea, 3);
	R(0, hz, CV_W, 1, 0xc8f8f0u);
	for (i = 0; i < 9; i++) {				/* reef patches */
		static const int rx[9] = {10, 40, 96, 120, 180, 204, 230, 20, 150};
		static const int ry[9] = {12, 16, 14, 18, 12, 18, 15, 19, 17};

		D(rx[i], hz + ry[i] - 6, 5, 0x1a8a9au);
		D(rx[i] + 4, hz + ry[i] - 5, 3, 0x2a9aaau);
		cv_checker(c, rx[i] - 6, hz + ry[i] - 9, 14, 6, 0x3ad0c0u);
	}
	for (k = 0; k < 24; k++)
		R(wrap(k * 29 + T, CV_W), hz + 1 + (k * 7) % 21, 3, 1, 0xd8fcf6u);
	/* the cruise ship */
	R(52, gy - 14, 134, 8, 0x1f2f5au);
	R(52, gy - 14, 134, 1, 0xd8d8e0u);
	TRI(186, gy - 14, 186, gy - 6, 196, gy - 14, 0x1f2f5au);
	R(60, gy - 22, 116, 8, 0xf4f4f6u);
	R(72, gy - 29, 90, 7, 0xf4f4f6u);
	R(88, gy - 35, 54, 6, 0xf4f4f6u);
	for (x = 64; x < 172; x += 4)
		P(x, gy - 19, 0x35505cu);
	for (x = 76; x < 158; x += 4)
		P(x, gy - 26, 0x35505cu);
	for (x = 92; x < 138; x += 4)
		P(x, gy - 32, 0x35505cu);
	for (x = 56; x < 182; x += 4)
		P(x, gy - 10, 0xe8e8f0u);
	R(120, gy - 44, 8, 9, 0x3a6ac0u);			/* the funnel */
	R(120, gy - 44, 8, 2, 0x222a40u);
	R(122, gy - 40, 4, 1, 0xf4f4f6u);
	R(190, gy - 8, 56, 3, 0x7a5a3au);			/* the pier */
	for (x = 192; x < 246; x += 8)
		R(x, gy - 8, 2, 5, 0x4a3a2au);
	R(238, gy - 8, 82, 4, 0xf2e2b8u);			/* the point and the beach */
	TRI(230, gy - 4, 250, gy - 4, 250, gy - 10, 0xf2e2b8u);
	R(0, gy - 8, 52, 1, 0xf4f4f6u);
	art_palm(c, 262, gy - 4, 20, 0x6a4a2au, 0x2e8a4au);
	art_palm(c, 276, gy - 4, 14, 0x6a4a2au, 0x2e8a4au);
	art_palm(c, 232, gy - 4, 16, 0x6a4a2au, 0x2e8a4au);
	for (k = 0; k < 7; k++) {				/* the striped lighthouse */
		int yy = gy - 6 - k * 5, w = 6 - k / 3;

		R(lx - w / 2, yy - 5, w, 5, k % 2 ? 0xf4f4f0u : 0xd8483au);
	}
	lb = gy - 6 - 35;
	R(lx - 3, lb - 2, 6, 2, 0x3a3a44u);
	R(lx - 2, lb - 5, 4, 3, (T / 8) % 2 == 0 ? 0xfff6b0u : 0xc8b868u);
	TRI(lx - 3, lb - 5, lx + 3, lb - 5, lx, lb - 9, 0xd8483au);
	if ((T / 8) % 2 == 0) {
		L(lx - 3, lb - 4, lx - 40, lb - 10, 0xfff6b0u);
		L(lx - 3, lb - 4, lx - 40, lb - 2, 0xfff6b0u);
	}
	R(0, gy - 4, CV_W, 4, 0xf2e2b8u);
	for (x = 3; x < CV_W; x += 9)
		P(x, gy - 2, 0xd8c8a0u);
}

/* ---------------------------------------------------------------- Los Cabos */

static void sjd(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x103a8au, 0x1f66c0u, 0x4a9ae0u, 0x9cd0f0u};
	static const uint32_t sea[3] = {0x0f5aa0u, 0x1a8ac8u, 0x4ec4e0u};
	long long T = t;
	int x, i, k, hz = gy - 22, cxa = 270, ra = 11;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	for (x = 0; x < CV_W; x++) {				/* two ranges of desert mountains */
		int h1 = hz - 12 - RD(8LL * sn(x * 2 + 10, 360) + 5LL * sn(x * 3 + 80, 150), 1024);
		int h2 = hz - 4 - RD(4LL * sn(x * 3 + 130, 260) + 2LL * sn(x * 5, 90), 1024);

		R(x, h1, 1, hz - h1, 0xd09a78u);
		if (x % 2 == 0)
			P(x, h1 + 3 + (x % 5), 0xb2704au);
		R(x, h2, 1, hz - h2, 0xb2704au);
		if (x % 3 == 0)
			P(x, h2 + 2 + (x % 4), 0x8a5238u);
	}
	cv_grad(c, 0, hz, CV_W, gy - 4 - hz, sea, 3);
	R(0, hz, CV_W, 1, 0x8ad8f0u);
	for (k = 0; k < 24; k++)
		R(wrap(k * 31 + T, CV_W), hz + 2 + (k * 5) % 18, 3, 1, 0xa8ecf8u);
	/* the cliffs of land's end, with a rock arch standing off the point */
	for (x = 230; x < 306; x++) {
		int top = gy - 34 + RD(3LL * sn(x * 3, 60) + 2LL * sn(x * 7, 30), 1024)
			  + (x < 252 ? (252 - x) * 3 / 2 : 0) + (x > 290 ? (x - 290) * 2 : 0);
		int dx = x - cxa, under = gy - 4, y;
		uint32_t base = x > cxa + 3 ? 0x6a5a4eu : 0x8a7a68u;

		if (dx > -ra && dx < ra)
			under = (gy - 8) - isqrt(ra * ra - dx * dx);
		if (under > top) {
			R(x, top, 1, under - top, base);
			R(x, top, 1, 1, 0xb4a48eu);
			for (y = top + 2; y < under; y += 3)
				if (((x * 7 + y * 5) % 11) < 3)
					P(x, y, x > cxa + 3 ? 0x54463cu : 0x6e5e50u);
		}
	}
	for (x = 232; x < 304; x += 3)
		R(x, gy - 7 + (x % 2), 2, 1, 0xf0fbffu);
	if ((T / 6) % 2 == 0)
		R(cxa - 4, gy - 6, 8, 1, 0xe0f6fcu);
	/* the marina */
	R(0, gy - 8, 220, 1, 0xd8c8a0u);
	for (x = 6; x < 212; x += 14)
		R(x, gy - 8, 2, 4, 0x6a5a4au);
	for (i = 0; i < 5; i++) {
		static const int bx[5] = {16, 54, 98, 140, 182};
		int bob = RD(sn(T + i * 33, 70), 1024), b0 = bx[i], by = gy - 12 + bob;

		R(b0, by, 22, 3, 0xf6f6f2u);
		R(b0, by + 2, 22, 1, 0x2a4a8au);
		TRI(b0 + 20, by, b0 + 26, by, b0 + 20, by + 3, 0xf6f6f2u);
		if (i % 2 == 0) {
			R(b0 + 6, by - 4, 9, 4, 0xe8e8e0u);
			R(b0 + 8, by - 3, 5, 1, 0x35505cu);
		} else {
			L(b0 + 8, by, b0 + 8, by - 16, 0xe8e0d0u);
			TRI(b0 + 9, by - 15, b0 + 9, by - 1, b0 + 20, by - 1, 0xf6f2e8u);
		}
	}
	for (i = 0; i < 2; i++) {				/* saguaro cactus on the shore */
		int cx0 = i ? 226 : 6;

		R(cx0, gy - 20, 3, 16, 0x2e5a3au);
		R(cx0 - 4, gy - 16, 2, 6, 0x2e5a3au);
		R(cx0 - 4, gy - 11, 5, 2, 0x2e5a3au);
		R(cx0 + 5, gy - 14, 2, 6, 0x2e5a3au);
		R(cx0 + 3, gy - 9, 4, 2, 0x2e5a3au);
	}
	R(0, gy - 4, CV_W, 4, 0xd8c28eu);
	for (x = 4; x < CV_W; x += 10)
		P(x, gy - 2, 0xb89e6au);
}

/* ---------------------------------------------------------------- Sinaloa */

static void snl(Canvas *c, int y0, int gy, ArtRng *r, uint32_t t)
{
	static const uint32_t sky[4] = {0x3a2a7au, 0xb04a8au, 0xf0704au, 0xffb050u};
	static const uint32_t sea[3] = {0xe0804au, 0xa8486au, 0x4a2a6au};
	static const uint32_t taxi[4] = {0xf4f0e0u, 0x3fb3a8u, 0xf2c24au, 0xe8545au};
	long long T = t;
	int x, i, k, hz = gy - 28;

	(void)r;
	cv_grad(c, 0, y0, CV_W, gy - y0 - 4, sky, 4);
	cv_grad(c, 0, hz, CV_W, gy - 4 - hz, sea, 3);
	R(0, hz, CV_W, 1, 0xffd080u);
	D(84, hz, 14, 0xffe090u);				/* the sun sets into the sea */
	R(60, hz, 50, 14, 0xe0804au);
	for (k = 0; k < 12; k++) {				/* its glitter on the water */
		int sw = 10 - k / 2, sx0 = 84 - sw / 2 + RD(3LL * sn(T * 3 + k * 40, 90), 1024);

		R(sx0, hz + 1 + k, sw, 1, 0xffd080u);
	}
	for (k = 0; k < 6; k++) {				/* breaking waves rolling in to the wall */
		int p = (int)((T / 2 + k * 5) % 30), yy = hz + 3 + RD(p * 11LL, 30), len = 8 + p / 2;
		int sx0 = wrap(k * 61 + p * 2, 230);

		R(sx0, yy, len, 1, 0xffe8e0u);
		R(sx0 + 2, yy + 1, len - 4, 1, 0xf0b8a0u);
	}
	/* the old town and the cathedral's twin towers */
	for (x = 190; x < CV_W; x += 22) {
		static const uint32_t hc[4] = {0xe8c070u, 0xd8808au, 0x7fb3c8u, 0xe0a060u};
		int h = 14 + (x / 22) % 3 * 4;

		R(x, gy - 12 - h, 22, h, hc[(x / 22) % 4]);
		for (k = x + 4; k < x + 20; k += 7)
			R(k, gy - 12 - h + 4, 3, 5, 0x3a2438u);
	}
	R(244, gy - 36, 44, 24, 0xe0b46au);			/* the nave */
	R(236, gy - 52, 11, 40, 0xe0b46au);
	R(285, gy - 52, 11, 40, 0xe0b46au);
	R(244, gy - 36, 44, 2, 0xb07a48u);
	R(243, gy - 52, 4, 40, 0xb07a48u);
	TRI(235, gy - 52, 248, gy - 52, 241, gy - 66, 0x4a8a8au);
	TRI(284, gy - 52, 297, gy - 52, 290, gy - 66, 0x4a8a8au);
	R(240, gy - 49, 3, 6, 0x3a2438u);
	R(289, gy - 49, 3, 6, 0x3a2438u);
	D(266, gy - 38, 8, 0xe0b46au);
	D(266, gy - 38, 6, 0xf0c87au);
	R(265, gy - 48, 2, 4, 0xb07a48u);
	R(262, gy - 30, 8, 14, 0x3a2438u);
	D(266, gy - 30, 4, 0x3a2438u);
	R(0, gy - 14, CV_W, 2, 0xe8d8c0u);			/* the seawall, balustrade and promenade */
	for (x = 0; x < CV_W; x += 4)
		R(x, gy - 13, 2, 3, 0xc8b8a0u);
	R(0, gy - 10, CV_W, 6, 0x6a4a5au);
	R(0, gy - 10, CV_W, 1, 0x8a6a72u);
	for (x = 12; x < 190; x += 60)
		art_palm(c, x, gy - 4, 22, 0x3a2438u, 0x24442eu);
	for (i = 0; i < 4; i++) {				/* open-air taxis, rolling both ways */
		int dir = i % 2 ? -1 : 1, bx = wrap(i * 83 + dir * T * 2, CV_W + 30) - 15;
		int ly = gy - (i % 2 ? 7 : 5);

		R(bx, ly, 10, 3, taxi[i]);
		R(bx - 1, ly - 4, 12, 1, taxi[(i + 1) % 4]);
		P(bx + 1, ly - 3, 0x3a2438u);
		P(bx + 8, ly - 3, 0x3a2438u);
		P(bx + 2, ly + 3, 0x161616u);
		P(bx + 7, ly + 3, 0x161616u);
	}
	R(0, gy - 4, CV_W, 4, 0x4a3040u);
}

void art_backdrop2(Canvas *c, int idx, int y0, int gy, ArtRng *r, uint32_t t)
{
	static void (*const bd[11])(Canvas *, int, int, ArtRng *, uint32_t) = {
		dal, sfo, lax, sea1, cvg, dtn, dst, tij, czm, sjd, snl};

	if (idx >= 0 && idx < 11)
		bd[idx](c, y0, gy, r, t);
}
