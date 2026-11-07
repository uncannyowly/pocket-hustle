/* The daily market at the player's location: prices, spikes, crashes, history. */
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

void market_roll(Game *g)
{
	const Params *p = &g->p;
	int i, priced = 0;

	for (i = 0; i < NUM_GOODS; i++) {
		int32_t price = 0;

		if (!rng_permille(&g->rng, p->absent_permille)) {
			price = base_price(g, i);
			if (rng_permille(&g->rng, p->spike_permille)) {
				price = scale(price, rng_range(&g->rng, p->spike_lo_pct, p->spike_hi_pct));
				game_msg(g, M_SPIKE, (int16_t)i, 0, 0, price);
				g->stats.spikes_seen++;
			} else if (rng_permille(&g->rng, p->crash_permille)) {
				price = scale(price, rng_range(&g->rng, p->crash_lo_pct, p->crash_hi_pct));
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

	for (i = 0; i < NUM_GOODS; i++)
		g->hist[i][g->day] = g->price[i];
}
