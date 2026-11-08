/* Wild cards, finds and the shady crew: offers, stakes, the crew, the blacklist, saves, and a
 * bot that takes every offer over thousands of games. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "tests.h"
#include "helpers.h"
#include "bots.h"
#include "core/save.h"
#include "content/text.h"

static Loc at(int city, int hood)
{
	Loc l;
	l.city = (uint8_t)city;
	l.hood = (uint8_t)hood;
	return l;
}

/* Rides to the next hood in the same city. */
static int ride(Game *g)
{
	return game_move(g, at(g->loc.city, (g->loc.hood + 1) % HOODS_PER_CITY));
}

static int msg_index(const Game *g, MsgKind k)
{
	int i;
	for (i = 0; i < g->nmsgs; i++)
		if (g->msgs[i].kind == k)
			return i;
	return -1;
}

static void stock_up(Game *g)
{
	int i;
	for (i = 0; i < NUM_GOODS; i++) {
		g->stash[i] = 10;
		g->stash_paid[i] = 10 * (int64_t)g->price[i];
	}
}

static Params rc_params(void)
{
	Params p = test_params();
	p.rc_offer_permille = 1000;
	return p;
}

static void put_stake(Game *g, int chem, int64_t stake, int resolve_day, int found)
{
	Stake *s = &g->stakes[g->nstakes++];
	s->chem = (int8_t)chem;
	s->stake = stake;
	s->resolve_day = (int16_t)resolve_day;
	s->found = (int8_t)found;
}

/* --- chemical offers --- */

static void test_offer_appears_and_expires(void)
{
	Params p = rc_params();
	Game g;
	new_test_game(&g, &p, 30);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(g.nstakes, 0);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_RC);
	CHECK(g.offer.chem >= 0 && g.offer.chem < NUM_CHEMS);
	CHECK_EQ(g.offer.day, g.day);
	CHECK_EQ(count_msgs(&g, M_RC_OFFER), 1);
	/* it stays through trading */
	CHECK_EQ(game_buy(&g, 0, 1), OK);
	CHECK_EQ(g.offer.kind, OFFER_RC);
	/* the next move closes it (and with the chance off, no new one) */
	g.p.rc_offer_permille = 0;
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(count_msgs(&g, M_RC_OFFER), 0);
}

static void test_no_offer_when_chance_zero(void)
{
	Params p = test_params();
	Game g;
	int i;
	new_test_game(&g, &p, 30);
	for (i = 0; i < 20; i++) {
		CHECK_EQ(ride(&g), OK);
		CHECK_EQ(g.offer.kind, OFFER_NONE);
		CHECK_EQ(g.nstakes, 0);
		CHECK_EQ(g.nmsgs, 1);	/* just the ride */
	}
}

static void test_back_takes_cash_and_makes_stake(void)
{
	Params p = rc_params();
	Game g;
	int64_t cash;
	int chem;
	new_test_game(&g, &p, 30);
	CHECK_EQ(game_rc_back(&g, 100), E_PHASE);	/* no offer yet */
	CHECK_EQ(game_offer_pass(&g), E_PHASE);
	CHECK_EQ(game_crew_accept(&g), E_PHASE);
	ride(&g);
	chem = g.offer.chem;
	cash = g.cash;
	CHECK_EQ(game_rc_back(&g, 0), E_BAD_ARG);
	CHECK_EQ(game_rc_back(&g, -5), E_BAD_ARG);
	CHECK_EQ(game_rc_back(&g, cash + 1), E_NO_CASH);
	CHECK_EQ(g.cash, cash);
	CHECK_EQ(g.nstakes, 0);
	CHECK_EQ(g.offer.kind, OFFER_RC);
	CHECK_EQ(game_rc_back(&g, 1000), OK);
	CHECK_EQ(g.cash, cash - 1000);
	CHECK_EQ(g.nstakes, 1);
	CHECK_EQ(g.stakes[0].chem, chem);
	CHECK_EQ(g.stakes[0].stake, 1000);
	CHECK_EQ(g.stakes[0].found, 0);
	CHECK(g.stakes[0].resolve_day >= g.day + p.rc_days_lo && g.stakes[0].resolve_day <= g.day + p.rc_days_hi);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(game_rc_back(&g, 100), E_PHASE);	/* one offer, one stake */
	CHECK_EQ(g.nstakes, 1);
}

static void test_back_everything(void)
{
	Params p = rc_params();
	Game g;
	new_test_game(&g, &p, 30);
	ride(&g);
	CHECK_EQ(game_rc_back(&g, g.cash), OK);
	CHECK_EQ(g.cash, 0);
	CHECK_EQ(g.stakes[0].stake, p.start_cash);
}

static void test_pass_declines(void)
{
	Params p = rc_params();
	Game g;
	int64_t cash;
	new_test_game(&g, &p, 30);
	ride(&g);
	cash = g.cash;
	CHECK_EQ(game_offer_pass(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(g.cash, cash);
	CHECK_EQ(g.nstakes, 0);
	CHECK_EQ(game_rc_back(&g, 100), E_PHASE);
}

static void test_wild_actions_need_trading(void)
{
	Params p = rc_params();
	Game g;
	new_test_game(&g, &p, 15);
	ride(&g);
	g.phase = PH_CHASE;
	CHECK_EQ(game_rc_back(&g, 100), E_PHASE);
	CHECK_EQ(game_offer_pass(&g), E_PHASE);
	g.phase = PH_OVER;
	CHECK_EQ(game_rc_back(&g, 100), E_PHASE);
}

static void test_no_offer_when_stakes_full(void)
{
	Params p = rc_params();
	Game g;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 100, 29, 0);
	put_stake(&g, 1, 100, 29, 0);
	put_stake(&g, 2, 100, 29, 0);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(g.nstakes, 3);
}

static void test_offer_uses_a_slot_freed_this_arrival(void)
{
	Params p = rc_params();
	Game g;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 100, 29, 0);
	put_stake(&g, 1, 100, 29, 0);
	put_stake(&g, 2, 100, 2, 0);	/* due on the next arrival */
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.nstakes, 2);
	CHECK_EQ(g.offer.kind, OFFER_RC);
}

/* --- stakes resolve --- */

static void test_stake_resolves_on_its_day(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 1000, 3, 0);
	CHECK_EQ(ride(&g), OK);		/* day 2 */
	CHECK_EQ(g.nstakes, 1);
	CHECK_EQ(count_msgs(&g, M_RC_HIT) + count_msgs(&g, M_RC_BUST), 0);
	CHECK_EQ(ride(&g), OK);		/* day 3 */
	CHECK_EQ(g.nstakes, 0);
	CHECK_EQ(count_msgs(&g, M_RC_HIT) + count_msgs(&g, M_RC_BUST), 1);
	/* gone: nothing more on later days */
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(count_msgs(&g, M_RC_HIT) + count_msgs(&g, M_RC_BUST), 0);
}

static void test_stake_pays_exact_multiple_or_nothing(void)
{
	Params p = test_params();
	int hits = 0, busts = 0;
	uint64_t seed;
	for (seed = 1; seed <= 200; seed++) {
		Game g;
		int64_t before, delta;
		game_new(&g, &p, seed, 30, "T");
		put_stake(&g, 0, 1000, 2, 0);	/* 2C-B: 200..250 percent */
		before = g.cash;
		CHECK_EQ(ride(&g), OK);
		delta = g.cash - before;
		CHECK_EQ(g.nstakes, 0);
		if (count_msgs(&g, M_RC_HIT)) {
			int pct = (int)(delta / 10);
			hits++;
			CHECK_EQ(delta % 10, 0);
			CHECK(pct >= CHEMS[0].mult_lo_pct && pct <= CHEMS[0].mult_hi_pct);
			CHECK_EQ(g.msgs[msg_index(&g, M_RC_HIT)].amount, delta);
			CHECK_EQ(g.msgs[msg_index(&g, M_RC_HIT)].a, 0);
			CHECK_EQ(g.stats.wild_wins, 1);
		} else {
			busts++;
			CHECK_EQ(delta, 0);
			CHECK_EQ(count_msgs(&g, M_RC_BUST), 1);
			CHECK_EQ(g.msgs[msg_index(&g, M_RC_BUST)].amount, 1000);
			CHECK_EQ(g.stats.wild_losses, 1);
		}
	}
	CHECK(hits > 60 && busts > 40);	/* 600 permille over 200 games */
}

static void test_found_stake_bust_costs_nothing(void)
{
	Params p = test_params();
	int hits = 0, busts = 0;
	uint64_t seed;
	for (seed = 1; seed <= 200; seed++) {
		Game g;
		int64_t before;
		game_new(&g, &p, seed, 30, "T");
		put_stake(&g, 7, 3000, 2, 1);	/* Hexen: 250 permille, 300..450 percent */
		before = g.cash;
		CHECK_EQ(ride(&g), OK);
		if (count_msgs(&g, M_RC_HIT)) {
			int64_t delta = g.cash - before;
			hits++;
			CHECK(delta >= 3000 * 3 && delta <= 3000 * 450 / 100);
		} else {
			busts++;
			CHECK_EQ(g.cash, before);
			CHECK_EQ(count_msgs(&g, M_RC_BUST_FOUND), 1);
			CHECK_EQ(count_msgs(&g, M_RC_BUST), 0);
		}
	}
	CHECK(hits > 20 && busts > 100);
}

static void test_two_stakes_resolve_independently(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 500, 2, 0);
	put_stake(&g, 1, 700, 4, 0);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.nstakes, 1);
	CHECK_EQ(g.stakes[0].chem, 1);
	CHECK_EQ(g.stakes[0].stake, 700);
	CHECK_EQ(g.stakes[1].stake, 0);	/* the freed slot is cleared */
}

static void test_stake_pays_on_late_arrival_after_overdue(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 500, 1, 0);	/* already past due: resolves at the next arrival */
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.nstakes, 0);
}

/* --- finds --- */

static void test_find_is_a_free_stake(void)
{
	Params p = test_params();
	Game g;
	int64_t cash;
	p.rc_find_permille = 1000;
	new_test_game(&g, &p, 30);
	cash = g.cash;
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.cash, cash);
	CHECK_EQ(g.nstakes, 1);
	CHECK_EQ(g.stakes[0].found, 1);
	CHECK(g.stakes[0].stake >= p.rc_find_lo && g.stakes[0].stake <= p.rc_find_hi);
	CHECK(g.stakes[0].chem >= 0 && g.stakes[0].chem < NUM_CHEMS);
	CHECK(g.stakes[0].resolve_day >= g.day + p.rc_days_lo && g.stakes[0].resolve_day <= g.day + p.rc_days_hi);
	CHECK_EQ(count_msgs(&g, M_RC_FIND), 1);
	CHECK_EQ(g.msgs[msg_index(&g, M_RC_FIND)].amount, g.stakes[0].stake);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
}

static void test_never_more_than_three_stakes(void)
{
	Params p = test_params();
	Game g;
	int i, max_seen = 0;
	p.rc_find_permille = 1000;
	p.rc_offer_permille = 1000;
	new_test_game(&g, &p, 90);
	for (i = 0; i < 60; i++) {
		CHECK_EQ(ride(&g), OK);
		if (g.offer.kind == OFFER_RC)
			CHECK_EQ(game_rc_back(&g, 10), OK);	/* a slot is always kept for an open offer */
		CHECK(g.nstakes <= 3);
		if (g.nstakes > max_seen)
			max_seen = g.nstakes;
	}
	CHECK_EQ(max_seen, 3);
}

static void test_find_leaves_the_slot_for_an_offer(void)
{
	Params p = test_params();
	Game g;
	p.rc_find_permille = 1000;
	p.rc_offer_permille = 1000;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 100, 29, 0);
	put_stake(&g, 1, 100, 29, 0);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_RC);
	CHECK_EQ(g.nstakes, 2);		/* the find waited: the last slot is the offer's */
	CHECK_EQ(count_msgs(&g, M_RC_FIND), 0);
}

/* --- arrival order --- */

static void test_arrival_order(void)
{
	Params p = test_params();
	Game g;
	int hit, bust, rumor, offer, find;
	p.rumor_permille = 1000;
	p.rumor_true_permille = 0;
	p.rc_offer_permille = 1000;
	p.rc_find_permille = 1000;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 100, 2, 0);
	CHECK_EQ(ride(&g), OK);
	hit = msg_index(&g, M_RC_HIT);
	bust = msg_index(&g, M_RC_BUST);
	rumor = msg_index(&g, M_RUMOR_JOKE);
	offer = msg_index(&g, M_RC_OFFER);
	find = msg_index(&g, M_RC_FIND);
	CHECK((hit >= 0) != (bust >= 0));
	CHECK((hit >= 0 ? hit : bust) < rumor);
	CHECK(rumor < offer);
	CHECK(offer < find);
}

static void test_no_offer_during_a_chase_but_stakes_resolve(void)
{
	Params p = test_params();
	Game g;
	p.cop_base_permille = 1000;
	p.rc_offer_permille = 1000;
	p.rc_find_permille = 1000;
	new_test_game(&g, &p, 30);
	put_stake(&g, 0, 100, 2, 0);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.phase, PH_CHASE);
	CHECK_EQ(g.nstakes, 0);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(count_msgs(&g, M_RC_OFFER) + count_msgs(&g, M_RC_FIND), 0);
}

/* The wild cards use their own random stream: the gameplay streams do not notice them. */
static void test_separate_random_stream(void)
{
	Params off = test_params(), on = test_params();
	Game a, b;
	int i;
	on.rc_offer_permille = 500;
	on.rc_find_permille = 500;
	on.spike_permille = off.spike_permille = 100;
	on.rumor_permille = off.rumor_permille = 0;
	game_new(&a, &off, 9, 60, "T");
	game_new(&b, &on, 9, 60, "T");
	a.cash = b.cash = 1000000;
	for (i = 0; i < 40; i++) {
		Loc d = at((a.loc.city + (i % 5 == 4 ? 1 : 0)) % NUM_CITIES, (a.loc.hood + 1) % HOODS_PER_CITY);
		if (d.city != a.loc.city)
			d.hood = 0;
		CHECK_EQ(game_move(&a, d), OK);
		CHECK_EQ(game_move(&b, d), OK);
		CHECK(memcmp(a.price, b.price, sizeof a.price) == 0);
		CHECK(a.rng.state == b.rng.state && a.rng.inc == b.rng.inc);
	}
	CHECK(b.stats.wild_wins + b.stats.wild_losses > 0);
	CHECK_EQ(a.stats.wild_wins + a.stats.wild_losses, 0);
}

/* --- the crew --- */

static Params crew_params(void)
{
	Params p = test_params();
	p.cohort_offer_permille = 1000;
	return p;
}

static void test_crew_needs_a_big_stash(void)
{
	Params p = crew_params();
	Game g;
	new_test_game(&g, &p, 30);
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_NONE);	/* empty hands */
	CHECK_EQ(g.crew_offers, 0);
	g.stash[0] = 1;
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_NONE);	/* one cart is not a stash */
	stock_up(&g);
	CHECK(game_stash_value(&g) >= g.p.cohort_min_stash);
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	CHECK_EQ(g.crew_offers, 1);
	CHECK_EQ(count_msgs(&g, M_CREW_OFFER), 1);
}

static void test_crew_at_most_twice_even_declined(void)
{
	Params p = crew_params();
	Game g;
	int i, offers = 0;
	new_test_game(&g, &p, 90);
	stock_up(&g);
	for (i = 0; i < 40; i++) {
		CHECK_EQ(ride(&g), OK);
		offers += count_msgs(&g, M_CREW_OFFER);
		if (g.offer.kind == OFFER_CREW && i % 2)
			CHECK_EQ(game_offer_pass(&g), OK);	/* declining still counts */
	}
	CHECK_EQ(offers, 2);
	CHECK_EQ(g.crew_offers, 2);
}

static void test_crew_beats_chemist_then_chemist_returns(void)
{
	Params p = crew_params();
	Game g;
	int i;
	p.rc_offer_permille = 1000;
	new_test_game(&g, &p, 30);
	stock_up(&g);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	CHECK_EQ(count_msgs(&g, M_RC_OFFER), 0);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	for (i = 0; i < 2; i++)
		CHECK_EQ(ride(&g), OK);
	CHECK_EQ(g.offer.kind, OFFER_RC);
}

static void test_crew_payout(void)
{
	Params p = crew_params();
	Game g;
	int64_t cash, value = 0;
	int i;
	p.cohort_od_permille = 0;
	new_test_game(&g, &p, 30);
	stock_up(&g);
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	CHECK_EQ(game_rc_back(&g, 10), E_PHASE);	/* wrong kind of offer */
	cash = g.cash;
	for (i = 0; i < NUM_GOODS; i++)
		value += (int64_t)g.stash[i] * (g.price[i] > 0 ? g.price[i] : (GOODS[i].lo + GOODS[i].hi) / 2);
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK_EQ(g.cash, cash + (value * 3 > 10000 ? value * 3 : 10000));
	CHECK_EQ(game_carried(&g), 0);
	for (i = 0; i < NUM_GOODS; i++)
		CHECK_EQ(g.stash_paid[i], 0);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(g.banned, 0);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(count_msgs(&g, M_CREW_PAID), 1);
	CHECK_EQ(g.msgs[msg_index(&g, M_CREW_PAID)].amount, g.cash - cash);
	CHECK_EQ(game_crew_accept(&g), E_PHASE);
}

static void test_crew_payout_at_least_the_minimum(void)
{
	Params p = crew_params();
	Game g;
	int64_t cash;
	p.cohort_od_permille = 0;
	p.cohort_min_stash = 1;
	p.cohort_mult_pct = 100;
	new_test_game(&g, &p, 30);
	g.stash[0] = 1;
	g.stash_paid[0] = 30;
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	cash = g.cash;
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK(g.cash - cash >= p.cohort_min_payout);
}

static void test_crew_accept_needs_the_stash_still(void)
{
	Params p = crew_params();
	Game g;
	int i;
	new_test_game(&g, &p, 30);
	stock_up(&g);
	ride(&g);
	for (i = 0; i < NUM_GOODS; i++)
		g.stash[i] = 0;		/* sold it all after the offer */
	CHECK_EQ(game_crew_accept(&g), E_NO_STOCK);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
}

static void test_overdose(void)
{
	Params p = crew_params();
	Game g;
	int old, i;
	int64_t cash;
	p.cohort_od_permille = 1000;
	new_test_game(&g, &p, 30);
	stock_up(&g);
	ride(&g);
	old = g.loc.city;
	cash = g.cash;
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK_EQ(g.cash, cash);
	CHECK_EQ(game_carried(&g), 0);
	CHECK_EQ(game_city_banned(&g, old), 1);
	CHECK((g.banned & (1u << old)) != 0);
	CHECK(g.loc.city != old);
	CHECK_EQ(g.loc.hood, 0);
	CHECK_EQ(game_city_banned(&g, g.loc.city), 0);
	CHECK_EQ(g.day, 3);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(count_msgs(&g, M_OVERDOSE), 1);
	CHECK_EQ(g.msgs[msg_index(&g, M_OVERDOSE)].a, old);
	CHECK_EQ(count_msgs(&g, M_CREW_PAID), 0);
	CHECK(g.price[0] > 0 || g.price[1] > 0);	/* the market rolled in the new city */
	for (i = 0; i < NUM_GOODS; i++)
		CHECK_EQ(g.stash[i], 0);
	/* the ban holds: no flight back, nothing spent */
	cash = g.cash;
	CHECK_EQ(game_move(&g, at(old, 0)), E_BANNED);
	CHECK_EQ(g.cash, cash);
	CHECK_EQ(g.day, 3);
	CHECK_EQ(game_city_banned(&g, -1), 0);
	CHECK_EQ(game_city_banned(&g, NUM_CITIES), 0);
}

static void test_overdose_run_is_quiet(void)
{
	Params p = crew_params();
	Game g;
	int old;
	p.cohort_od_permille = 1000;
	p.cop_base_permille = 1000;
	p.rumor_permille = 1000;
	p.rumor_true_permille = 0;
	p.mug_permille = 1000;
	p.rc_offer_permille = 1000;
	p.rc_find_permille = 1000;
	new_test_game(&g, &p, 30);
	stock_up(&g);
	g.p.cop_base_permille = 0;
	g.p.mug_permille = 0;
	g.p.rumor_permille = 0;
	g.p.rc_offer_permille = 0;
	g.p.rc_find_permille = 0;
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	put_stake(&g, 0, 100, g.day + 1, 0);	/* due on the relocation day */
	g.p.cop_base_permille = 1000;
	g.p.mug_permille = 1000;
	g.p.rumor_permille = 1000;
	g.p.rc_offer_permille = 1000;
	g.p.rc_find_permille = 1000;
	old = g.loc.city;
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK(g.loc.city != old);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(count_msgs(&g, M_COPS) + count_msgs(&g, M_MUGGED) + count_msgs(&g, M_RUMOR_JOKE) +
		 count_msgs(&g, M_RUMOR_TIP), 0);
	CHECK_EQ(count_msgs(&g, M_RC_OFFER) + count_msgs(&g, M_RC_FIND), 0);
	CHECK_EQ(g.offer.kind, OFFER_NONE);
	CHECK_EQ(g.nstakes, 1);		/* not resolved on this day */
	g.p.cop_base_permille = 0;
	g.p.mug_permille = 0;
	g.p.rumor_permille = 0;
	g.p.rc_offer_permille = 0;
	g.p.rc_find_permille = 0;
	ride(&g);
	CHECK_EQ(g.nstakes, 0);		/* it resolves on the next arrival */
}

static void test_overdose_picks_only_unbanned_cities(void)
{
	Params p = crew_params();
	uint64_t seed;
	p.cohort_od_permille = 1000;
	for (seed = 1; seed <= 40; seed++) {
		Game g;
		int c;
		game_new(&g, &p, seed, 30, "T");
		for (c = 0; c < NUM_CITIES; c++)
			if (c != 0 && c != 5)
				g.banned |= 1u << c;
		stock_up(&g);
		ride(&g);
		CHECK_EQ(g.offer.kind, OFFER_CREW);
		CHECK_EQ(game_crew_accept(&g), OK);
		CHECK_EQ(g.loc.city, 5);
	}
}

static void test_overdose_spreads_over_cities(void)
{
	Params p = crew_params();
	uint64_t seed;
	int seen[NUM_CITIES];
	int c, kinds = 0;
	p.cohort_od_permille = 1000;
	memset(seen, 0, sizeof seen);
	for (seed = 1; seed <= 300; seed++) {
		Game g;
		game_new(&g, &p, seed, 30, "T");
		stock_up(&g);
		ride(&g);
		game_crew_accept(&g);
		seen[g.loc.city] = 1;
	}
	for (c = 0; c < NUM_CITIES; c++)
		kinds += seen[c];
	CHECK(kinds >= 12);
	CHECK_EQ(seen[0], 0);	/* never the city that banned you */
}

static void test_overdose_on_the_last_day_ends_the_game(void)
{
	Params p = crew_params();
	Game g;
	p.cohort_od_permille = 1000;
	new_test_game(&g, &p, 15);
	stock_up(&g);
	ride(&g);
	CHECK_EQ(g.offer.kind, OFFER_CREW);
	g.day = g.last_day;
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK_EQ(g.phase, PH_OVER);
	CHECK_EQ(g.end, END_DAYS_UP);
	CHECK_EQ(g.day, g.last_day);
	CHECK_EQ(count_msgs(&g, M_OVERDOSE), 1);
	CHECK_EQ(count_msgs(&g, M_GAME_OVER), 1);
	CHECK_EQ(game_carried(&g), 0);
}

static void test_overdose_into_the_last_day(void)
{
	Params p = crew_params();
	Game g;
	p.cohort_od_permille = 1000;
	new_test_game(&g, &p, 15);
	stock_up(&g);
	ride(&g);
	g.day = g.last_day - 1;
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK_EQ(g.day, g.last_day);
	CHECK_EQ(g.phase, PH_TRADING);
	CHECK_EQ(count_msgs(&g, M_LAST_DAY), 1);
}

static void test_outcomes_follow_the_odds(void)
{
	Params p = crew_params();
	int paid = 0, od = 0;
	uint64_t seed;
	for (seed = 1; seed <= 400; seed++) {
		Game g;
		game_new(&g, &p, seed, 30, "T");
		stock_up(&g);
		ride(&g);
		game_crew_accept(&g);
		if (g.banned)
			od++;
		else
			paid++;
	}
	CHECK(od > 180 && od < 260);	/* 550 permille of 400 is 220 */
	CHECK(paid > 140);
}

static void test_ban_stops_flights_only_to_that_city(void)
{
	Params p = test_params();
	Game g;
	new_test_game(&g, &p, 30);
	g.banned = 1u << 4;
	CHECK_EQ(game_move(&g, at(4, 0)), E_BANNED);
	CHECK_EQ(game_move(&g, at(4, 3)), E_BANNED);
	CHECK_EQ(game_move(&g, at(5, 0)), OK);
	CHECK_EQ(game_city_banned(&g, 4), 1);
	CHECK_EQ(game_city_banned(&g, 5), 0);
}


/* --- the end of the run: no offer or find that cannot resolve --- */

static void test_no_offer_or_find_that_cannot_resolve(void)
{
	Params p = test_params();
	Game g;
	int seen_late = 0;
	p.rc_offer_permille = 1000;
	p.rc_find_permille = 1000;
	new_test_game(&g, &p, 15);
	while (g.day < g.last_day) {
		int can = g.day + 1 + p.rc_days_lo <= g.last_day;	/* the arrival is on day + 1 */
		CHECK_EQ(ride(&g), OK);
		if (g.day + p.rc_days_lo > g.last_day) {
			CHECK_EQ(g.offer.kind, OFFER_NONE);
			CHECK_EQ(count_msgs(&g, M_RC_OFFER) + count_msgs(&g, M_RC_FIND), 0);
			CHECK_EQ(game_rc_back(&g, 10), E_PHASE);
			seen_late++;
		} else {
			CHECK(can);
		}
		if (g.offer.kind == OFFER_RC)
			game_offer_pass(&g);
	}
	CHECK_EQ(seen_late, p.rc_days_lo);
}

static void test_new_stakes_resolve_by_the_last_day(void)
{
	Params p = test_params();
	uint64_t seed;
	p.rc_offer_permille = 1000;
	p.rc_find_permille = 1000;
	p.rc_days_lo = 2;
	p.rc_days_hi = 4;
	for (seed = 1; seed <= 50; seed++) {
		Game g;
		int i;
		game_new(&g, &p, seed, 15, "T");
		g.day = g.last_day - 3;		/* the last arrivals with room: lands on last-2 .. last */
		for (i = 0; i < 3; i++) {
			CHECK_EQ(ride(&g), OK);
			if (g.offer.kind == OFFER_RC)
				game_rc_back(&g, 10);
			for (int k = 0; k < g.nstakes; k++)
				CHECK(g.stakes[k].resolve_day <= g.last_day);
		}
	}
}

static void test_no_stake_survives_the_end(void)
{
	Params p = test_params();
	uint64_t seed;
	p.rc_offer_permille = 700;
	p.rc_find_permille = 700;
	for (seed = 1; seed <= 100; seed++) {
		Game g;
		game_new(&g, &p, seed, 15, "T");
		while (g.phase != PH_OVER) {
			if (g.offer.kind == OFFER_RC)
				game_rc_back(&g, 10);
			ride(&g);
		}
		CHECK_EQ(g.end, END_DAYS_UP);
		CHECK_EQ(g.nstakes, 0);
	}
}

static void test_overdose_into_last_day_still_settles_stakes(void)
{
	Params p = crew_params();
	Game g;
	p.cohort_od_permille = 1000;
	new_test_game(&g, &p, 15);
	stock_up(&g);
	ride(&g);
	g.day = g.last_day - 1;
	put_stake(&g, 0, 100, g.last_day, 0);
	CHECK_EQ(game_crew_accept(&g), OK);
	CHECK_EQ(g.day, g.last_day);
	CHECK_EQ(g.nstakes, 1);
	CHECK_EQ(ride(&g), OK);		/* the final move closes the book */
	CHECK_EQ(g.phase, PH_OVER);
	CHECK_EQ(g.nstakes, 0);
}

/* --- saves --- */

static char dir[64];

static void path_in(char *out, size_t n, const char *name)
{
	snprintf(out, n, "%s/%s", dir, name);
}

static long slurp(const char *path, unsigned char *buf, long max)
{
	FILE *f = fopen(path, "rb");
	long n;
	if (!f)
		return -1;
	n = (long)fread(buf, 1, (size_t)max, f);
	fclose(f);
	return n;
}

static void wild_game(Game *g)
{
	Params p = test_params();
	p.rc_offer_permille = 1000;
	p.cohort_offer_permille = 1000;
	game_new(g, &p, 5, 30, "Wild");
	g->banned = (1u << 3) | (1u << 11);
	g->crew_offers = 1;
	g->wild.state ^= 0x1234;
	put_stake(g, 4, 1500, 6, 0);
	put_stake(g, 8, 2500, 7, 1);
	g->stats.wild_wins = 3;
	g->stats.wild_losses = 2;
	ride(g);			/* leaves an offer open */
}

static void test_save_round_trip_keeps_wild_state(void)
{
	static unsigned char a[200000], b[200000];
	char p1[128], p2[128];
	static Game g, g2;
	long na, nb;
	path_in(p1, sizeof p1, "w1.sav");
	path_in(p2, sizeof p2, "w2.sav");
	wild_game(&g);
	CHECK_EQ(g.offer.kind, OFFER_RC);
	CHECK_EQ(save_write(&g, p1), 0);
	memset(&g2, 0x5a, sizeof g2);
	CHECK_EQ(save_read(&g2, p1), 0);
	CHECK_EQ(g2.nstakes, 2);
	CHECK_EQ(g2.stakes[0].chem, 4);
	CHECK_EQ(g2.stakes[0].stake, 1500);
	CHECK_EQ(g2.stakes[0].resolve_day, 6);
	CHECK_EQ(g2.stakes[0].found, 0);
	CHECK_EQ(g2.stakes[1].chem, 8);
	CHECK_EQ(g2.stakes[1].found, 1);
	CHECK_EQ(g2.offer.kind, OFFER_RC);
	CHECK_EQ(g2.offer.chem, g.offer.chem);
	CHECK_EQ(g2.offer.day, g.offer.day);
	CHECK_EQ(g2.crew_offers, 1);
	CHECK_EQ(g2.banned, (1u << 3) | (1u << 11));
	CHECK_EQ(g2.stats.wild_wins, 3);
	CHECK_EQ(g2.stats.wild_losses, 2);
	CHECK(g2.wild.state == g.wild.state && g2.wild.inc == g.wild.inc);
	CHECK_EQ(g2.p.rc_offer_permille, 1000);
	CHECK_EQ(g2.p.cohort_mult_pct, 300);
	CHECK_EQ(save_write(&g2, p2), 0);
	na = slurp(p1, a, sizeof a);
	nb = slurp(p2, b, sizeof b);
	CHECK(na > 16 && na == nb && memcmp(a, b, (size_t)na) == 0);
	/* and the loaded game plays on the same */
	CHECK_EQ(game_rc_back(&g, 100), OK);
	CHECK_EQ(game_rc_back(&g2, 100), OK);
	CHECK_EQ(ride(&g), OK);
	CHECK_EQ(ride(&g2), OK);
	CHECK(g.wild.state == g2.wild.state);
	CHECK_EQ(g.nstakes, g2.nstakes);
	CHECK_EQ(g.cash, g2.cash);
}

static int refused(const Game *g, const char *name)
{
	char p1[128];
	static Game t;
	path_in(p1, sizeof p1, name);
	save_write(g, p1);
	memset(&t, 0, sizeof t);
	return save_read(&t, p1) == -2;
}

static void test_save_refuses_nonsense_wild_state(void)
{
	static Game g;
	CHECK(!refused((wild_game(&g), &g), "ok.sav"));
	wild_game(&g);
	g.loc.city = 3;				/* standing in a banned city */
	CHECK(refused(&g, "b1.sav"));
	wild_game(&g);
	g.banned |= 1u << NUM_CITIES;		/* a city that does not exist */
	CHECK(refused(&g, "b2.sav"));
	wild_game(&g);
	g.nstakes = MAX_STAKES + 1;
	CHECK(refused(&g, "b3.sav"));
	wild_game(&g);
	g.stakes[0].chem = NUM_CHEMS;
	CHECK(refused(&g, "b4.sav"));
	wild_game(&g);
	g.stakes[1].stake = -1;
	CHECK(refused(&g, "b5.sav"));
	wild_game(&g);
	g.stakes[0].found = 2;
	CHECK(refused(&g, "b6.sav"));
	wild_game(&g);
	g.stakes[2].stake = 5;			/* an unused slot must be empty */
	CHECK(refused(&g, "b7.sav"));
	wild_game(&g);
	g.offer.kind = 3;
	CHECK(refused(&g, "b8.sav"));
	wild_game(&g);
	g.offer.chem = NUM_CHEMS;
	CHECK(refused(&g, "b9.sav"));
	wild_game(&g);
	g.offer.day = (int16_t)(g.day + 1);	/* an offer from another day */
	CHECK(refused(&g, "b10.sav"));
	wild_game(&g);
	g.crew_offers = 3;			/* more crews than a run allows */
	CHECK(refused(&g, "b11.sav"));
	wild_game(&g);
	g.stats.wild_wins = -1;
	CHECK(refused(&g, "b12.sav"));
	wild_game(&g);
	g.p.rc_max_stakes = MAX_STAKES + 1;
	CHECK(refused(&g, "b13.sav"));
	wild_game(&g);
	g.p.rc_days_lo = 5;
	g.p.rc_days_hi = 2;
	CHECK(refused(&g, "b14.sav"));
}

static void test_a_banned_city_stays_banned_across_a_load(void)
{
	Params p = crew_params();
	char p1[128];
	static Game g, g2;
	int old;
	p.cohort_od_permille = 1000;
	game_new(&g, &p, 3, 30, "T");
	stock_up(&g);
	ride(&g);
	old = g.loc.city;
	game_crew_accept(&g);
	path_in(p1, sizeof p1, "ban.sav");
	CHECK_EQ(save_write(&g, p1), 0);
	CHECK_EQ(save_read(&g2, p1), 0);
	CHECK_EQ(game_city_banned(&g2, old), 1);
	CHECK_EQ(game_move(&g2, at(old, 0)), E_BANNED);
	CHECK_EQ(g2.crew_offers, 1);
}

/* --- the bot that takes every offer --- */

static void check_invariants(const Game *g)
{
	int i, ok = 1;
	if (g->nstakes < 0 || g->nstakes > MAX_STAKES) ok = 0;
	for (i = 0; ok && i < MAX_STAKES; i++) {
		const Stake *s = &g->stakes[i];
		if (i < g->nstakes) {
			if (s->chem < 0 || s->chem >= NUM_CHEMS || s->stake <= 0 || s->stake > MONEY_MAX ||
			    (s->found != 0 && s->found != 1))
				ok = 0;
		} else if (s->stake != 0 || s->chem != 0 || s->resolve_day != 0 || s->found != 0) {
			ok = 0;
		}
	}
	if (g->cash < 0 || g->cash > MONEY_MAX) ok = 0;
	if (g->crew_offers > g->p.cohort_max) ok = 0;
	if (g->banned >> NUM_CITIES) ok = 0;
	if (g->phase != PH_OVER && game_city_banned(g, g->loc.city)) ok = 0;
	if (g->offer.kind > OFFER_CREW) ok = 0;
	if (g->offer.kind != OFFER_NONE && g->offer.day != g->day) ok = 0;
	CHECK(ok);
}

static void test_wild_bot_thousands_of_games(void)
{
	Params p = params_default();
	static Game g, g2;
	char p1[128];
	int n, seeds_with_od = 0, crews = 0, stakes_seen = 0;
	path_in(p1, sizeof p1, "bot.sav");
	for (n = 0; n < 3000; n++) {
		int steps = 0, day = 0;
		game_new(&g, &p, 1000 + (uint64_t)n, n % 2 ? 30 : 60, "Bot");
		while (g.phase != PH_OVER && steps++ < 2000) {
			bot_play_day(&g, BOT_WILD);
			check_invariants(&g);
			if (g.nstakes)
				stakes_seen++;
			if (n % 15 == 0 && g.day != day && g.phase != PH_OVER) {
				static unsigned char a[200000], b[200000];
				char p2[128];
				long na, nb;
				path_in(p2, sizeof p2, "bot2.sav");
				if (save_write(&g, p1) != 0 || save_read(&g2, p1) != 0) {
					CHECK(0);
					return;
				}
				save_write(&g2, p2);
				na = slurp(p1, a, sizeof a);
				nb = slurp(p2, b, sizeof b);
				CHECK(na == nb && na > 0 && memcmp(a, b, (size_t)na) == 0);
			}
			day = g.day;
		}
		CHECK(g.phase == PH_OVER);
		crews += g.crew_offers;
		seeds_with_od += g.banned != 0;
	}
	CHECK(stakes_seen > 1000);
	CHECK(crews > 100);
	CHECK(seeds_with_od > 20);
}

static void test_other_bots_never_take_offers(void)
{
	Params p = params_default();
	static Game g;
	uint64_t seed;
	for (seed = 1; seed <= 100; seed++) {
		int steps = 0;
		game_new(&g, &p, seed, 30, "Bot");
		while (g.phase != PH_OVER && steps++ < 2000) {
			int i;
			bot_play_day(&g, seed % 2 ? BOT_GREEDY : BOT_CAUTIOUS);
			for (i = 0; i < g.nstakes; i++)
				CHECK_EQ(g.stakes[i].found, 1);	/* only free finds: it never backs a chemical */
		}
		CHECK_EQ(g.banned, 0);
	}
}

void tests_wild(void)
{
	snprintf(dir, sizeof dir, "/tmp/ph-wild-XXXXXX");
	if (!mkdtemp(dir)) {
		CHECK(0);
		return;
	}
	test_offer_appears_and_expires();
	test_no_offer_when_chance_zero();
	test_back_takes_cash_and_makes_stake();
	test_back_everything();
	test_pass_declines();
	test_wild_actions_need_trading();
	test_no_offer_when_stakes_full();
	test_offer_uses_a_slot_freed_this_arrival();
	test_stake_resolves_on_its_day();
	test_stake_pays_exact_multiple_or_nothing();
	test_found_stake_bust_costs_nothing();
	test_two_stakes_resolve_independently();
	test_stake_pays_on_late_arrival_after_overdue();
	test_find_is_a_free_stake();
	test_never_more_than_three_stakes();
	test_find_leaves_the_slot_for_an_offer();
	test_arrival_order();
	test_no_offer_during_a_chase_but_stakes_resolve();
	test_separate_random_stream();
	test_crew_needs_a_big_stash();
	test_crew_at_most_twice_even_declined();
	test_crew_beats_chemist_then_chemist_returns();
	test_crew_payout();
	test_crew_payout_at_least_the_minimum();
	test_crew_accept_needs_the_stash_still();
	test_overdose();
	test_overdose_run_is_quiet();
	test_overdose_picks_only_unbanned_cities();
	test_overdose_spreads_over_cities();
	test_overdose_on_the_last_day_ends_the_game();
	test_overdose_into_the_last_day();
	test_outcomes_follow_the_odds();
	test_ban_stops_flights_only_to_that_city();
	test_no_offer_or_find_that_cannot_resolve();
	test_new_stakes_resolve_by_the_last_day();
	test_no_stake_survives_the_end();
	test_overdose_into_last_day_still_settles_stakes();
	test_save_round_trip_keeps_wild_state();
	test_save_refuses_nonsense_wild_state();
	test_a_banned_city_stays_banned_across_a_load();
	test_wild_bot_thousands_of_games();
	test_other_bots_never_take_offers();
}
