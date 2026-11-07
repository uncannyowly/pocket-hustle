/* What happens when you arrive somewhere: goons first, else cops, else a mugging
 * (Task 7); then maybe a rumor (Task 8). At most one kind of trouble per arrival. */
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

void encounters_on_arrival(Game *g)
{
	if (goons_check(g))
		return;
}
