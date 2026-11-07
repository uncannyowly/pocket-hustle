#include "check.h"
#include "tests.h"
#include "helpers.h"

static void setup(Game *g)
{
	Params p = test_params();
	int i;
	new_test_game(g, &p, 30);
	for (i = 0; i < NUM_GOODS; i++)
		g->price[i] = 0;
	g->capacity = 100;
}

static void test_buy_basic(void)
{
	Game g;
	setup(&g);
	g.price[0] = 100;
	g.cash = 1000;
	CHECK_EQ(game_buy(&g, 0, 3), OK);
	CHECK_EQ(g.cash, 700);
	CHECK_EQ(g.stash[0], 3);
	CHECK_EQ(g.stash_paid[0], 300);
	CHECK_EQ(game_carried(&g), 3);
}

static void test_buy_errors_change_nothing(void)
{
	Game g, snap;
	setup(&g);
	g.price[0] = 100;
	g.cash = 1000;
	g.capacity = 5;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_buy(&g, 1, 1), E_NOT_SOLD);
	CHECK(same_game(&g, &snap));
	CHECK_EQ(game_buy(&g, 0, 0), E_BAD_ARG);
	CHECK_EQ(game_buy(&g, 0, -1), E_BAD_ARG);
	CHECK_EQ(game_buy(&g, -1, 1), E_BAD_ARG);
	CHECK_EQ(game_buy(&g, NUM_GOODS, 1), E_BAD_ARG);
	CHECK(same_game(&g, &snap));
	CHECK_EQ(game_buy(&g, 0, 11), E_NO_CASH);
	CHECK(same_game(&g, &snap));
	CHECK_EQ(game_buy(&g, 0, 6), E_NO_SPACE);
	CHECK(same_game(&g, &snap));
}

static void test_sell_basic(void)
{
	Game g;
	setup(&g);
	g.cash = 0;
	g.stash[1] = 4;
	g.stash_paid[1] = 400;
	g.price[1] = 250;
	CHECK_EQ(game_sell(&g, 1, 1), OK);
	CHECK_EQ(g.cash, 250);
	CHECK_EQ(g.stash[1], 3);
	CHECK_EQ(g.stash_paid[1], 300);
}

static void test_sell_more_than_owned(void)
{
	Game g, snap;
	setup(&g);
	g.stash[1] = 4;
	g.price[1] = 250;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_sell(&g, 1, 5), E_NO_STOCK);
	CHECK(same_game(&g, &snap));
	g.price[1] = 0;
	memcpy(&snap, &g, sizeof g);
	CHECK_EQ(game_sell(&g, 1, 1), E_NOT_SOLD);
	CHECK(same_game(&g, &snap));
}

static void test_max_buy(void)
{
	Game g;
	setup(&g);
	g.price[0] = 100;
	g.capacity = 20;
	g.stash[3] = 15;
	g.cash = 950;
	CHECK_EQ(game_max_buy(&g, 0), 5);
	g.cash = 350;
	CHECK_EQ(game_max_buy(&g, 0), 3);
	CHECK_EQ(game_max_buy(&g, 1), 0);
}

static void test_huge_cash_no_overflow(void)
{
	Game g;
	setup(&g);
	g.cash = MONEY_MAX;
	g.price[0] = 2000000000;
	g.capacity = 100;
	CHECK_EQ(game_max_buy(&g, 0), 100);
	CHECK_EQ(game_buy(&g, 0, 100), OK);
	CHECK_EQ(g.cash, MONEY_MAX - 200000000000LL);
	CHECK_EQ(g.stash_paid[0], 200000000000LL);
}

void tests_trade(void)
{
	test_buy_basic();
	test_buy_errors_change_nothing();
	test_sell_basic();
	test_sell_more_than_owned();
	test_max_buy();
	test_huge_cash_no_overflow();
}
