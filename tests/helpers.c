#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"

Params test_params(void)
{
	Params p = params_default();
	p.absent_permille = 0;
	p.spike_permille = 0;
	p.crash_permille = 0;
	p.cop_base_permille = 0;
	p.cop_per_10k_permille = 0;
	p.mug_permille = 0;
	p.rumor_permille = 0;
	p.drop_permille = 0;
	p.enforce_days = 1000;
	p.min_goods = 0;
	return p;
}

void new_test_game(Game *g, const Params *p, int days)
{
	if (game_new(g, p, 1, days, "Test") != OK) {
		fprintf(stderr, "new_test_game: game_new failed\n");
		exit(2);
	}
}

int same_game(const Game *a, const Game *b)
{
	return memcmp(a, b, sizeof *a) == 0;
}

int count_msgs(const Game *g, MsgKind k)
{
	int i, n = 0;
	for (i = 0; i < g->nmsgs; i++)
		if (g->msgs[i].kind == k)
			n++;
	return n;
}
