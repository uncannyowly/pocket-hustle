/* What the core reports to frontends after each action. The text layer
 * (src/content/text.c) turns a Msg into words; variant picks the joke. */
#ifndef PH_MSG_H
#define PH_MSG_H

#include <stdint.h>

typedef enum {
	M_SPIKE,	/* a good, amount price */
	M_CRASH,	/* a good, amount price */
	M_RODE,		/* a hood */
	M_FLEW,		/* a city, c 1 if it crossed the border, amount cost */
	M_LAST_DAY,
	M_GAME_OVER,
	M_GOONS,	/* a city, amount damage */
	M_DEAD,
	M_COPS,		/* a posse */
	M_ESCAPED,
	M_DROPPED,	/* a good, amount qty */
	M_YOU_HIT,	/* a cops left */
	M_YOU_MISSED,
	M_HIT_YOU,	/* amount damage */
	M_COPS_MISSED,
	M_COPS_BEATEN,	/* amount loot */
	M_JAILED,	/* amount fine */
	M_MUGGED,	/* amount */
	M_RUMOR_TIP,	/* a good, b loc index city*6+hood, c 1 spike 0 crash, amount day */
	M_RUMOR_JOKE,	/* a city */
	M_HEALED,	/* amount hp */
	M_BOUGHT_GEAR,	/* a gear */
	NUM_MSG_KINDS
} MsgKind;

typedef struct { uint8_t kind; uint16_t variant; int16_t a, b, c; int64_t amount; } Msg;

#endif
