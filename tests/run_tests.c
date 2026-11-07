/* Test runner: run-tests [module]. Prints "N checks, M failed", exits 1 on any failure. */
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "tests.h"

int t_checks, t_failed;

static const struct { const char *name; void (*fn)(void); } MODULES[] = {
	{"rng", tests_rng},
	{"content", tests_content},
	{"game", tests_game},
	{"market", tests_market},
	{"trade", tests_trade},
	{"move", tests_move},
	{"finance", tests_finance},
	{"chase", tests_chase},
	{"shop", tests_shop},
	{"rumor", tests_rumor},
	{"text", tests_text},
	{"save", tests_save},
	{"report", tests_report},
	{"bots", tests_bots},
	{"gfx", tests_gfx},
	{"font", tests_font},
	{"art", tests_art},
	{"sfx", tests_sfx},
	{"input", tests_input},
	{"app", tests_app},
};

int main(int argc, char **argv)
{
	const char *only = argc > 1 ? argv[1] : NULL;
	int ran = 0;
	size_t i;

	for (i = 0; i < sizeof MODULES / sizeof MODULES[0]; i++) {
		if (only && strcmp(only, MODULES[i].name) != 0)
			continue;
		MODULES[i].fn();
		ran++;
	}
	if (!ran) {
		fprintf(stderr, "no test module named %s\n", only ? only : "(none)");
		return 2;
	}
	printf("%d checks, %d failed\n", t_checks, t_failed);
	return t_failed ? 1 : 0;
}
