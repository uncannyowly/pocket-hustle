/* ph-sim bots. They see only what a player sees (today's prices, their own state). */
#include "bots.h"

static Rng bot_rng;
static uint64_t bot_seed_for = ~0ULL;

static void seed_for(const Game *g)
{
	if (bot_seed_for != g->seed) {
		rng_seed(&bot_rng, g->seed, 99);
		bot_seed_for = g->seed;
	}
}

static int64_t avg_paid(const Game *g, int i)
{
	return g->stash[i] > 0 ? g->stash_paid[i] / g->stash[i] : 0;
}

static void sell_all(Game *g, int i)
{
	if (g->stash[i] > 0 && g->price[i] > 0)
		game_sell(g, i, g->stash[i]);
}

static void chase_action(Game *g, BotKind k)
{
	if (k == BOT_GREEDY)
		game_chase(g, g->weapon >= 0 && g->health >= 50 ? CH_FIGHT : CH_RUN);
	else
		game_chase(g, g->health < 30 ? CH_SURRENDER : CH_RUN);
}

static void buy_gear(Game *g)
{
	int i, best = -1;

	if (g->day < 3)
		return;
	for (i = 0; i < NUM_GEAR; i++) {
		if (!game_gear_available(g, i) || GEAR[i].price > g->cash)
			continue;
		if (GEAR[i].kind == GEAR_WEAPON) {
			int cur = g->weapon >= 0 ? GEAR[g->weapon].power : 0;
			if (GEAR[i].power > cur && (best < 0 || GEAR[i].power > GEAR[best].power))
				best = i;
		} else if (!g->gear_owned[i] && g->cash > 4LL * GEAR[i].price) {
			game_buy_gear(g, i);
		}
	}
	if (best >= 0 && g->cash > 2LL * GEAR[best].price)
		game_buy_gear(g, best);
}

static void trade(Game *g, BotKind k)
{
	int i, pick = -1, c = g->loc.city;
	int64_t best_num = 0, best_den = 1;

	/* sell: at a good markup, or everything on the last day */
	for (i = 0; i < NUM_GOODS; i++) {
		if (g->stash[i] <= 0 || g->price[i] <= 0)
			continue;
		if (g->day == g->last_day || g->price[i] * 10 >= avg_paid(g, i) * 13)
			sell_all(g, i);
	}

	if (g->health < 40) {
		int32_t hp = 100 - g->health;
		if (g->p.hospital_per_hp > 0 && (int64_t)hp * g->p.hospital_per_hp > g->cash)
			hp = (int32_t)(g->cash / g->p.hospital_per_hp);
		if (hp > 0)
			game_heal(g, hp);
	}

	if (g->debt[c] > 0) {
		if (k == BOT_CAUTIOUS)
			game_repay(g, g->cash < g->debt[c] ? g->cash : g->debt[c]);
		else if (g->cash > 3 * g->debt[c])
			game_repay(g, g->debt[c]);
	}

	if (k == BOT_GREEDY)
		buy_gear(g);

	if (g->day == g->last_day)
		return;

	/* buy the good that is cheapest against its range midpoint */
	for (i = 0; i < NUM_GOODS; i++) {
		int64_t mid = (GOODS[i].lo + GOODS[i].hi) / 2;
		if (g->price[i] <= 0)
			continue;
		if (pick < 0 || (int64_t)g->price[i] * best_den < best_num * mid) {
			pick = i;
			best_num = g->price[i];
			best_den = mid;
		}
	}
	if (pick >= 0) {
		int32_t n = game_max_buy(g, pick);
		if (k == BOT_CAUTIOUS) {
			int32_t room = g->capacity / 2 - game_carried(g);
			if (n > room)
				n = room;
		}
		if (n > 0)
			game_buy(g, pick, n);
	}

	if (g->cash > 20000)
		game_deposit(g, (g->cash - 20000) / 2);
}

static void move(Game *g)
{
	Loc dest = g->loc;

	if (rng_below(&bot_rng, 10) < 7) {
		dest.hood = (uint8_t)((g->loc.hood + 1 + rng_below(&bot_rng, HOODS_PER_CITY - 1)) % HOODS_PER_CITY);
	} else {
		dest.city = (uint8_t)((g->loc.city + 1 + rng_below(&bot_rng, NUM_CITIES - 1)) % NUM_CITIES);
		dest.hood = (uint8_t)rng_below(&bot_rng, HOODS_PER_CITY);
		if (game_flight_cost(g, dest.city) > g->cash) {
			dest = g->loc;
			dest.hood = (uint8_t)((g->loc.hood + 1) % HOODS_PER_CITY);
		}
	}
	game_move(g, dest);
}

void bot_play_day(Game *g, BotKind k)
{
	seed_for(g);
	if (g->phase == PH_OVER)
		return;
	if (g->phase == PH_CHASE) {
		chase_action(g, k);
		return;
	}
	trade(g, k);
	move(g);
}
