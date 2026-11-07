#include "check.h"
#include "tests.h"
#include "helpers.h"
#include "core/game_internal.h"

static int32_t biased(int32_t v, int city, int good)
{
	int64_t b = (int64_t)v * CITIES[city].bias_pct[good] / 100;
	return b < 1 ? 1 : (int32_t)b;
}

static void test_market_within_biased_range(void)
{
	Params p = test_params();
	Game g;
	uint64_t seed;
	int i, ok = 1;
	for (seed = 1; seed <= 200; seed++) {
		game_new(&g, &p, seed, 30, "t");
		for (i = 0; i < NUM_GOODS; i++) {
			int c = g.loc.city;
			if (g.price[i] < biased(GOODS[i].lo, c, i) || g.price[i] > biased(GOODS[i].hi, c, i))
				ok = 0;
		}
	}
	CHECK(ok);
}

static void test_market_min_goods(void)
{
	Params p = test_params();
	Game g;
	int i, n = 0;
	p.absent_permille = 1000;
	p.min_goods = 4;
	new_test_game(&g, &p, 30);
	for (i = 0; i < NUM_GOODS; i++)
		if (g.price[i] > 0) n++;
	CHECK_EQ(n, 4);
}

static void test_market_spike_doubles(void)
{
	Params p = test_params();
	Game g;
	int i, k, ok = 1;
	p.spike_permille = 1000;
	p.spike_lo_pct = 200;
	p.spike_hi_pct = 200;
	new_test_game(&g, &p, 30);
	for (i = 0; i < NUM_GOODS; i++) {
		int found = 0;
		if (g.price[i] < 2 * biased(GOODS[i].lo, g.loc.city, i))
			ok = 0;
		for (k = 0; k < g.nmsgs; k++)
			if (g.msgs[k].kind == M_SPIKE && g.msgs[k].a == i && g.msgs[k].amount == g.price[i])
				found++;
		CHECK_EQ(found, 1);
	}
	CHECK(ok);
	CHECK_EQ(count_msgs(&g, M_SPIKE), NUM_GOODS);
}

static void test_market_history(void)
{
	Params p = test_params();
	Game g;
	int i;
	new_test_game(&g, &p, 30);
	for (i = 0; i < NUM_GOODS; i++)
		CHECK_EQ(g.hist[i][g.day], g.price[i]);
}

static void test_market_deterministic(void)
{
	Params p = params_default();
	Game a, b;
	game_new(&a, &p, 5, 30, "a");
	game_new(&b, &p, 5, 30, "b");
	CHECK(memcmp(a.price, b.price, sizeof a.price) == 0);
}

static void test_flavor_independent(void)
{
	Params p = test_params();
	Game a, b;
	int i;
	game_new(&a, &p, 5, 30, "a");
	game_new(&b, &p, 5, 30, "a");
	for (i = 0; i < 7; i++)
		rng_u32(&b.flavor);
	market_roll(&a);
	market_roll(&b);
	CHECK(memcmp(a.price, b.price, sizeof a.price) == 0);
}

void tests_market(void)
{
	test_market_within_biased_range();
	test_market_min_goods();
	test_market_spike_doubles();
	test_market_history();
	test_market_deterministic();
	test_flavor_independent();
}
