/* ph-sim bots: simple players used to measure the balance. */
#ifndef PH_BOTS_H
#define PH_BOTS_H

#include "core/game.h"

/* BOT_WILD plays the greedy game and takes every offer (a test bot; the balance bots never do). */
typedef enum { BOT_GREEDY, BOT_CAUTIOUS, BOT_WILD } BotKind;

/* Trades, then moves once (or takes one chase action when the cops are on it). */
void bot_play_day(Game *g, BotKind k);

#endif
