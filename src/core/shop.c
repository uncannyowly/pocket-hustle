/* The hospital and the store. */
#include "core/game_internal.h"

int game_heal(Game *g, int32_t hp)
{
	int64_t cost;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (hp <= 0)
		return E_BAD_ARG;
	if (g->health + hp > 100)
		return E_LIMIT;
	cost = (int64_t)hp * g->p.hospital_per_hp;
	if (cost > g->cash)
		return E_NO_CASH;

	game_clear_msgs(g);
	g->cash -= cost;
	g->health = (int16_t)(g->health + hp);
	g->stats.hospital_spent += cost;
	game_msg(g, M_HEALED, 0, 0, 0, hp);
	return OK;
}

int game_gear_available(const Game *g, int gear)
{
	if (gear < 0 || gear >= NUM_GEAR)
		return 0;
	return GEAR[gear].city < 0 || GEAR[gear].city == g->loc.city;
}

int game_buy_gear(Game *g, int gear)
{
	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (gear < 0 || gear >= NUM_GEAR)
		return E_BAD_ARG;
	if (!game_gear_available(g, gear))
		return E_NOT_SOLD;
	if (GEAR[gear].kind == GEAR_SPACE ? g->gear_owned[gear] != 0 : g->weapon == gear)
		return E_LIMIT;
	if (GEAR[gear].price > g->cash)
		return E_NO_CASH;

	game_clear_msgs(g);
	g->cash -= GEAR[gear].price;
	g->gear_owned[gear] = 1;
	if (GEAR[gear].kind == GEAR_SPACE)
		g->capacity += GEAR[gear].power;
	else
		g->weapon = (int8_t)gear;
	game_msg(g, M_BOUGHT_GEAR, (int16_t)gear, 0, 0, 0);
	return OK;
}
