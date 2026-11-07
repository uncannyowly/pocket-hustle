/* The rumor-monger: sometimes a true tip about a coming spike or crash, mostly jokes. */
#include "core/game_internal.h"

void rumor_check(Game *g)
{
	const Params *p = &g->p;

	if (!rng_permille(&g->rng, p->rumor_permille))
		return;
	if (rng_permille(&g->rng, p->rumor_true_permille) && g->tip.day == 0) {
		int32_t lead = rng_range(&g->rng, p->rumor_lead_lo, p->rumor_lead_hi);
		if (g->day + lead <= g->last_day) {
			int here = g->loc.city * HOODS_PER_CITY + g->loc.hood;
			int idx = (int)rng_below(&g->rng, NUM_LOCS - 1);
			if (idx >= here)
				idx++;
			g->tip.day = (int16_t)(g->day + lead);
			g->tip.loc.city = (uint8_t)(idx / HOODS_PER_CITY);
			g->tip.loc.hood = (uint8_t)(idx % HOODS_PER_CITY);
			g->tip.good = (int8_t)rng_below(&g->rng, NUM_GOODS);
			g->tip.spike = (int8_t)rng_below(&g->rng, 2);
			g->stats.rumors_true++;
			game_msg(g, M_RUMOR_TIP, g->tip.good, (int16_t)idx, g->tip.spike, g->tip.day);
			return;
		}
	}
	g->stats.rumors_joke++;
	game_msg(g, M_RUMOR_JOKE, g->loc.city, 0, 0, 0);
}
