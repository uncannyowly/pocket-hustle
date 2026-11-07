#include "check.h"
#include "tests.h"
#include "gfx/canvas.h"

#include <string.h>

static Canvas cv;
static uint32_t big[640 * 480];

#define X 0xabcdefu

static int count_set(void)
{
	int i, n = 0;

	for (i = 0; i < CV_W * CV_H; i++)
		if (cv.px[i] == X)
			n++;
	return n;
}

static int at(int x, int y)
{
	return cv.px[y * CV_W + x] == X;
}

static void test_rect_clips_to_canvas(void)
{
	int x, y, only_corner = 1;

	cv_clear(&cv, 0);
	cv_rect(&cv, -50, -50, 60, 60, X);
	CHECK_EQ(count_set(), 100);
	for (y = 0; y < CV_H; y++)
		for (x = 0; x < CV_W; x++)
			if (at(x, y) != (x < 10 && y < 10))
				only_corner = 0;
	CHECK(only_corner);

	cv_clear(&cv, 0);
	cv_rect(&cv, 315, 235, 100, 100, X);
	CHECK_EQ(count_set(), 25);
	CHECK(at(319, 239) && at(315, 235) && !at(314, 235));

	cv_clear(&cv, 0);
	cv_rect(&cv, 10, 10, -1000000, 20, X);
	cv_rect(&cv, 10, 10, 20, -1000000, X);
	cv_rect(&cv, -2000000000, 0, -2000000000, 5, X);
	CHECK_EQ(count_set(), 0);
}

static void test_clip_window(void)
{
	cv_clear(&cv, 0);
	cv_clip(&cv, 10, 10, 20, 20);
	cv_rect(&cv, 0, 0, CV_W, CV_H, X);
	CHECK_EQ(count_set(), 100);
	CHECK(at(10, 10) && at(19, 19) && !at(20, 19) && !at(9, 10));
	cv_px(&cv, 5, 5, X);
	CHECK(!at(5, 5));
	cv_noclip(&cv);
	cv_rect(&cv, 0, 0, CV_W, CV_H, X);
	CHECK_EQ(count_set(), CV_W * CV_H);

	cv_clear(&cv, 0);
	cv_clip(&cv, -5, -5, 1000, 1000);
	CHECK_EQ(cv.cx0, 0);
	CHECK_EQ(cv.cx1, CV_W);
	CHECK_EQ(cv.cy1, CV_H);
	cv_clip(&cv, 10, 10, 20, 20);
	cv_clear(&cv, 0);
	CHECK_EQ(cv.cx0, 0);
	CHECK_EQ(cv.cy1, CV_H);
}

static void test_disc_and_ring(void)
{
	int x, y, sym = 1;

	cv_clear(&cv, 0);
	cv_disc(&cv, 50, 50, 5, X);
	for (y = 40; y <= 60; y++)
		for (x = 40; x <= 60; x++)
			if (at(x, y) != at(100 - x, y) || at(x, y) != at(x, 100 - y))
				sym = 0;
	CHECK(sym);
	CHECK(at(50, 45) && at(55, 50) && !at(56, 50) && at(50, 50));

	cv_clear(&cv, 0);
	cv_ring(&cv, 50, 50, 5, X);
	CHECK(at(50, 45) && at(50, 55) && at(45, 50) && at(55, 50));
	CHECK(!at(50, 50));
}

static void test_tri_fills(void)
{
	int x, y, outside = 0;

	cv_clear(&cv, 0);
	cv_tri(&cv, 20, 20, 60, 30, 30, 70, X);
	CHECK(at((20 + 60 + 30) / 3, (20 + 30 + 70) / 3));
	for (y = 0; y < CV_H; y++)
		for (x = 0; x < CV_W; x++)
			if (at(x, y) && (x < 20 || x > 60 || y < 20 || y > 70))
				outside++;
	CHECK_EQ(outside, 0);
	CHECK(count_set() > 500);

	/* winding order must not matter */
	{
		static uint32_t first[CV_W * CV_H];

		memcpy(first, cv.px, sizeof first);
		cv_clear(&cv, 0);
		cv_tri(&cv, 30, 70, 60, 30, 20, 20, X);
		CHECK(memcmp(first, cv.px, sizeof first) == 0);
	}
}

static void test_grad_endpoints(void)
{
	static const uint32_t cols[4] = { 0x111111, 0x222222, 0x333333, 0x444444 };
	int x, y, first_ok = 1, last_ok = 1, mid_mixed = 0;

	cv_clear(&cv, 0);
	cv_grad(&cv, 0, 0, 40, 40, cols, 4);
	for (x = 0; x < 40; x++) {
		if (cv.px[x] != cols[0])
			first_ok = 0;
		if (cv.px[39 * CV_W + x] != cols[3])
			last_ok = 0;
	}
	CHECK(first_ok);
	CHECK(last_ok);
	/* between two bands the Bayer pattern mixes both colors */
	for (y = 12; y <= 14; y++)
		for (x = 0; x < 4; x++)
			if (cv.px[y * CV_W + x] == cols[0] || cv.px[y * CV_W + x] == cols[1])
				mid_mixed = 1;
	CHECK(mid_mixed);
}

static void test_dim_halves(void)
{
	cv_clear(&cv, 0xff8041);
	cv_dim(&cv);
	CHECK_EQ(cv.px[0], 0x7f4020u);
	CHECK_EQ(cv.px[CV_W * CV_H - 1], 0x7f4020u);
}

static void test_scale2x(void)
{
	cv_clear(&cv, 0);
	cv.px[4 * CV_W + 3] = X;
	memset(big, 0, sizeof big);
	cv_scale2x(&cv, big, 640);
	CHECK_EQ(big[8 * 640 + 6], X);
	CHECK_EQ(big[8 * 640 + 7], X);
	CHECK_EQ(big[9 * 640 + 6], X);
	CHECK_EQ(big[9 * 640 + 7], X);
	CHECK_EQ(big[8 * 640 + 8], 0u);
	CHECK_EQ(big[10 * 640 + 6], 0u);
	CHECK_EQ(big[8 * 640 + 5], 0u);
}

static void test_line_and_checker(void)
{
	int x;

	cv_clear(&cv, 0);
	cv_line(&cv, 10, 10, 30, 10, X);
	CHECK_EQ(count_set(), 21);
	cv_clear(&cv, 0);
	cv_line(&cv, 0, 0, 20, 20, X);
	CHECK(at(0, 0) && at(20, 20) && at(7, 7));
	CHECK_EQ(count_set(), 21);
	cv_clear(&cv, 0);
	cv_box(&cv, 5, 5, 10, 8, X);
	CHECK_EQ(count_set(), 2 * 10 + 2 * 6);
	cv_clear(&cv, 0);
	cv_checker(&cv, 0, 0, 8, 8, X);
	CHECK_EQ(count_set(), 32);
	for (x = 0; x < 7; x++)
		CHECK(at(x, 0) != at(x + 1, 0));
}

static void test_isin_table(void)
{
	int a, mono = 1, odd = 1;

	CHECK_EQ(isin(0), 0);
	CHECK_EQ(isin(64), 1024);
	CHECK_EQ(isin(128), 0);
	CHECK_EQ(isin(192), -1024);
	CHECK_EQ(icos(0), 1024);
	CHECK_EQ(icos(64), 0);
	CHECK_EQ(icos(128), -1024);
	CHECK_EQ(isin(-64), -1024);
	CHECK_EQ(isin(320), 1024);
	CHECK_EQ(isin(32), 724);
	for (a = 0; a < 64; a++)
		if (isin(a) > isin(a + 1))
			mono = 0;
	CHECK(mono);
	for (a = -300; a < 300; a++)
		if (isin(-a) != -isin(a) || isin(a) < -1024 || isin(a) > 1024)
			odd = 0;
	CHECK(odd);
}

static void test_isqrt(void)
{
	int v, exact = 1;

	CHECK_EQ(isqrt(0), 0);
	CHECK_EQ(isqrt(1), 1);
	CHECK_EQ(isqrt(15), 3);
	CHECK_EQ(isqrt(16), 4);
	CHECK_EQ(isqrt(1000000), 1000);
	CHECK_EQ(isqrt(2147483647), 46340);
	for (v = 0; v < 5000; v++) {
		int r = isqrt(v);

		if (r * r > v || (r + 1) * (r + 1) <= v)
			exact = 0;
	}
	CHECK(exact);
}

static void test_art_rng_deterministic(void)
{
	ArtRng a, b;
	int i, same = 1, in_range = 1;

	art_seed(&a, 777);
	art_seed(&b, 777);
	for (i = 0; i < 100; i++)
		if (art_next(&a) != art_next(&b))
			same = 0;
	CHECK(same);
	for (i = 0; i < 1000; i++) {
		int v = art_below(&a, 7);

		if (v < 0 || v > 6)
			in_range = 0;
	}
	CHECK(in_range);
	/* mulberry32 reference values for seed 12345 */
	art_seed(&a, 12345);
	CHECK_EQ(art_next(&a), 0xfacf78c5u);
	CHECK_EQ(art_next(&a), 0x4e875100u);
}

void tests_gfx(void)
{
	test_rect_clips_to_canvas();
	test_clip_window();
	test_disc_and_ring();
	test_tri_fills();
	test_grad_endpoints();
	test_dim_halves();
	test_scale2x();
	test_line_and_checker();
	test_isin_table();
	test_isqrt();
	test_art_rng_deterministic();
}
