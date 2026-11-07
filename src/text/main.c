/* ph-text: play Pocket Hustle in a terminal.
 *   ph-text [--seed N] [--days 15|30|60|90] [--name NAME] [--save PATH] [--scores PATH] [--script] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "text/commands.h"

static Ctx ctx;

static int usage(void)
{
	fprintf(stderr, "usage: ph-text [--seed N] [--days 15|30|60|90] [--name NAME] "
		"[--save PATH] [--scores PATH] [--script]\n");
	return 2;
}

int main(int argc, char **argv)
{
	Params p = params_default();
	uint64_t seed = (uint64_t)time(NULL);
	const char *name = "Player";
	int days = 30, script = 0, i, resumed = 0;
	char line[256];

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--seed") && i + 1 < argc)
			seed = strtoull(argv[++i], NULL, 10);
		else if (!strcmp(argv[i], "--days") && i + 1 < argc)
			days = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--name") && i + 1 < argc)
			name = argv[++i];
		else if (!strcmp(argv[i], "--save") && i + 1 < argc)
			ctx.save_path = argv[++i];
		else if (!strcmp(argv[i], "--scores") && i + 1 < argc)
			ctx.scores_path = argv[++i];
		else if (!strcmp(argv[i], "--script"))
			script = 1;
		else
			return usage();
	}

	if (ctx.scores_path)
		hiscores_read(&ctx.scores, ctx.scores_path);	/* missing or bad: empty table */
	if (ctx.save_path && save_read(&ctx.g, ctx.save_path) == 0 && ctx.g.phase != PH_OVER) {
		resumed = 1;
		printf("Resuming your saved game.\n");
	} else if (game_new(&ctx.g, &p, seed, days, name) != OK) {
		fprintf(stderr, "ph-text: --days must be 15, 30, 60 or 90\n");
		return 2;
	}

	if (!resumed)
		printf("Pocket Hustle. Seed %llu. You have %d days. Type help for commands.\n",
		       (unsigned long long)seed, days);
	ph_print_msgs(&ctx);	/* day-one news, or the chase prompt when resuming mid-chase */
	ph_print_status(&ctx);

	for (;;) {
		if (!script) {
			printf("> ");
			fflush(stdout);
		}
		if (!fgets(line, sizeof line, stdin))
			break;
		line[strcspn(line, "\r\n")] = 0;
		if (script)
			printf("> %s\n", line);
		if (ph_command(&ctx, line))
			break;
	}
	return 0;
}
