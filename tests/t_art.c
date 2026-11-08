#include "check.h"
#include "tests.h"
#include "gfx/canvas.h"
#include "gfx/art.h"

#include <string.h>

static Canvas cv;

#define SENT 0x13579bu

static uint32_t hash_rows(int y0, int h)
{
	uint32_t hv = 2166136261u;
	int i;

	for (i = y0 * CV_W; i < (y0 + h) * CV_W; i++) {
		hv ^= cv.px[i];
		hv *= 16777619u;
	}
	return hv;
}

static uint32_t city_hash(int city, int y0, int h, uint32_t t)
{
	cv_clear(&cv, SENT);
	art_backdrop(&cv, city, y0, h, t);
	return hash_rows(y0, h);
}

static void test_backdrop_stays_in_band(void)
{
	static const int bands[2][2] = {{12, 56}, {0, 240}};
	int city, b, i, outside;

	for (city = 0; city < 18; city++)
		for (b = 0; b < 2; b++) {
			int y0 = bands[b][0], h = bands[b][1];

			cv_clear(&cv, SENT);
			art_backdrop(&cv, city, y0, h, 40);
			outside = 0;
			for (i = 0; i < CV_W * CV_H; i++)
				if ((i / CV_W < y0 || i / CV_W >= y0 + h) && cv.px[i] != SENT)
					outside++;
			CHECK_EQ(outside, 0);
		}
}

static void test_backdrop_fills_band(void)
{
	static const int bands[2][2] = {{12, 56}, {0, 240}};
	int city, b, i, left;

	for (city = 0; city < 18; city++)
		for (b = 0; b < 2; b++) {
			int y0 = bands[b][0], h = bands[b][1];

			cv_clear(&cv, SENT);
			art_backdrop(&cv, city, y0, h, 0);
			left = 0;
			for (i = y0 * CV_W; i < (y0 + h) * CV_W; i++)
				if (cv.px[i] == SENT)
					left++;
			CHECK_EQ(left, 0);
		}
}

static void test_backdrops_differ(void)
{
	uint32_t h[18], f[18];
	int a, b, same = 0;

	for (a = 0; a < 18; a++) {
		h[a] = city_hash(a, 12, 56, 0);
		f[a] = city_hash(a, 0, 240, 0);
	}
	for (a = 0; a < 18; a++)
		for (b = a + 1; b < 18; b++)
			if (h[a] == h[b] || f[a] == f[b])
				same++;
	CHECK_EQ(same, 0);
}

static void test_backdrop_deterministic(void)
{
	int city;

	for (city = 0; city < 18; city++)
		CHECK(city_hash(city, 12, 56, 33) == city_hash(city, 12, 56, 33));
}

static void test_backdrops_animate(void)
{
	/* OKC (turbines), Houston (traffic), San Jose (train) and Portland (rain).  The mockup's
	 * train enters from off screen and is first visible at t = 10, so San Jose compares t = 30.
	 * Every new city moves too (water, traffic, a ferry, a biplane...), compared at t = 30. */
	int i, city;

	for (i = 0; i < 4; i++) {
		static const int cities[4] = {0, 1, 2, 6};
		static const uint32_t later[4] = {7, 7, 30, 7};

		CHECK(city_hash(cities[i], 12, 56, 0) != city_hash(cities[i], 12, 56, later[i]));
	}
	for (city = 7; city < 18; city++) {
		CHECK(city_hash(city, 12, 56, 0) != city_hash(city, 12, 56, 30));
		CHECK(city_hash(city, 0, 240, 0) != city_hash(city, 0, 240, 30));
	}
}

static int set_count(const Canvas *c)
{
	int i, n = 0;

	for (i = 0; i < CV_W * CV_H; i++)
		if (c->px[i] != SENT)
			n++;
	return n;
}

static void test_portraits(void)
{
	uint32_t h[18];
	int city, a, b, i, outside, same = 0;

	for (city = 0; city < 18; city++) {
		cv_clear(&cv, SENT);
		art_portrait(&cv, city, 100, 80);
		outside = 0;
		for (i = 0; i < CV_W * CV_H; i++) {
			int x = i % CV_W, y = i / CV_W;

			if ((x < 100 || x >= 156 || y < 80 || y >= 136) && cv.px[i] != SENT)
				outside++;
		}
		CHECK_EQ(outside, 0);
		CHECK(set_count(&cv) == 56 * 56);	/* the whole box is painted */
		h[city] = hash_rows(80, 56);
	}
	for (a = 0; a < 18; a++)
		for (b = a + 1; b < 18; b++)
			if (h[a] == h[b])
				same++;
	CHECK_EQ(same, 0);
}

static void test_cop_boss_differs(void)
{
	uint32_t hb, hp;
	int i, minx = 999, maxx = -1, miny = 999, maxy = -1;

	cv_clear(&cv, SENT);
	art_cop(&cv, 50, 200, 2, 1);
	hb = hash_rows(0, 240);
	for (i = 0; i < CV_W * CV_H; i++)
		if (cv.px[i] != SENT) {
			int x = i % CV_W, y = i / CV_W;

			if (x < minx) minx = x;
			if (x > maxx) maxx = x;
			if (y < miny) miny = y;
			if (y > maxy) maxy = y;
		}
	CHECK(minx >= 50 && maxx < 90);
	CHECK(miny >= 144 && maxy <= 199);
	CHECK_EQ(maxy, 199);	/* feet on the base line */
	cv_clear(&cv, SENT);
	art_cop(&cv, 50, 200, 2, 0);
	hp = hash_rows(0, 240);
	CHECK(hb != hp);
}

static void test_icons_nonempty(void)
{
	int id;

	for (id = 0; id < NUM_ICONS; id++) {
		int n1, n2;

		cv_clear(&cv, SENT);
		art_icon(&cv, (IconId)id, 20, 20, 1, 0xffffffu);
		n1 = set_count(&cv);
		cv_clear(&cv, SENT);
		art_icon(&cv, (IconId)id, 20, 20, 2, 0xffffffu);
		n2 = set_count(&cv);
		CHECK(n1 > 0);
		CHECK_EQ(n2, n1 * 4);
	}
}

static void test_scenes_draw(void)
{
	cv_clear(&cv, SENT);
	art_vault(&cv, 160, 120, 0);
	CHECK(set_count(&cv) > 3000);
	CHECK(cv.px[120 * CV_W + 160] != SENT);
	cv_clear(&cv, SENT);
	art_chase_scene(&cv, 12, 3, 0);
	CHECK(set_count(&cv) == CV_W * 96);
	CHECK(cv.px[11 * CV_W + 5] == SENT && cv.px[108 * CV_W + 5] == SENT);
}

void tests_art(void)
{
	test_backdrop_stays_in_band();
	test_backdrop_fills_band();
	test_backdrops_differ();
	test_backdrop_deterministic();
	test_backdrops_animate();
	test_portraits();
	test_cop_boss_differs();
	test_icons_nonempty();
	test_scenes_draw();
}
