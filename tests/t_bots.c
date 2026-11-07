#include "check.h"
#include "tests.h"
#include "bots.h"

/* The cautious bot stands for a careful player: it must trade, not just pay its
 * debt down to zero cash and stand still for the whole game. */
static void test_cautious_bot_trades(void)
{
	Params p = params_default();
	Game g;
	uint64_t seed;
	int traded_games = 0;
	for (seed = 1; seed <= 20; seed++) {
		int steps = 0, carried = 0;
		game_new(&g, &p, seed, 30, "Bot");
		while (g.phase != PH_OVER && steps++ < 1000) {
			bot_play_day(&g, BOT_CAUTIOUS);
			if (game_carried(&g) > 0)
				carried = 1;
		}
		traded_games += carried;
	}
	CHECK(traded_games >= 15);
}

void tests_bots(void)
{
	test_cautious_bot_trades();
}
