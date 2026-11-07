#include "check.h"
#include "tests.h"
#include "helpers.h"

static Loc at(int city, int hood)
{
	Loc l;
	l.city = (uint8_t)city;
	l.hood = (uint8_t)hood;
	return l;
}

static void setup(Game *g, Params *p)
{
	new_test_game(g, p, 30);
	memset(g->debt, 0, sizeof g->debt);
	memset(g->debt_age, 0, sizeof g->debt_age);
}

static void test_borrow_to_limit(void)
{
	Params p = test_params();
	Game g;
	int c;
	setup(&g, &p);
	c = g.loc.city;
	g.debt[c] = CITIES[c].lender_max - 100;
	g.cash = 0;
	CHECK_EQ(game_borrow(&g, 100), OK);
	CHECK_EQ(g.cash, 100);
	CHECK_EQ(g.debt[c], CITIES[c].lender_max);
	CHECK_EQ(game_borrow(&g, 1), E_LIMIT);
	CHECK_EQ(game_borrow(&g, 0), E_BAD_ARG);
}

static void test_borrow_into_zero_resets_clock(void)
{
	Params p = test_params();
	Game g;
	int c;
	setup(&g, &p);
	c = g.loc.city;
	g.debt_age[c] = 9;
	CHECK_EQ(game_borrow(&g, 500), OK);
	CHECK_EQ(g.debt_age[c], 0);
}

static void test_repay_errors(void)
{
	Params p = test_params();
	Game g, snap;
	int c;
	setup(&g, &p);
	c = g.loc.city;
	g.debt[c] = 1000;
	g.cash = 500;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_repay(&g, 600), E_NO_CASH);
	g.cash = 2000;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_repay(&g, 1500), E_BAD_ARG);
	CHECK_EQ(game_repay(&g, 0), E_BAD_ARG);
	CHECK(same_game(&g, &snap));
	CHECK_EQ(game_repay(&g, 1000), OK);
	CHECK_EQ(g.debt[c], 0);
	CHECK_EQ(g.cash, 1000);
}

static void test_small_payment_keeps_clock(void)
{
	Params p = test_params();
	Game g;
	int c;
	p.min_payment_pct = 10;
	setup(&g, &p);
	c = g.loc.city;
	g.debt[c] = 10000;
	g.debt_age[c] = 5;
	g.cash = 5000;
	CHECK_EQ(game_repay(&g, 999), OK);
	CHECK_EQ(g.debt_age[c], 5);
	CHECK_EQ(game_repay(&g, 1000), OK);
	CHECK_EQ(g.debt_age[c], 0);
}

static void test_bank_moves(void)
{
	Params p = test_params();
	Game g;
	setup(&g, &p);
	g.cash = 1000;
	g.bank = 0;
	CHECK_EQ(game_deposit(&g, 300), OK);
	CHECK_EQ(g.cash, 700);
	CHECK_EQ(g.bank, 300);
	CHECK_EQ(game_deposit(&g, 701), E_NO_CASH);
	CHECK_EQ(game_withdraw(&g, 400), E_NO_CASH);
	CHECK_EQ(game_withdraw(&g, 300), OK);
	CHECK_EQ(g.cash, 1000);
	CHECK_EQ(g.bank, 0);
	CHECK_EQ(game_withdraw(&g, -5), E_BAD_ARG);
}

static void test_goons_when_overdue(void)
{
	Params p = test_params();
	Game g;
	int c, i, found = 0;
	p.enforce_days = 3;
	p.enforce_dmg_lo = 10;
	p.enforce_dmg_hi = 10;
	setup(&g, &p);
	c = g.loc.city;
	g.debt[c] = 5000;
	g.debt_age[c] = 4;
	CHECK_EQ(game_move(&g, at(c, 1)), OK);
	CHECK_EQ(g.health, 90);
	for (i = 0; i < g.nmsgs; i++)
		if (g.msgs[i].kind == M_GOONS && g.msgs[i].a == c && g.msgs[i].amount == 10)
			found = 1;
	CHECK(found);
	CHECK_EQ(g.debt_age[c], 0);
	CHECK_EQ(g.stats.goon_visits, 1);
}

static void test_goons_only_in_their_city(void)
{
	Params p = test_params();
	Game g;
	p.enforce_days = 3;
	setup(&g, &p);
	g.debt[0] = 5000;
	g.debt_age[0] = 10;
	g.loc = at(1, 0);
	CHECK_EQ(game_move(&g, at(1, 1)), OK);
	CHECK_EQ(count_msgs(&g, M_GOONS), 0);
	CHECK_EQ(g.health, 100);
}

static void test_goons_can_kill(void)
{
	Params p = test_params();
	Game g;
	int c;
	p.enforce_days = 3;
	p.enforce_dmg_lo = 10;
	p.enforce_dmg_hi = 10;
	setup(&g, &p);
	c = g.loc.city;
	g.debt[c] = 5000;
	g.debt_age[c] = 4;
	g.health = 5;
	CHECK_EQ(game_move(&g, at(c, 1)), OK);
	CHECK_EQ(g.phase, PH_OVER);
	CHECK_EQ(g.end, END_DEAD);
	CHECK_EQ(g.health, 0);
	CHECK_EQ(count_msgs(&g, M_DEAD), 1);
}

void tests_finance(void)
{
	test_borrow_to_limit();
	test_borrow_into_zero_resets_clock();
	test_repay_errors();
	test_small_payment_keeps_clock();
	test_bank_moves();
	test_goons_when_overdue();
	test_goons_only_in_their_city();
	test_goons_can_kill();
}
