#include "check.h"
#include "tests.h"
#include "helpers.h"

static void test_new_game_start_state(void)
{
	Params p = params_default();
	Game g;
	CHECK_EQ(game_new(&g, &p, 1, 30, "Seth"), OK);
	CHECK_EQ(g.day, 1);
	CHECK_EQ(g.last_day, 30);
	CHECK_EQ(g.loc.city, p.start_city);
	CHECK_EQ(g.loc.hood, 0);
	CHECK_EQ(g.cash, p.start_cash);
	CHECK_EQ(g.debt[p.start_city], p.start_debt);
	CHECK_EQ(g.health, 100);
	CHECK_EQ(g.weapon, -1);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(g.end, END_NONE);
	CHECK_STR(g.name, "Seth");
}

static void test_new_game_lengths(void)
{
	Params p = params_default();
	Game g;
	CHECK_EQ(game_new(&g, &p, 1, 15, "a"), OK);
	CHECK_EQ(game_new(&g, &p, 1, 30, "a"), OK);
	CHECK_EQ(game_new(&g, &p, 1, 60, "a"), OK);
	CHECK_EQ(game_new(&g, &p, 1, 90, "a"), OK);
	CHECK_EQ(game_new(&g, &p, 1, 0, "a"), E_BAD_ARG);
	CHECK_EQ(game_new(&g, &p, 1, 20, "a"), E_BAD_ARG);
	CHECK_EQ(game_new(&g, &p, 1, 91, "a"), E_BAD_ARG);
}

#define PM(x) CHECK((x) >= 0 && (x) <= 1000)
#define LOHI(a, b) CHECK((a) <= (b))

static void test_params_default_sane(void)
{
	Params p = params_default();
	PM(p.absent_permille); PM(p.spike_permille); PM(p.crash_permille);
	PM(p.cop_base_permille); PM(p.cop_per_10k_permille); PM(p.cop_max_permille);
	PM(p.run_permille); PM(p.cop_hit_permille); PM(p.drop_permille); PM(p.mug_permille);
	PM(p.rumor_permille); PM(p.rumor_true_permille);
	LOHI(p.spike_lo_pct, p.spike_hi_pct); LOHI(p.crash_lo_pct, p.crash_hi_pct);
	LOHI(p.enforce_dmg_lo, p.enforce_dmg_hi); LOHI(p.cop_dmg_lo, p.cop_dmg_hi);
	LOHI(p.loot_lo, p.loot_hi); LOHI(p.drop_lo_pct, p.drop_hi_pct);
	LOHI(p.mug_lo_pct, p.mug_hi_pct); LOHI(p.rumor_lead_lo, p.rumor_lead_hi);
	LOHI(p.flight_lo, p.flight_hi); LOHI(p.intl_flight_lo, p.intl_flight_hi);
	CHECK(p.start_city >= 0 && p.start_city < NUM_CITIES);
	CHECK(p.bank_rate_bp >= 0 && p.bank_rate_bp <= 3000);
	CHECK(p.min_goods >= 0 && p.min_goods <= NUM_GOODS);
	CHECK(params_sane(&p));
}

static void test_net_worth(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	g.cash = 1000;
	g.bank = 500;
	memset(g.debt, 0, sizeof g.debt);
	g.debt[0] = 300;
	g.debt[2] = 200;
	CHECK_EQ(game_net_worth(&g), 1000);
}

void tests_game(void)
{
	test_new_game_start_state();
	test_new_game_lengths();
	test_params_default_sane();
	test_net_worth();
}
