/* Helpers shared inside the core. Frontends use game.h only. */
#ifndef PH_GAME_INTERNAL_H
#define PH_GAME_INTERNAL_H

#include "core/game.h"

void     game_msg(Game *g, MsgKind k, int16_t a, int16_t b, int16_t c, int64_t amount);
void     game_clear_msgs(Game *g);		/* every public action starts with this */
void     market_roll(Game *g);
uint64_t mix64(uint64_t x);			/* splitmix64 finalizer */
void     game_advance_day(Game *g);		/* interest, debt clocks, day++ */
void     game_check_death(Game *g);		/* health <= 0 ends the game */
void     encounters_on_arrival(Game *g);	/* goons, cops, muggings, rumors */

#endif
