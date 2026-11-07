/* Cop chases: run, fight or give up. Giving up means a night in jail. */
#include "core/game_internal.h"

int64_t game_stash_value(const Game *g)
{
	int64_t v = 0;
	int i;

	for (i = 0; i < NUM_GOODS; i++)
		v += (int64_t)g->stash[i] * ((GOODS[i].lo + GOODS[i].hi) / 2);
	return v;
}

/* Called on arrival when no goons came. Returns 1 if a chase started. */
int cops_check(Game *g)
{
	const Params *p = &g->p;
	int64_t chance = p->cop_base_permille + (int64_t)p->cop_per_10k_permille * (game_stash_value(g) / 10000);

	if (chance > p->cop_max_permille)
		chance = p->cop_max_permille;
	if (!rng_permille(&g->rng, (int32_t)chance))
		return 0;
	g->chase.posse = (int16_t)(p->posse_start + p->posse_growth * g->stats.cop_chases);
	g->chase.cops_left = (int16_t)(g->chase.posse + 1);
	g->stats.cop_chases++;
	g->phase = PH_CHASE;
	game_msg(g, M_COPS, g->chase.posse, 0, 0, 0);
	return 1;
}

static void cops_shoot(Game *g)
{
	const Params *p = &g->p;
	int shooters = g->chase.cops_left < p->max_shooters ? g->chase.cops_left : p->max_shooters, i;

	for (i = 0; i < shooters; i++) {
		if (rng_permille(&g->rng, p->cop_hit_permille)) {
			int32_t dmg = rng_range(&g->rng, p->cop_dmg_lo, p->cop_dmg_hi);
			g->health = (int16_t)(g->health - dmg);
			game_msg(g, M_HIT_YOU, 0, 0, 0, dmg);
		} else {
			game_msg(g, M_COPS_MISSED, 0, 0, 0, 0);
		}
	}
	game_check_death(g);
}

static void maybe_drop(Game *g)
{
	const Params *p = &g->p;
	int start, k;

	if (game_carried(g) <= 0 || !rng_permille(&g->rng, p->drop_permille))
		return;
	start = (int)rng_below(&g->rng, NUM_GOODS);
	for (k = 0; k < NUM_GOODS; k++) {
		int i = (start + k) % NUM_GOODS;
		int32_t lost;
		if (g->stash[i] <= 0)
			continue;
		lost = (int32_t)((int64_t)g->stash[i] * rng_range(&g->rng, p->drop_lo_pct, p->drop_hi_pct) / 100);
		if (lost < 1)
			lost = 1;
		if (lost > g->stash[i])
			lost = g->stash[i];
		g->stash_paid[i] -= g->stash_paid[i] * lost / g->stash[i];
		g->stash[i] -= lost;
		game_msg(g, M_DROPPED, (int16_t)i, 0, 0, lost);
		g->stats.times_dropped++;
		return;
	}
}

static void go_to_jail(Game *g)
{
	int64_t fine = g->cash * g->p.jail_fine_pct / 100;
	int i;

	for (i = 0; i < NUM_GOODS; i++) {
		g->stash[i] = 0;
		g->stash_paid[i] = 0;
	}
	g->cash -= fine;
	g->stats.times_jailed++;
	game_msg(g, M_JAILED, 0, 0, 0, fine);
	g->chase.cops_left = 0;
	if (g->day >= g->last_day) {
		g->phase = PH_OVER;
		g->end = END_DAYS_UP;
		game_msg(g, M_GAME_OVER, 0, 0, 0, 0);
		return;
	}
	game_advance_day(g);
	g->phase = PH_TRADING;
	market_roll(g);
	if (g->day == g->last_day)
		game_msg(g, M_LAST_DAY, 0, 0, 0, 0);
}

int game_chase(Game *g, ChaseAction a)
{
	if (g->phase != PH_CHASE)
		return E_PHASE;
	if (a != CH_RUN && a != CH_FIGHT && a != CH_SURRENDER)
		return E_BAD_ARG;
	if (a == CH_FIGHT && (g->weapon < 0 || g->weapon >= NUM_GEAR))
		return E_BAD_ARG;

	game_clear_msgs(g);
	switch (a) {
	case CH_RUN:
		if (rng_permille(&g->rng, g->p.run_permille)) {
			game_msg(g, M_ESCAPED, 0, 0, 0, 0);
			g->stats.escapes++;
			g->chase.cops_left = 0;
			g->phase = PH_TRADING;
			maybe_drop(g);
		} else {
			cops_shoot(g);
		}
		break;
	case CH_FIGHT:
		if (rng_permille(&g->rng, GEAR[g->weapon].power)) {
			g->chase.cops_left--;
			game_msg(g, M_YOU_HIT, g->chase.cops_left, 0, 0, 0);
		} else {
			game_msg(g, M_YOU_MISSED, 0, 0, 0, 0);
		}
		if (g->chase.cops_left <= 0) {
			int64_t loot = rng_range(&g->rng, (int32_t)g->p.loot_lo, (int32_t)g->p.loot_hi);
			g->cash += loot;
			if (g->cash > MONEY_MAX)
				g->cash = MONEY_MAX;
			g->stats.cops_beaten++;
			game_msg(g, M_COPS_BEATEN, 0, 0, 0, loot);
			g->phase = PH_TRADING;
		} else {
			cops_shoot(g);
		}
		break;
	case CH_SURRENDER:
		go_to_jail(g);
		break;
	}
	return OK;
}
