/* Getting around: transit inside a city, flights between cities, the final day. */
#include "core/game_internal.h"

int game_flight_crosses(const Game *g, int city)
{
	if (city < 0 || city >= NUM_CITIES || g->loc.city >= NUM_CITIES || city == g->loc.city)
		return 0;
	return CITIES[city].country != CITIES[g->loc.city].country;
}

int64_t game_flight_cost(const Game *g, int city)
{
	uint64_t h;
	int64_t span, lo, hi;

	if (city == g->loc.city)
		return 0;
	if (game_flight_crosses(g, city)) {
		lo = g->p.intl_flight_lo;
		hi = g->p.intl_flight_hi;
	} else {
		lo = g->p.flight_lo;
		hi = g->p.flight_hi;
	}
	span = hi - lo + 1;
	if (span <= 0)
		return lo;
	h = mix64(g->seed ^ ((uint64_t)(uint16_t)g->day << 16) ^ ((uint64_t)g->loc.city << 8) ^ (uint64_t)city);
	return lo + (int64_t)(h % (uint64_t)span);
}

int game_move(Game *g, Loc dest)
{
	int64_t cost;
	int flying, crossing;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (dest.city >= NUM_CITIES || dest.hood >= HOODS_PER_CITY ||
	    (dest.city == g->loc.city && dest.hood == g->loc.hood))
		return E_BAD_ARG;
	if (game_city_banned(g, dest.city))
		return E_BANNED;
	flying = dest.city != g->loc.city;
	crossing = flying && game_flight_crosses(g, dest.city);
	cost = flying ? game_flight_cost(g, dest.city) : 0;
	if (cost > g->cash)
		return E_NO_CASH;

	game_clear_msgs(g);
	wild_expire(g);
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
		game_msg(g, M_FLEW, dest.city, 0, crossing, cost);
	} else {
		game_msg(g, M_RODE, dest.hood, 0, 0, 0);
	}

	market_roll(g);
	if (g->day == g->last_day)
		game_msg(g, M_LAST_DAY, 0, 0, 0, 0);
	wild_resolve(g);
	encounters_on_arrival(g);
	wild_offers(g);
	return OK;
}
