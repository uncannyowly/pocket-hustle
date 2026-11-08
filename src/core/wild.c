/* Wild cards, finds and the shady crew. All their randomness comes from g->wild, so the
 * gameplay streams (g->rng, g->flavor draws aside from messages) never notice them. */
#include <string.h>

#include "core/game_internal.h"

static int64_t clamp_money(int64_t v)
{
	return v > MONEY_MAX ? MONEY_MAX : v;
}

int game_city_banned(const Game *g, int city)
{
	return city >= 0 && city < NUM_CITIES && ((g->banned >> city) & 1u) != 0;
}

void wild_expire(Game *g)
{
	g->offer.kind = OFFER_NONE;
	g->offer.chem = 0;
	g->offer.day = 0;
}

static void add_stake(Game *g, int chem, int64_t stake, int found)
{
	Stake *s = &g->stakes[g->nstakes++];

	memset(s, 0, sizeof *s);
	s->chem = (int8_t)chem;
	s->stake = stake;
	s->found = (int8_t)found;
	s->resolve_day = (int16_t)(g->day + rng_range(&g->wild, g->p.rc_days_lo, g->p.rc_days_hi));
	if (s->resolve_day > g->last_day)	/* the last arrival settles every stake */
		s->resolve_day = g->last_day;
}

/* Every stake due on or before today pays or busts, once, and leaves its slot. */
void wild_resolve(Game *g)
{
	int i = 0;

	while (i < g->nstakes) {
		Stake s = g->stakes[i];
		const ChemDef *c;

		if (s.resolve_day > g->day) {
			i++;
			continue;
		}
		memmove(&g->stakes[i], &g->stakes[i + 1], (size_t)(g->nstakes - i - 1) * sizeof g->stakes[0]);
		memset(&g->stakes[g->nstakes - 1], 0, sizeof g->stakes[0]);
		g->nstakes--;
		c = &CHEMS[s.chem];
		if (rng_permille(&g->wild, c->odds_permille)) {
			int64_t pay = clamp_money(s.stake * rng_range(&g->wild, c->mult_lo_pct, c->mult_hi_pct) / 100);
			g->cash = clamp_money(g->cash + pay);
			g->stats.wild_wins++;
			game_msg(g, M_RC_HIT, s.chem, 0, 0, pay);
		} else {
			g->stats.wild_losses++;
			if (s.found)
				game_msg(g, M_RC_BUST_FOUND, s.chem, 0, 0, 0);
			else
				game_msg(g, M_RC_BUST, s.chem, 0, 0, s.stake);
		}
	}
}

void wild_offers(Game *g)
{
	const Params *p = &g->p;
	int can_resolve = g->day + p->rc_days_lo <= g->last_day;

	if (g->phase != PH_TRADING)
		return;
	if (g->crew_offers < p->cohort_max && game_stash_value(g) >= p->cohort_min_stash &&
	    rng_permille(&g->wild, p->cohort_offer_permille)) {
		g->crew_offers++;
		g->offer.kind = OFFER_CREW;
		g->offer.chem = 0;
		g->offer.day = g->day;
		game_msg(g, M_CREW_OFFER, 0, 0, 0, 0);
	} else if (can_resolve && g->nstakes < p->rc_max_stakes && rng_permille(&g->wild, p->rc_offer_permille)) {
		g->offer.kind = OFFER_RC;
		g->offer.chem = (int8_t)rng_below(&g->wild, NUM_CHEMS);
		g->offer.day = g->day;
		game_msg(g, M_RC_OFFER, g->offer.chem, 0, 0, 0);
	}
	/* an open chemical offer keeps its slot, so backing it can never fail for room */
	if (can_resolve && g->nstakes + (g->offer.kind == OFFER_RC ? 1 : 0) < p->rc_max_stakes &&
	    rng_permille(&g->wild, p->rc_find_permille)) {
		int chem = (int)rng_below(&g->wild, NUM_CHEMS);
		int64_t value = rng_range(&g->wild, (int32_t)p->rc_find_lo, (int32_t)p->rc_find_hi);

		add_stake(g, chem, value, 1);
		game_msg(g, M_RC_FIND, (int16_t)chem, 0, 0, value);
	}
}

int game_rc_back(Game *g, int64_t stake)
{
	if (g->phase != PH_TRADING || g->offer.kind != OFFER_RC)
		return E_PHASE;
	if (stake <= 0)
		return E_BAD_ARG;
	if (stake > g->cash)
		return E_NO_CASH;
	if (g->nstakes >= MAX_STAKES)
		return E_LIMIT;
	game_clear_msgs(g);
	g->cash -= stake;
	add_stake(g, g->offer.chem, stake, 0);
	wild_expire(g);
	return OK;
}

int game_offer_pass(Game *g)
{
	if (g->phase != PH_TRADING || g->offer.kind == OFFER_NONE)
		return E_PHASE;
	game_clear_msgs(g);
	wild_expire(g);
	return OK;
}

/* The stash at today's prices; a good this market doesn't buy counts at its mid price. */
static int64_t stash_value_today(const Game *g)
{
	int64_t v = 0;
	int i;

	for (i = 0; i < NUM_GOODS; i++) {
		int64_t price = g->price[i] > 0 ? g->price[i] : (GOODS[i].lo + GOODS[i].hi) / 2;
		v += g->stash[i] * price;
	}
	return v;
}

int game_crew_accept(Game *g)
{
	int64_t value, pay;
	int i;

	if (g->phase != PH_TRADING || g->offer.kind != OFFER_CREW)
		return E_PHASE;
	if (game_stash_value(g) < g->p.cohort_min_stash)
		return E_NO_STOCK;
	game_clear_msgs(g);
	wild_expire(g);
	value = stash_value_today(g);
	for (i = 0; i < NUM_GOODS; i++) {
		g->stash[i] = 0;
		g->stash_paid[i] = 0;
	}
	if (rng_permille(&g->wild, 1000 - g->p.cohort_od_permille)) {
		pay = clamp_money(value * g->p.cohort_mult_pct / 100);
		if (pay < g->p.cohort_min_payout)
			pay = g->p.cohort_min_payout;
		g->cash = clamp_money(g->cash + pay);
		game_msg(g, M_CREW_PAID, 0, 0, 0, pay);
		return OK;
	}

	/* People overdosed: this city is closed to you for good, and the crew drives you out. */
	{
		int old = g->loc.city, pick = -1, n = 0, c;

		g->banned |= 1u << old;
		game_msg(g, M_OVERDOSE, (int16_t)old, 0, 0, 0);
		for (c = 0; c < NUM_CITIES; c++)
			if (!game_city_banned(g, c))
				n++;
		if (g->day >= g->last_day || n == 0) {	/* the last day, or nowhere left to go */
			wild_resolve(g);
			g->phase = PH_OVER;
			g->end = END_DAYS_UP;
			game_msg(g, M_GAME_OVER, 0, 0, 0, 0);
			return OK;
		}
		{
			int want = (int)rng_below(&g->wild, (uint32_t)n);
			for (c = 0; c < NUM_CITIES; c++)
				if (!game_city_banned(g, c) && want-- == 0) {
					pick = c;
					break;
				}
		}
		game_advance_day(g);
		g->loc.city = (uint8_t)pick;
		g->loc.hood = 0;
		g->stats.moves++;
		g->stats.flights++;
		market_roll(g);
		if (g->debt[old] > 0) {		/* the old lender sells the debt to the new city's */
			if (g->debt[pick] <= 0)
				g->debt_age[pick] = 0;
			g->debt[pick] = clamp_money(g->debt[pick] + g->debt[old]);
			g->debt[old] = 0;
			g->debt_age[old] = 0;
			game_msg(g, M_DEBT_SOLD, (int16_t)pick, 0, 0, g->debt[pick]);
		}
		if (g->day == g->last_day)
			game_msg(g, M_LAST_DAY, 0, 0, 0, 0);
	}
	return OK;
}
