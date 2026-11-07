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

static const Msg *find_msg(const Game *g, MsgKind k)
{
	int i;
	for (i = 0; i < g->nmsgs; i++)
		if (g->msgs[i].kind == k)
			return &g->msgs[i];
	return NULL;
}

static void test_true_tip_comes_true(void)
{
	Params p = test_params();
	Game g;
	const Msg *m;
	int good, spike, k, matched = 0;
	Loc where;
	p.rumor_permille = 1000;
	p.rumor_true_permille = 1000;
	p.rumor_lead_lo = 1;
	p.rumor_lead_hi = 1;
	new_test_game(&g, &p, 30);
	g.cash = 1000000000;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	m = find_msg(&g, M_RUMOR_TIP);
	CHECK(m != NULL);
	if (!m)
		return;
	good = m->a;
	where = at(m->b / HOODS_PER_CITY, m->b % HOODS_PER_CITY);
	spike = m->c;
	CHECK_EQ(m->amount, g.day + 1);
	CHECK_EQ(g.stats.rumors_true, 1);
	g.p.rumor_permille = 0;
	CHECK_EQ(game_move(&g, where), OK);
	CHECK(g.price[good] > 0);
	for (k = 0; k < g.nmsgs; k++)
		if (g.msgs[k].kind == (spike ? M_SPIKE : M_CRASH) && g.msgs[k].a == good)
			matched = 1;
	CHECK(matched);
	CHECK_EQ(g.tip.day, 0);
}

static void test_no_tip_past_last_day(void)
{
	Params p = test_params();
	Game g;
	p.rumor_permille = 1000;
	p.rumor_true_permille = 1000;
	p.rumor_lead_lo = 3;
	p.rumor_lead_hi = 3;
	new_test_game(&g, &p, 15);
	g.day = 13;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(g.day, 14);
	CHECK_EQ(count_msgs(&g, M_RUMOR_JOKE), 1);
	CHECK_EQ(count_msgs(&g, M_RUMOR_TIP), 0);
	CHECK_EQ(g.tip.day, 0);
}

static void test_missed_tip_expires(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	g.tip.day = 3;
	g.tip.loc = at(5, 5);
	g.tip.good = 2;
	g.tip.spike = 1;
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	CHECK_EQ(game_move(&g, at(g.loc.city, 2)), OK);
	CHECK_EQ(g.tip.day, 3);
	CHECK_EQ(game_move(&g, at(g.loc.city, 3)), OK);
	CHECK_EQ(g.day, 4);
	CHECK_EQ(g.tip.day, 0);
}

static void test_false_rumor_is_joke(void)
{
	Params p = test_params();
	Game g;
	const Msg *m;
	p.rumor_permille = 1000;
	p.rumor_true_permille = 0;
	new_test_game(&g, &p, 30);
	CHECK_EQ(game_move(&g, at(g.loc.city, 1)), OK);
	m = find_msg(&g, M_RUMOR_JOKE);
	CHECK(m != NULL);
	if (m)
		CHECK_EQ(m->a, g.loc.city);
	CHECK_EQ(g.stats.rumors_joke, 1);
}

void tests_rumor(void)
{
	test_true_tip_comes_true();
	test_no_tip_past_last_day();
	test_missed_tip_expires();
	test_false_rumor_is_joke();
}
