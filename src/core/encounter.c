/* What happens when you arrive somewhere: goons first, else cops, else a mugging.
 * At most one kind of trouble per arrival. Then, if no chase started, maybe a rumor. */
#include "core/game_internal.h"

/* An overdue lender's goons find you in their city. Returns 1 if they came. */
static int goons_check(Game *g)
{
	int c = g->loc.city;
	int32_t dmg;

	if (g->debt[c] <= 0 || g->debt_age[c] <= g->p.enforce_days)
		return 0;
	dmg = rng_range(&g->rng, g->p.enforce_dmg_lo, g->p.enforce_dmg_hi);
	g->health = (int16_t)(g->health - dmg);
	game_msg(g, M_GOONS, (int16_t)c, 0, 0, dmg);
	g->stats.goon_visits++;
	g->debt_age[c] = 0;
	game_check_death(g);
	return 1;
}

/* Pocket cash only; the bank is safe. Returns 1 if it happened. */
static int mug_check(Game *g)
{
	int64_t lost;

	if (g->cash <= 0 || !rng_permille(&g->rng, g->p.mug_permille))
		return 0;
	lost = g->cash * rng_range(&g->rng, g->p.mug_lo_pct, g->p.mug_hi_pct) / 100;
	g->cash -= lost;
	g->stats.times_mugged++;
	game_msg(g, M_MUGGED, 0, 0, 0, lost);
	return 1;
}

void encounters_on_arrival(Game *g)
{
	if (!goons_check(g) && !cops_check(g))
		mug_check(g);
	if (g->phase == PH_TRADING)
		rumor_check(g);
}
