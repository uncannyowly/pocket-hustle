#include "check.h"
#include "tests.h"
#include "gfx/canvas.h"
#include "gfx/font.h"

#include <string.h>

static Canvas cv, cv2;

#define X 0xabcdefu
#define O 0x123456u

static int glyph_pixels(int code)
{
	int j, i, n = 0;

	for (j = 0; j < 8; j++)
		for (i = 0; i < 8; i++)
			if (FONT8[code - 0x20][j] & (0x80 >> i))
				n++;
	return n;
}

static void test_font_covers_ascii(void)
{
	int c, bad = 0;

	CHECK_EQ(glyph_pixels(0x20), 0);
	for (c = 0x21; c <= 0x7e; c++)
		if (glyph_pixels(c) == 0)
			bad++;
	CHECK_EQ(bad, 0);
}

static void test_font_distinct(void)
{
	int a, b, same = 0;

	for (a = 0x20; a <= 0x7e; a++)
		for (b = a + 1; b <= 0x7e; b++)
			if (memcmp(FONT8[a - 0x20], FONT8[b - 0x20], 8) == 0)
				same++;
	CHECK_EQ(same, 0);
}

static void test_tx_width(void)
{
	CHECK_EQ(tx_width("Hello", 2), 80);
	CHECK_EQ(tx_width("", 3), 0);
	CHECK_EQ(tx_width("AB", 1), 16);
}

static void bounds(int *minx, int *maxx)
{
	int x, y;

	*minx = 9999;
	*maxx = -1;
	for (y = 0; y < CV_H; y++)
		for (x = 0; x < CV_W; x++)
			if (cv.px[y * CV_W + x] == X) {
				if (x < *minx)
					*minx = x;
				if (x > *maxx)
					*maxx = x;
			}
}

static void test_tx_align(void)
{
	int lo, hi, w;

	cv_clear(&cv, 0);
	w = tx(&cv, "AB", 100, 50, X, 1, TX_RIGHT);
	CHECK_EQ(w, 16);
	bounds(&lo, &hi);
	CHECK(hi < 100);
	CHECK(hi >= 84 && lo >= 84);

	cv_clear(&cv, 0);
	tx(&cv, "AB", 160, 50, X, 1, TX_CENTER);
	bounds(&lo, &hi);
	CHECK(lo < 160);
	CHECK(hi >= 160);

	cv_clear(&cv, 0);
	tx(&cv, "AB", 20, 50, X, 1, TX_LEFT);
	bounds(&lo, &hi);
	CHECK(lo >= 20 && hi < 36);
}

static void test_tx_unknown_bytes(void)
{
	cv_clear(&cv, 0);
	cv_clear(&cv2, 0);
	tx(&cv2, "?", 10, 10, X, 1, TX_LEFT);
	tx(&cv, "\x01", 10, 10, X, 1, TX_LEFT);
	CHECK(memcmp(cv.px, cv2.px, sizeof cv.px) == 0);
	cv_clear(&cv, 0);
	tx(&cv, "\xe9", 10, 10, X, 1, TX_LEFT);
	CHECK(memcmp(cv.px, cv2.px, sizeof cv.px) == 0);
	CHECK_EQ(tx_width("\xe9", 1), 8);
}

static void test_overflow_counted(void)
{
	cv_clear(&cv, 0);
	tx(&cv, "HELLO", 300, 10, X, 1, TX_LEFT);
	CHECK(cv.overflow > 0);

	cv_clear(&cv, 0);
	tx(&cv, "HELLO", 10, 10, X, 1, TX_LEFT);
	CHECK_EQ(cv.overflow, 0);

	cv_clear(&cv, 0);
	cv_clip(&cv, 0, 0, 12, 100);
	tx(&cv, "HELLO", 10, 10, X, 1, TX_LEFT);
	CHECK_EQ(cv.overflow, 0);
	cv_noclip(&cv);

	cv_clear(&cv, 0);
	tx(&cv, "A", 10, -3, X, 1, TX_LEFT);
	CHECK_EQ(cv.overflow, 1);

	cv_clear(&cv, 0);
	tx_outline(&cv, "A", 316, 10, X, O, 1, TX_LEFT);
	CHECK_EQ(cv.overflow, 1);
}

static void test_wrap(void)
{
	char out[8][TX_LINE_MAX];
	int n;

	n = tx_wrap("Buy low. Sell high. Don't get caught.", 16, out, 8);
	CHECK_EQ(n, 3);
	CHECK_STR(out[0], "Buy low. Sell");
	CHECK_STR(out[1], "high. Don't get");
	CHECK_STR(out[2], "caught.");

	n = tx_wrap("abcdefghijklmnopqrst", 8, out, 8);
	CHECK_EQ(n, 3);
	CHECK_STR(out[0], "abcdefgh");
	CHECK_STR(out[1], "ijklmnop");
	CHECK_STR(out[2], "qrst");

	n = tx_wrap("Buy low. Sell high. Don't get caught.", 16, out, 2);
	CHECK_EQ(n, 2);
	CHECK_STR(out[1], "high. Don't get");

	n = tx_wrap("", 16, out, 8);
	CHECK_EQ(n, 0);
	n = tx_wrap("hi", 16, out, 0);
	CHECK_EQ(n, 0);
}

static void test_outline(void)
{
	int x, y, dx, dy, bad_main = 0, missing = 0, stray = 0, glyph_n = 0;

	cv_clear(&cv, 0);
	cv_clear(&cv2, 0);
	tx(&cv2, "Hi", 20, 20, X, 1, TX_LEFT);
	tx_outline(&cv, "Hi", 20, 20, X, O, 1, TX_LEFT);
	for (y = 1; y < CV_H - 1; y++)
		for (x = 1; x < CV_W - 1; x++) {
			int g = cv2.px[y * CV_W + x] == X, near = 0;

			for (dy = -1; dy <= 1; dy++)
				for (dx = -1; dx <= 1; dx++)
					if (cv2.px[(y + dy) * CV_W + x + dx] == X)
						near = 1;
			if (g) {
				glyph_n++;
				if (cv.px[y * CV_W + x] != X)
					bad_main = 1;
			} else if (near && cv.px[y * CV_W + x] != O) {
				missing++;
			} else if (!near && cv.px[y * CV_W + x] != 0) {
				stray++;
			}
		}
	CHECK(glyph_n > 0);
	CHECK_EQ(bad_main, 0);
	CHECK_EQ(missing, 0);
	CHECK_EQ(stray, 0);
	CHECK_EQ(tx_outline(&cv, "Hi", 20, 20, X, O, 2, TX_LEFT), 32);
}

void tests_font(void)
{
	test_font_covers_ascii();
	test_font_distinct();
	test_tx_width();
	test_tx_align();
	test_tx_unknown_bytes();
	test_overflow_counted();
	test_wrap();
	test_outline();
}
