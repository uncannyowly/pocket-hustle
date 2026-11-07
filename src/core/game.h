/* Pocket Hustle rules: the game state and every action a frontend can take.
 * Deterministic: integer math only, all randomness from g->rng (gameplay) and
 * g->flavor (which joke to tell). */
#ifndef PH_GAME_H
#define PH_GAME_H

#include <stdint.h>

#include "content/content.h"
#include "core/msg.h"
#include "core/params.h"
#include "core/rng.h"

#define MONEY_MAX 1000000000000000LL

enum { MAX_DAYS = 90, MAX_MSGS = 32 };

typedef enum { OK = 0, E_PHASE = -1, E_BAD_ARG = -2, E_NOT_SOLD = -3, E_NO_CASH = -4,
	       E_NO_SPACE = -5, E_NO_STOCK = -6, E_LIMIT = -7 } Err;
typedef enum { PH_TRADING, PH_CHASE, PH_OVER } Phase;
typedef enum { END_NONE, END_DAYS_UP, END_DEAD, END_QUIT } EndReason;
typedef enum { CH_RUN, CH_FIGHT, CH_SURRENDER } ChaseAction;

typedef struct { uint8_t city, hood; } Loc;
typedef struct { int16_t day; Loc loc; int8_t good, spike; } Tip;	/* day 0 = none */
typedef struct { int16_t posse, cops_left; } Chase;

typedef struct {
	int32_t moves, flights, cop_chases, cops_beaten, escapes, times_jailed, times_mugged,
		times_dropped, goon_visits, spikes_seen, crashes_seen, rumors_true, rumors_joke;
	int64_t best_net_worth, hospital_spent;
} Stats;

typedef struct {
	Params p;
	Rng rng, flavor;
	uint64_t seed;
	char name[16];
	int16_t day, last_day;
	Loc loc;
	int64_t cash, bank, debt[NUM_CITIES];
	int16_t debt_age[NUM_CITIES];
	int32_t stash[NUM_GOODS];
	int64_t stash_paid[NUM_GOODS];
	int32_t capacity;
	int16_t health;
	int8_t weapon;				/* GEAR index, -1 = none */
	uint8_t gear_owned[MAX_GEAR];
	int32_t price[NUM_GOODS];		/* today's market here, 0 = not sold */
	int32_t hist[NUM_GOODS][MAX_DAYS + 1];	/* price seen here each day, 0 = none */
	Tip tip;
	Chase chase;
	Phase phase;
	EndReason end;
	Stats stats;
	Msg msgs[MAX_MSGS];
	int16_t nmsgs;
} Game;

int     game_new(Game *g, const Params *p, uint64_t seed, int days, const char *name);
int64_t game_net_worth(const Game *g);		/* cash + bank - all debts */

/* Trading at today's market (PH_TRADING only). */
int     game_buy(Game *g, int good, int32_t qty);
int     game_sell(Game *g, int good, int32_t qty);
int32_t game_max_buy(const Game *g, int good);	/* 0 if not sold here */
int32_t game_carried(const Game *g);

/* Moving: transit inside a city is free, a flight costs a ticket. Each move is a day.
 * Moving on the last day ends the game instead. */
int     game_move(Game *g, Loc dest);
int64_t game_flight_cost(const Game *g, int city);	/* 0 inside the current city */
int     game_end(Game *g);				/* quit early */

/* Money: the current city's lender, and the bank (any city). */
int     game_borrow(Game *g, int64_t amt);
int     game_repay(Game *g, int64_t amt);
int     game_deposit(Game *g, int64_t amt);
int     game_withdraw(Game *g, int64_t amt);

#endif
