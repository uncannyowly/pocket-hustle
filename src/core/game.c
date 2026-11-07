/* Game setup, net worth and the message log. */
#include <string.h>

#include "core/game_internal.h"

int game_new(Game *g, const Params *p, uint64_t seed, int days, const char *name)
{
	if (days != 15 && days != 30 && days != 60 && days != 90)
		return E_BAD_ARG;
	if (p->start_city < 0 || p->start_city >= NUM_CITIES)
		return E_BAD_ARG;

	memset(g, 0, sizeof *g);
	memcpy(&g->p, p, sizeof *p);
	g->seed = seed;
	rng_seed(&g->rng, seed, 1);
	rng_seed(&g->flavor, seed, 2);
	if (name)
		strncpy(g->name, name, sizeof g->name - 1);

	g->day = 1;
	g->last_day = (int16_t)days;
	g->loc.city = (uint8_t)p->start_city;
	g->loc.hood = 0;
	g->cash = p->start_cash;
	g->debt[p->start_city] = p->start_debt;
	g->capacity = p->start_capacity;
	g->health = 100;
	g->weapon = -1;
	g->phase = PH_TRADING;
	g->end = END_NONE;
	g->stats.best_net_worth = game_net_worth(g);

	market_roll(g);
	return OK;
}

int64_t game_net_worth(const Game *g)
{
	int64_t v = g->cash + g->bank;
	int c;

	for (c = 0; c < NUM_CITIES; c++)
		v -= g->debt[c];
	return v;
}

void game_msg(Game *g, MsgKind k, int16_t a, int16_t b, int16_t c, int64_t amount)
{
	Msg *m;

	if (g->nmsgs >= MAX_MSGS)
		return;
	m = &g->msgs[g->nmsgs++];
	m->kind = (uint8_t)k;
	m->variant = (uint16_t)(rng_u32(&g->flavor) & 0xffffu);
	m->a = a;
	m->b = b;
	m->c = c;
	m->amount = amount;
}

void game_clear_msgs(Game *g)
{
	g->nmsgs = 0;
}

uint64_t mix64(uint64_t x)
{
	x ^= x >> 30;
	x *= 0xbf58476d1ce4e5b9ULL;
	x ^= x >> 27;
	x *= 0x94d049bb133111ebULL;
	x ^= x >> 31;
	return x;
}
