#ifndef PH_HELPERS_H
#define PH_HELPERS_H

#include "core/game.h"

/* params_default() with every random event off: absent, spike, crash, cop, mug, rumor,
 * drop = 0, and no wild cards, finds or crew offers; enforce_days = 1000; min_goods = 0; cop_max = 1000 (uncapped). Tests switch on
 * what they need. */
Params test_params(void);
void   new_test_game(Game *g, const Params *p, int days);	/* seed 1, name "Test" */
int    same_game(const Game *a, const Game *b);			/* memcmp */
int    count_msgs(const Game *g, MsgKind k);

#endif
