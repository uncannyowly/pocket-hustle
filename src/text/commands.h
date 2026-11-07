/* ph-text: the terminal version. One command per line; see help_text in commands.c. */
#ifndef PH_TEXT_COMMANDS_H
#define PH_TEXT_COMMANDS_H

#include "core/game.h"
#include "core/save.h"

typedef struct {
	Game g;
	HiTable scores;
	const char *save_path;		/* NULL = no autosave */
	const char *scores_path;	/* NULL = scores kept in memory only */
	int over_reported;		/* the game-over summary has been printed */
} Ctx;

void ph_print_status(const Ctx *c);
void ph_print_msgs(const Ctx *c);
/* Runs one command line. Returns 1 when the program should exit. */
int  ph_command(Ctx *c, const char *line);

#endif
