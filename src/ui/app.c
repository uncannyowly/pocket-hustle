#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core/params.h"
#include "ui/app.h"

static uint64_t splitmix_next(uint64_t *s)
{
	uint64_t z;
	*s += 0x9E3779B97F4A7C15ULL;
	z = *s;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

int app_default_dir(char *buf, size_t n)
{
	const char *x = getenv("XDG_DATA_HOME"), *h = getenv("HOME");
	int len;

	if (x && x[0])
		len = snprintf(buf, n, "%s/pocket-hustle", x);
	else if (h && h[0])
		len = snprintf(buf, n, "%s/.local/share/pocket-hustle", h);
	else
		return -1;
	return (len < 0 || (size_t)len >= n) ? -1 : 0;
}

static int mkdir_p(const char *dir)
{
	char tmp[512];
	size_t i, len = strlen(dir);

	if (len == 0 || len >= sizeof tmp)
		return -1;
	memcpy(tmp, dir, len + 1);
	for (i = 1; i <= len; i++) {
		if (tmp[i] == '/' || tmp[i] == '\0') {
			char c = tmp[i];
			tmp[i] = '\0';
			if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
				return -1;
			tmp[i] = c;
		}
	}
	return 0;
}

int app_init(App *a, const char *dir)
{
	int ok = 0;

	memset(a, 0, sizeof *a);
	a->sound_on = 1;
	a->last_rank = -1;
	a->next_seed = 1;
	if (!dir)
		return 0;
	if (mkdir_p(dir) != 0)
		ok = -1;
	if (snprintf(a->save_path, sizeof a->save_path, "%s/save.dat", dir) >= (int)sizeof a->save_path ||
	    snprintf(a->scores_path, sizeof a->scores_path, "%s/scores.dat", dir) >= (int)sizeof a->scores_path ||
	    snprintf(a->options_path, sizeof a->options_path, "%s/options.txt", dir) >= (int)sizeof a->options_path) {
		a->save_path[0] = a->scores_path[0] = a->options_path[0] = '\0';
		return -1;
	}
	return ok;
}

void app_load(App *a)
{
	Game g;

	memset(&a->scores, 0, sizeof a->scores);
	a->sound_on = 1;
	a->has_game = 0;
	a->recorded = 0;
	a->last_rank = -1;
	if (a->scores_path[0] && hiscores_read(&a->scores, a->scores_path) != 0)
		memset(&a->scores, 0, sizeof a->scores);
	if (a->options_path[0]) {
		FILE *f = fopen(a->options_path, "r");
		if (f) {
			char line[64];
			while (fgets(line, sizeof line, f))
				if (strncmp(line, "sound=", 6) == 0)
					a->sound_on = line[6] != '0';
			fclose(f);
		}
	}
	if (a->save_path[0] && save_read(&g, a->save_path) == 0 && g.phase != PH_OVER) {
		a->g = g;
		a->has_game = 1;
	}
}

int app_new_game(App *a, int days, const char *name)
{
	Params p = params_default();
	int r;

	r = game_new(&a->g, &p, a->next_seed, days, name);
	if (r != 0)
		return r;
	a->next_seed = splitmix_next(&a->next_seed);
	a->has_game = 1;
	a->recorded = 0;
	a->last_rank = -1;
	return 0;
}

int app_save_now(App *a)
{
	if (!a->has_game || !a->save_path[0])
		return 0;
	return save_write(&a->g, a->save_path);
}

int app_after_action(App *a, int16_t day_before, Phase phase_before)
{
	Game *g = &a->g;
	int rank = -1;

	if (!a->has_game)
		return -1;
	if (g->phase == PH_OVER) {
		if (a->recorded)
			return -1;
		rank = hiscore_insert(&a->scores, g->last_day, g->name, game_net_worth(g), g->end);
		if (a->scores_path[0])
			hiscores_write(&a->scores, a->scores_path);
		if (a->save_path[0])
			unlink(a->save_path);
		a->recorded = 1;
		a->last_rank = rank;
		a->has_game = 0;
		return rank;
	}
	if ((g->day != day_before || g->phase != phase_before) && a->save_path[0])
		save_write(g, a->save_path);
	return -1;
}

int app_options_write(const App *a)
{
	const char *s = a->sound_on ? "sound=1\n" : "sound=0\n";

	if (!a->options_path[0])
		return 0;
	return atomic_write_file(a->options_path, s, strlen(s));
}
