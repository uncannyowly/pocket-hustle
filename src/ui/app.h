/* The App: the game in progress, the high scores, the options and the save policy.
 * The screen state machine sits on top of it. Memory only when the directory is NULL. */
#ifndef PH_APP_H
#define PH_APP_H

#include <stddef.h>
#include <stdint.h>

#include "core/game.h"
#include "core/save.h"

typedef struct {
	Game g;
	int has_game;			/* a game in progress, loaded or started */
	int recorded;			/* this game's high score is already written */
	HiTable scores;
	char save_path[512], scores_path[512], options_path[512];	/* "" = keep in memory only */
	int sound_on;
	int last_rank;			/* rank of the score just recorded, -1 if none */
	uint64_t next_seed;
} App;

int  app_default_dir(char *buf, size_t n);	/* XDG_DATA_HOME or HOME based; 0 or -1 */
int  app_init(App *a, const char *dir);		/* mkdir -p, set the paths; dir NULL = memory only */
void app_load(App *a);				/* scores, options, resumable save; bad files mean empty */
int  app_new_game(App *a, int days, const char *name);	/* game_new with next_seed */
int  app_after_action(App *a, int16_t day_before, Phase phase_before);	/* returns rank 0..9, -1 */
int  app_save_now(App *a);
int  app_options_write(const App *a);

#endif
