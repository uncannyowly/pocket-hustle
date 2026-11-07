/* Getting around: transit inside a city, flights between cities, the final day. */
#include "core/game_internal.h"

int64_t game_flight_cost(const Game *g, int city)
{
	uint64_t h;
	int64_t span;

	if (city == g->loc.city)
		return 0;
	span = g->p.flight_hi - g->p.flight_lo + 1;
	if (span <= 0)
		return g->p.flight_lo;
	h = mix64(g->seed ^ ((uint64_t)(uint16_t)g->day << 16) ^ ((uint64_t)g->loc.city << 8) ^ (uint64_t)city);
	return g->p.flight_lo + (int64_t)(h % (uint64_t)span);
}

int game_move(Game *g, Loc dest)
{
	int64_t cost;
	int flying;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (dest.city >= NUM_CITIES || dest.hood >= HOODS_PER_CITY ||
	    (dest.city == g->loc.city && dest.hood == g->loc.hood))
		return E_BAD_ARG;
	flying = dest.city != g->loc.city;
	cost = flying ? game_flight_cost(g, dest.city) : 0;
	if (cost > g->cash)
		return E_NO_CASH;

	game_clear_msgs(g);
	if (g->day >= g->last_day) {
		g->phase = PH_OVER;
		g->end = END_DAYS_UP;
		game_msg(g, M_GAME_OVER, 0, 0, 0, 0);
		return OK;
	}

	g->cash -= cost;
	game_advance_day(g);
	g->loc = dest;
	g->stats.moves++;
	if (flying) {
		g->stats.flights++;
		game_msg(g, M_FLEW, dest.city, 0, 0, cost);
	} else {
		game_msg(g, M_RODE, dest.hood, 0, 0, 0);
	}

	market_roll(g);
	if (g->day == g->last_day)
		game_msg(g, M_LAST_DAY, 0, 0, 0, 0);
	encounters_on_arrival(g);
	return OK;
}
