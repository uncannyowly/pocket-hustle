/* The pure helpers behind the Miyoo backend: panel blit and MENU hold timing. */
#include "check.h"
#include "tests.h"
#include "gfx/panel.h"
#include "sfx/halve.h"
#include "ui/hold.h"
#include "ui/switcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void test_hold_quits_after_two_seconds(void)
{
	Hold h;
	hold_init(&h);
	CHECK_EQ(hold_step(&h, 1, 1000), 0);
	CHECK_EQ(hold_step(&h, 1, 2999), 0);
	CHECK_EQ(hold_step(&h, 1, 3000), 1);
}

static void test_hold_release_resets(void)
{
	Hold h;
	hold_init(&h);
	CHECK_EQ(hold_step(&h, 1, 0), 0);
	CHECK_EQ(hold_step(&h, 1, 1900), 0);
	CHECK_EQ(hold_step(&h, 0, 1950), 0);
	CHECK_EQ(hold_step(&h, 1, 2000), 0);
	CHECK_EQ(hold_step(&h, 1, 3999), 0);
	CHECK_EQ(hold_step(&h, 1, 4000), 1);
}

static void test_hold_survives_clock_start_at_zero(void)
{
	Hold h;
	hold_init(&h);
	CHECK_EQ(hold_step(&h, 1, 0), 0);
	CHECK_EQ(hold_step(&h, 1, 1999), 0);
	CHECK_EQ(hold_step(&h, 1, 2000), 1);
}

static void test_hold_wraps_with_the_clock(void)
{
	Hold h;
	hold_init(&h);
	CHECK_EQ(hold_step(&h, 1, 0xFFFFFF00u), 0);
	CHECK_EQ(hold_step(&h, 1, 0xFFFFFF00u + 1999u), 0);
	CHECK_EQ(hold_step(&h, 1, 0xFFFFFF00u + 2000u), 1);
}

/* 4 x 2 picture: pixel (x,y) = 10*y + x + 1 */
static void test_rot180_exact_size(void)
{
	uint32_t src[8], dst[8];
	int i;
	for (i = 0; i < 8; i++)
		src[i] = (uint32_t)(i + 1);
	panel_blit_rot180(dst, 4, 4, 2, src, 4, 2, 4);
	/* rotated: dst[0] is the last source pixel */
	for (i = 0; i < 8; i++)
		CHECK_EQ(dst[i], 0xFF000000u | (uint32_t)(8 - i));
}

static void test_rot180_centers_on_larger_panel(void)
{
	uint32_t src[4] = {1, 2, 3, 4}, dst[6 * 4];	/* 2 x 2 on 6 x 4 */
	int i, x, y;
	for (i = 0; i < 24; i++)
		dst[i] = 0x12345678u;
	panel_blit_rot180(dst, 6, 6, 4, src, 2, 2, 2);
	for (y = 0; y < 4; y++)
		for (x = 0; x < 6; x++) {
			uint32_t want = 0xFF000000u;
			if (x >= 2 && x < 4 && y >= 1 && y < 3)
				want |= (uint32_t)(4 - ((y - 1) * 2 + (x - 2)));
			CHECK_EQ(dst[y * 6 + x], want);
		}
}

static void test_rot180_odd_margin_is_symmetric(void)
{
	uint32_t src[1] = {7}, dst[5 * 1];	/* 1 x 1 on 5 x 1: centered at x = 2 */
	int x;
	panel_blit_rot180(dst, 5, 5, 1, src, 1, 1, 1);
	for (x = 0; x < 5; x++)
		CHECK_EQ(dst[x], x == 2 ? 0xFF000007u : 0xFF000000u);
}

static void test_rot180_pitch_and_crop(void)
{
	uint32_t src[3 * 3], dst[2 * 2 + 2];	/* 3 x 3 source on a 2 x 1 panel: crops the middle */
	int i;
	for (i = 0; i < 9; i++)
		src[i] = (uint32_t)(i + 1);
	for (i = 0; i < 6; i++)
		dst[i] = 0;
	panel_blit_rot180(dst, 3, 2, 1, src, 3, 3, 3);
	/* centered crop: source x 0..1 + (3-2)/2 = x 0.., row 1: pixels 4,5 (offset floor) rotated: 5,4 */
	CHECK_EQ(dst[0], 0xFF000005u);
	CHECK_EQ(dst[1], 0xFF000004u);
	CHECK_EQ(dst[2], 0u);	/* pitch padding untouched */
}

static void test_halve_averages_pairs_per_channel(void)
{
	int16_t src[8] = {10, 100, 20, 300, -10, -100, -30, -200}, dst[4];
	audio_halve(dst, src, 2);
	CHECK_EQ(dst[0], 15);
	CHECK_EQ(dst[1], 200);
	CHECK_EQ(dst[2], -20);
	CHECK_EQ(dst[3], -150);
}

static void test_halve_rounds_half_up(void)
{
	int16_t src[8] = {1, 0, 2, 1, -1, -2, -2, -1}, dst[4];
	audio_halve(dst, src, 2);
	CHECK_EQ(dst[0], 2);	/* 1.5 */
	CHECK_EQ(dst[1], 1);	/* 0.5 */
	CHECK_EQ(dst[2], -1);	/* -1.5 */
	CHECK_EQ(dst[3], -1);	/* -1.5 */
}

static void test_halve_never_overflows(void)
{
	int16_t src[8] = {32767, -32768, 32767, -32768, 32767, 32767, -32768, -32768}, dst[4];
	audio_halve(dst, src, 2);
	CHECK_EQ(dst[0], 32767);
	CHECK_EQ(dst[1], -32768);
	CHECK_EQ(dst[2], 0);
	CHECK_EQ(dst[3], 0);
}

/* Holding MENU on OnionOS leaves through its GameSwitcher: the game creates the flag file
 * OnionOS's own MENU handling sets for RetroArch. Without the folder (any other system)
 * nothing is created. */
static void test_switcher_flag(void)
{
	char dir[64], path[96], missing[128];
	FILE *f;

	CHECK_STR(SWITCHER_FLAG_ONION, "/mnt/SDCARD/.tmp_update/.runGameSwitcher");
	system("mkdir -p build");
	snprintf(dir, sizeof dir, "build/switcher-XXXXXX");
	if (!mkdtemp(dir)) {
		CHECK(0);
		return;
	}
	snprintf(path, sizeof path, "%s/.runGameSwitcher", dir);
	CHECK_EQ(switcher_request(path), 1);
	f = fopen(path, "r");
	CHECK(f != NULL);
	if (f)
		fclose(f);
	CHECK_EQ(switcher_request(path), 1);		/* already set: still fine */
	snprintf(missing, sizeof missing, "%s/no-such-dir/.runGameSwitcher", dir);
	CHECK_EQ(switcher_request(missing), 0);
	CHECK(fopen(missing, "r") == NULL);
	CHECK_EQ(switcher_request(""), 0);
	CHECK_EQ(switcher_request(NULL), 0);
	remove(path);
	rmdir(dir);
}

void tests_mini(void)
{
	test_halve_averages_pairs_per_channel();
	test_halve_rounds_half_up();
	test_halve_never_overflows();
	test_hold_quits_after_two_seconds();
	test_hold_release_resets();
	test_hold_survives_clock_start_at_zero();
	test_hold_wraps_with_the_clock();
	test_switcher_flag();
	test_rot180_exact_size();
	test_rot180_centers_on_larger_panel();
	test_rot180_odd_margin_is_symmetric();
	test_rot180_pitch_and_crop();
}
