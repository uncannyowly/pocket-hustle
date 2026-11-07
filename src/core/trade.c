/* Buying and selling at today's market. */
#include "core/game_internal.h"

int32_t game_carried(const Game *g)
{
	int32_t n = 0;
	int i;

	for (i = 0; i < NUM_GOODS; i++)
		n += g->stash[i];
	return n;
}

int32_t game_max_buy(const Game *g, int good)
{
	int64_t by_cash, by_space;

	if (good < 0 || good >= NUM_GOODS || g->price[good] <= 0)
		return 0;
	by_cash = g->cash / g->price[good];
	by_space = (int64_t)g->capacity - game_carried(g);
	if (by_space < 0)
		by_space = 0;
	return (int32_t)(by_cash < by_space ? by_cash : by_space);
}

int game_buy(Game *g, int good, int32_t qty)
{
	int64_t total;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (good < 0 || good >= NUM_GOODS || qty <= 0)
		return E_BAD_ARG;
	if (g->price[good] <= 0)
		return E_NOT_SOLD;
	total = (int64_t)qty * g->price[good];
	if (total > g->cash)
		return E_NO_CASH;
	if ((int64_t)game_carried(g) + qty > g->capacity)
		return E_NO_SPACE;

	game_clear_msgs(g);
	g->cash -= total;
	g->stash[good] += qty;
	g->stash_paid[good] += total;
	return OK;
}

int game_sell(Game *g, int good, int32_t qty)
{
	int64_t total;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (good < 0 || good >= NUM_GOODS || qty <= 0)
		return E_BAD_ARG;
	if (g->price[good] <= 0)
		return E_NOT_SOLD;
	if (qty > g->stash[good])
		return E_NO_STOCK;

	game_clear_msgs(g);
	total = (int64_t)qty * g->price[good];
	g->stash_paid[good] -= g->stash_paid[good] * qty / g->stash[good];
	g->stash[good] -= qty;
	g->cash += total;
	if (g->cash > MONEY_MAX)
		g->cash = MONEY_MAX;
	return OK;
}
