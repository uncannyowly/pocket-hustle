#include "check.h"
#include "tests.h"
#include "core/rng.h"

/* pcg-c-basic pcg32-demo, seed 42, seq 54 */
static void test_rng_matches_pcg32_reference(void)
{
	Rng r;
	rng_seed(&r, 42, 54);
	CHECK_EQ(rng_u32(&r), 0xa15c02b7u);
	CHECK_EQ(rng_u32(&r), 0x7b47f409u);
	CHECK_EQ(rng_u32(&r), 0xba1d3330u);
	CHECK_EQ(rng_u32(&r), 0x83d2f293u);
}

static void test_rng_below_bounds(void)
{
	Rng r;
	int seen[7] = {0}, i, all_below = 1;
	rng_seed(&r, 1, 1);
	for (i = 0; i < 10000; i++) {
		uint32_t v = rng_below(&r, 7);
		if (v >= 7) all_below = 0; else seen[v] = 1;
	}
	CHECK(all_below);
	for (i = 0; i < 7; i++)
		CHECK(seen[i]);
}

static void test_rng_range_inclusive(void)
{
	Rng r;
	int i, inside = 1, lo_seen = 0, hi_seen = 0;
	rng_seed(&r, 2, 1);
	for (i = 0; i < 10000; i++) {
		int32_t v = rng_range(&r, -3, 3);
		if (v < -3 || v > 3) inside = 0;
		if (v == -3) lo_seen = 1;
		if (v == 3) hi_seen = 1;
	}
	CHECK(inside);
	CHECK(lo_seen);
	CHECK(hi_seen);
}

static void test_rng_permille_edges(void)
{
	Rng r;
	int i, never = 1, always = 1;
	rng_seed(&r, 3, 1);
	for (i = 0; i < 1000; i++) {
		if (rng_permille(&r, 0)) never = 0;
		if (!rng_permille(&r, 1000)) always = 0;
	}
	CHECK(never);
	CHECK(always);
}

static void test_rng_same_seed_same_stream(void)
{
	Rng a, b;
	int i, same = 1;
	rng_seed(&a, 9, 1);
	rng_seed(&b, 9, 1);
	for (i = 0; i < 100; i++)
		if (rng_u32(&a) != rng_u32(&b)) same = 0;
	CHECK(same);
}

void tests_rng(void)
{
	test_rng_matches_pcg32_reference();
	test_rng_below_bounds();
	test_rng_range_inclusive();
	test_rng_permille_edges();
	test_rng_same_seed_same_stream();
}
