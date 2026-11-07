#include "check.h"
#include "tests.h"
#include "helpers.h"

static void test_heal(void)
{
	Params p = test_params();
	Game g;
	p.hospital_per_hp = 10;
	new_test_game(&g, &p, 30);
	g.health = 60;
	g.cash = 1000;
	CHECK_EQ(game_heal(&g, 50), E_LIMIT);
	CHECK_EQ(game_heal(&g, 0), E_BAD_ARG);
	CHECK_EQ(game_heal(&g, 40), OK);
	CHECK_EQ(g.health, 100);
	CHECK_EQ(g.cash, 600);
	CHECK_EQ(g.stats.hospital_spent, 400);
	CHECK_EQ(count_msgs(&g, M_HEALED), 1);
	CHECK_EQ(game_heal(&g, 1), E_LIMIT);
	g.health = 90;
	g.cash = 0;
	CHECK_EQ(game_heal(&g, 5), E_NO_CASH);
}

static int find_gear(GearKind kind, int local)
{
	int i;
	for (i = 0; i < NUM_GEAR; i++)
		if (GEAR[i].kind == kind && (local ? GEAR[i].city >= 0 : GEAR[i].city < 0))
			return i;
	return -1;
}

static void test_gear_rules(void)
{
	Params p = test_params();
	Game g;
	int local = find_gear(GEAR_WEAPON, 1), space = find_gear(GEAR_SPACE, 0), w1, w2, i;
	new_test_game(&g, &p, 30);
	g.cash = 1000000;

	CHECK(local >= 0);
	g.loc.city = (uint8_t)((GEAR[local].city + 1) % NUM_CITIES);
	CHECK(!game_gear_available(&g, local));
	CHECK_EQ(game_buy_gear(&g, local), E_NOT_SOLD);
	g.loc.city = (uint8_t)GEAR[local].city;
	CHECK(game_gear_available(&g, local));

	CHECK(space >= 0);
	CHECK_EQ(game_buy_gear(&g, space), OK);
	CHECK_EQ(g.capacity, p.start_capacity + GEAR[space].power);
	CHECK_EQ(g.cash, 1000000 - GEAR[space].price);
	CHECK_EQ(count_msgs(&g, M_BOUGHT_GEAR), 1);
	CHECK_EQ(game_buy_gear(&g, space), E_LIMIT);

	w1 = w2 = -1;
	for (i = 0; i < NUM_GEAR; i++)
		if (GEAR[i].kind == GEAR_WEAPON && GEAR[i].city < 0) {
			if (w1 < 0) w1 = i; else if (w2 < 0) w2 = i;
		}
	CHECK_EQ(game_buy_gear(&g, w1), OK);
	CHECK_EQ(g.weapon, w1);
	CHECK_EQ(game_buy_gear(&g, w2), OK);
	CHECK_EQ(g.weapon, w2);
	CHECK_EQ(game_buy_gear(&g, w2), E_LIMIT);
	CHECK_EQ(game_buy_gear(&g, NUM_GEAR), E_BAD_ARG);
	g.cash = 0;
	CHECK_EQ(game_buy_gear(&g, w1), E_NO_CASH);
}

static void test_stash_value(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	memset(g.stash, 0, sizeof g.stash);
	g.stash[0] = 2;
	g.stash[9] = 1;
	CHECK_EQ(game_stash_value(&g), 2LL * (GOODS[0].lo + GOODS[0].hi) / 2 + (GOODS[9].lo + GOODS[9].hi) / 2);
}

void tests_shop(void)
{
	test_heal();
	test_gear_rules();
	test_stash_value();
}
