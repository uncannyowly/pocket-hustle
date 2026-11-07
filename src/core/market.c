/* The daily market at the player's location: prices, spikes, crashes, history. */
#include <string.h>

#include "core/game_internal.h"

static int32_t base_price(Game *g, int good)
{
	int64_t b = (int64_t)rng_range(&g->rng, GOODS[good].lo, GOODS[good].hi)
		    * CITIES[g->loc.city].bias_pct[good] / 100;
	return b < 1 ? 1 : (int32_t)b;
}

static int32_t scale(int32_t price, int32_t pct)
{
	int64_t v = (int64_t)price * pct / 100;
	if (v < 1)
		return 1;
	return v > INT32_MAX ? INT32_MAX : (int32_t)v;
}

/* A true tip for today, here: that good is on sale with the promised spike or crash. */
static int tip_good_here(const Game *g)
{
	if (g->tip.day == 0 || g->tip.day != g->day ||
	    g->tip.loc.city != g->loc.city || g->tip.loc.hood != g->loc.hood)
		return -1;
	return g->tip.good;
}

void market_roll(Game *g)
{
	const Params *p = &g->p;
	int i, priced = 0, tipped = tip_good_here(g);
	int8_t ev[NUM_GOODS];

	for (i = 0; i < NUM_GOODS; i++) {
		int32_t price = 0;

		ev[i] = EV_NONE;
		if (i == tipped) {
			price = base_price(g, i);
			if (g->tip.spike) {
				price = scale(price, rng_range(&g->rng, p->spike_lo_pct, p->spike_hi_pct));
				ev[i] = EV_SPIKE;
				game_msg(g, M_SPIKE, (int16_t)i, 0, 0, price);
				g->stats.spikes_seen++;
			} else {
				price = scale(price, rng_range(&g->rng, p->crash_lo_pct, p->crash_hi_pct));
				ev[i] = EV_CRASH;
				game_msg(g, M_CRASH, (int16_t)i, 0, 0, price);
				g->stats.crashes_seen++;
			}
		} else if (!rng_permille(&g->rng, p->absent_permille)) {
			price = base_price(g, i);
			if (rng_permille(&g->rng, p->spike_permille)) {
				price = scale(price, rng_range(&g->rng, p->spike_lo_pct, p->spike_hi_pct));
				ev[i] = EV_SPIKE;
				game_msg(g, M_SPIKE, (int16_t)i, 0, 0, price);
				g->stats.spikes_seen++;
			} else if (rng_permille(&g->rng, p->crash_permille)) {
				price = scale(price, rng_range(&g->rng, p->crash_lo_pct, p->crash_hi_pct));
				ev[i] = EV_CRASH;
				game_msg(g, M_CRASH, (int16_t)i, 0, 0, price);
				g->stats.crashes_seen++;
			}
		}
		g->price[i] = price;
		if (price)
			priced++;
	}

	if (priced < p->min_goods) {
		int start = (int)rng_below(&g->rng, NUM_GOODS), k;
		for (k = 0; k < NUM_GOODS && priced < p->min_goods; k++) {
			int j = (start + k) % NUM_GOODS;
			if (g->price[j] == 0) {
				g->price[j] = base_price(g, j);
				priced++;
			}
		}
	}

	for (i = 0; i < NUM_GOODS; i++) {
		g->hist[i][g->day] = g->price[i];
		g->hist_ev[i][g->day] = g->price[i] > 0 ? ev[i] : EV_NONE;
	}
	if (tipped >= 0)
		memset(&g->tip, 0, sizeof g->tip);
}
