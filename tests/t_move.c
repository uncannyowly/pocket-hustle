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

static int has_msg(const Game *g, MsgKind k, int a, int64_t amount, int check_amount)
{
	int i;
	for (i = 0; i < g->nmsgs; i++)
		if (g->msgs[i].kind == k && g->msgs[i].a == a && (!check_amount || g->msgs[i].amount == amount))
			return 1;
	return 0;
}

static void test_transit_free_one_day(void)
{
	Params p = test_params();
	Game g;
	int64_t cash;
	new_test_game(&g, &p, 30);
	cash = g.cash;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.day, 2);
	CHECK_EQ(g.cash, cash);
	CHECK_EQ(g.loc.hood, 1);
	CHECK(has_msg(&g, M_RODE, 1, 0, 0));
	CHECK_EQ(g.stats.moves, 1);
}

static void test_flight_pays_ticket(void)
{
	Params p = test_params();
	Game g;
	int64_t c, cash;
	new_test_game(&g, &p, 30);
	g.cash = 10000;
	cash = g.cash;
	c = game_flight_cost(&g, 3);
	CHECK(c >= p.flight_lo && c <= p.flight_hi);
	CHECK_EQ(game_move(&g, at(3, 0)), OK);
	CHECK_EQ(g.cash, cash - c);
	CHECK(has_msg(&g, M_FLEW, 3, c, 1));
	CHECK_EQ(g.stats.flights, 1);
	CHECK_EQ(g.loc.city, 3);
}

static void test_flight_needs_cash(void)
{
	Params p = test_params();
	Game g, snap;
	new_test_game(&g, &p, 30);
	g.cash = 0;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_move(&g, at(3, 0)), E_NO_CASH);
	CHECK(same_game(&g, &snap));
}

static void test_flight_cost_stable(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	CHECK_EQ(game_flight_cost(&g, 4), game_flight_cost(&g, 4));
	CHECK_EQ(game_flight_cost(&g, g.loc.city), 0);
}

static void test_bad_destinations(void)
{
	Params p = test_params();
	Game g, snap;
	new_test_game(&g, &p, 30);
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_move(&g, g.loc), E_BAD_ARG);
	CHECK_EQ(game_move(&g, at(7, 0)), E_BAD_ARG);
	CHECK_EQ(game_move(&g, at(0, 6)), E_BAD_ARG);
	CHECK(same_game(&g, &snap));
}

static void test_debt_interest_rounds_up(void)
{
	Params p = test_params();
	Game g;
	int c;
	int64_t r;
	new_test_game(&g, &p, 30);
	c = g.loc.city;
	r = CITIES[c].lender_rate_bp;
	g.debt[c] = 5500;
	g.debt_age[c] = 0;
	CHECK_EQ(game_move(&g, at(c, 1)), OK);
	CHECK_EQ(g.debt[c], 5500 + (5500 * r + 9999) / 10000);
	CHECK_EQ(g.debt_age[c], 1);
}

static void test_bank_interest_rounds_down(void)
{
	Params p = test_params();
	Game g;
	p.bank_rate_bp = 150;
	new_test_game(&g, &p, 30);
	g.bank = 1000;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.bank, 1015);
	g.bank = 99;
	CHECK_EQ(game_move(&g, at(g.loc.city, 2)), OK);
	CHECK_EQ(g.bank, 100);
}

static void test_final_day_ends_without_events(void)
{
	Params p = test_params();
	Game g;
	Loc before;
	p.cop_base_permille = 1000;
	p.mug_permille = 1000;
	p.rumor_permille = 1000;
	p.spike_permille = 1000;
	new_test_game(&g, &p, 15);
	g.day = 15;
	before = g.loc;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.phase, PH_OVER);
	CHECK_EQ(g.end, END_DAYS_UP);
	CHECK_EQ(g.day, 15);
	CHECK_EQ(g.loc.city, before.city);
	CHECK_EQ(g.loc.hood, before.hood);
	CHECK_EQ(g.nmsgs, 1);
	CHECK_EQ(g.msgs[0].kind, M_GAME_OVER);
}

static void test_last_day_warning(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 15);
	g.day = 14;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.day, 15);
	CHECK_EQ(count_msgs(&g, M_LAST_DAY), 1);
	CHECK_EQ(g.phase, PH_TRADING);
}

static void test_compound_debt_clamped(void)
{
	Params p = test_params();
	Game g;
	int c;
	new_test_game(&g, &p, 30);
	c = g.loc.city;
	g.debt[c] = MONEY_MAX - 5;
	CHECK_EQ(game_move(&g, at(c, 1)), OK);
	CHECK_EQ(g.debt[c], MONEY_MAX);
}

static void test_end_quits(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	CHECK_EQ(game_end(&g), OK);
	CHECK_EQ(g.end, END_QUIT);
	CHECK_EQ(g.phase, PH_OVER);
	CHECK_EQ(game_end(&g), E_PHASE);
}

void tests_move(void)
{
	test_transit_free_one_day();
	test_flight_pays_ticket();
	test_flight_needs_cash();
	test_flight_cost_stable();
	test_bad_destinations();
	test_debt_interest_rounds_up();
	test_bank_interest_rounds_down();
	test_final_day_ends_without_events();
	test_last_day_warning();
	test_compound_debt_clamped();
	test_end_quits();
}
