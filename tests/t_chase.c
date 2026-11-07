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

static int first_weapon(void)
{
	int i;
	for (i = 0; i < NUM_GEAR; i++)
		if (GEAR[i].kind == GEAR_WEAPON && GEAR[i].city < 0)
			return i;
	return -1;
}

/* A game already in a chase with the given number of cops. */
static void in_chase(Game *g, Params *p, int cops_left)
{
	new_test_game(g, p, 30);
	g->phase = PH_CHASE;
	g->chase.posse = (int16_t)(cops_left - 1);
	g->chase.cops_left = (int16_t)cops_left;
}

static void test_cops_start_chase(void)
{
	Params p = test_params();
	Game g;
	p.cop_base_permille = 1000;
	new_test_game(&g, &p, 30);
	g.price[0] = 10;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.phase, PH_CHASE);
	CHECK_EQ(count_msgs(&g, M_COPS), 1);
	CHECK_EQ(g.stats.cop_chases, 1);
	CHECK_EQ(game_buy(&g, 0, 1), E_PHASE);
}

static void test_posse_grows(void)
{
	Params p = test_params();
	Game g;
	p.cop_base_permille = 1000;
	p.posse_start = 2;
	p.posse_growth = 1;
	new_test_game(&g, &p, 30);
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.chase.cops_left, 3);
	CHECK_EQ(g.msgs[g.nmsgs - 1].a, 2);
	new_test_game(&g, &p, 30);
	g.stats.cop_chases = 3;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.chase.posse, 5);
	CHECK_EQ(g.chase.cops_left, 6);
}

static void test_run_escapes(void)
{
	Params p = test_params();
	Game g;
	p.run_permille = 1000;
	in_chase(&g, &p, 3);
	CHECK_EQ(game_chase(&g, CH_RUN), OK);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(count_msgs(&g, M_ESCAPED), 1);
	CHECK_EQ(g.stats.escapes, 1);
}

static void test_run_fails_shot(void)
{
	Params p = test_params();
	Game g;
	p.run_permille = 0;
	p.cop_hit_permille = 1000;
	p.cop_dmg_lo = 7;
	p.cop_dmg_hi = 7;
	p.max_shooters = 2;
	in_chase(&g, &p, 5);
	CHECK_EQ(game_chase(&g, CH_RUN), OK);
	CHECK_EQ(g.health, 86);
	CHECK_EQ(count_msgs(&g, M_HIT_YOU), 2);
	CHECK_EQ(g.phase, PH_CHASE);
}

static void test_drop_on_escape(void)
{
	Params p = test_params();
	Game g;
	int i, found = 0;
	p.run_permille = 1000;
	p.drop_permille = 1000;
	p.drop_lo_pct = 50;
	p.drop_hi_pct = 50;
	in_chase(&g, &p, 2);
	g.stash[2] = 10;
	g.stash_paid[2] = 1000;
	CHECK_EQ(game_chase(&g, CH_RUN), OK);
	CHECK_EQ(g.stash[2], 5);
	CHECK_EQ(g.stash_paid[2], 500);
	for (i = 0; i < g.nmsgs; i++)
		if (g.msgs[i].kind == M_DROPPED && g.msgs[i].a == 2 && g.msgs[i].amount == 5)
			found = 1;
	CHECK(found);
	CHECK_EQ(g.stats.times_dropped, 1);
}

static void test_fight_needs_weapon(void)
{
	Params p = test_params();
	Game g, snap;
	in_chase(&g, &p, 2);
	g.weapon = -1;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_chase(&g, CH_FIGHT), E_BAD_ARG);
	CHECK(same_game(&g, &snap));
}

static void test_fight_until_won(void)
{
	Params p = test_params();
	Game g;
	int calls = 0, each_ok = 1;
	int64_t cash;
	p.cop_hit_permille = 0;
	p.loot_lo = 500;
	p.loot_hi = 500;
	in_chase(&g, &p, 2);
	g.weapon = (int8_t)first_weapon();
	CHECK(g.weapon >= 0);
	cash = g.cash;
	while (g.phase == PH_CHASE && calls < 200) {
		CHECK_EQ(game_chase(&g, CH_FIGHT), OK);
		if (count_msgs(&g, M_YOU_HIT) + count_msgs(&g, M_YOU_MISSED) != 1)
			each_ok = 0;
		calls++;
	}
	CHECK(each_ok);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(g.cash, cash + 500);
	CHECK_EQ(g.stats.cops_beaten, 1);
	CHECK_EQ(count_msgs(&g, M_COPS_BEATEN), 1);
}

static void test_surrender_jails(void)
{
	Params p = test_params();
	Game g;
	int i, found = 0;
	p.jail_fine_pct = 20;
	in_chase(&g, &p, 2);
	g.stash[1] = 10;
	g.stash_paid[1] = 700;
	g.cash = 1000;
	CHECK_EQ(game_chase(&g, CH_SURRENDER), OK);
	CHECK_EQ(g.stash[1], 0);
	CHECK_EQ(g.stash_paid[1], 0);
	CHECK_EQ(g.cash, 800);
	CHECK_EQ(g.day, 2);
	CHECK_EQ(g.phase, PH_TRADING);
	for (i = 0; i < g.nmsgs; i++)
		if (g.msgs[i].kind == M_JAILED && g.msgs[i].amount == 200)
			found = 1;
	CHECK(found);
	CHECK_EQ(g.stats.times_jailed, 1);
	CHECK_EQ(g.hist[0][2], g.price[0]);
}

static void test_jail_on_final_day_ends(void)
{
	Params p = test_params();
	Game g;
	in_chase(&g, &p, 2);
	g.day = g.last_day;
	CHECK_EQ(game_chase(&g, CH_SURRENDER), OK);
	CHECK_EQ(g.phase, PH_OVER);
	CHECK_EQ(g.end, END_DAYS_UP);
	CHECK_EQ(g.day, g.last_day);
}

static void test_mugging_pocket_only(void)
{
	Params p = test_params();
	Game g;
	int i, found = 0;
	p.mug_permille = 1000;
	p.mug_lo_pct = 25;
	p.mug_hi_pct = 25;
	new_test_game(&g, &p, 30);
	g.cash = 1000;
	g.bank = 500;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.cash, 750);
	CHECK(g.bank >= 500);
	for (i = 0; i < g.nmsgs; i++)
		if (g.msgs[i].kind == M_MUGGED && g.msgs[i].amount == 250)
			found = 1;
	CHECK(found);
	CHECK_EQ(g.stats.times_mugged, 1);
}

static void test_no_mugging_with_cops(void)
{
	Params p = test_params();
	Game g;
	int64_t cash;
	p.cop_base_permille = 1000;
	p.mug_permille = 1000;
	new_test_game(&g, &p, 30);
	cash = g.cash;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(count_msgs(&g, M_COPS), 1);
	CHECK_EQ(count_msgs(&g, M_MUGGED), 0);
	CHECK_EQ(g.cash, cash);
}

static void test_wrong_phase_changes_nothing(void)
{
	Params p = test_params();
	Game g, snap;
	Loc there = {0, 1};
	in_chase(&g, &p, 2);
	g.price[0] = 10;
	g.stash[0] = 1;
	g.bank = 100;
	g.debt[g.loc.city] = 100;
	g.health = 50;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_buy(&g, 0, 1), E_PHASE);
	CHECK_EQ(game_sell(&g, 0, 1), E_PHASE);
	CHECK_EQ(game_move(&g, there), E_PHASE);
	CHECK_EQ(game_borrow(&g, 10), E_PHASE);
	CHECK_EQ(game_repay(&g, 10), E_PHASE);
	CHECK_EQ(game_deposit(&g, 10), E_PHASE);
	CHECK_EQ(game_withdraw(&g, 10), E_PHASE);
	CHECK_EQ(game_heal(&g, 10), E_PHASE);
	CHECK_EQ(game_buy_gear(&g, 0), E_PHASE);
	CHECK(same_game(&g, &snap));

	g.phase = PH_TRADING;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_chase(&g, CH_RUN), E_PHASE);
	CHECK(same_game(&g, &snap));

	CHECK_EQ(game_end(&g), OK);
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_buy(&g, 0, 1), E_PHASE);
	CHECK_EQ(game_sell(&g, 0, 1), E_PHASE);
	CHECK_EQ(game_move(&g, there), E_PHASE);
	CHECK_EQ(game_borrow(&g, 10), E_PHASE);
	CHECK_EQ(game_repay(&g, 10), E_PHASE);
	CHECK_EQ(game_deposit(&g, 10), E_PHASE);
	CHECK_EQ(game_withdraw(&g, 10), E_PHASE);
	CHECK_EQ(game_heal(&g, 10), E_PHASE);
	CHECK_EQ(game_buy_gear(&g, 0), E_PHASE);
	CHECK_EQ(game_chase(&g, CH_FIGHT), E_PHASE);
	CHECK_EQ(game_end(&g), E_PHASE);
	CHECK(same_game(&g, &snap));
}

void tests_chase(void)
{
	test_cops_start_chase();
	test_posse_grows();
	test_run_escapes();
	test_run_fails_shot();
	test_drop_on_escape();
	test_fight_needs_weapon();
	test_fight_until_won();
	test_surrender_jails();
	test_jail_on_final_day_ends();
	test_mugging_pocket_only();
	test_no_mugging_with_cops();
	test_wrong_phase_changes_nothing();
}
