#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"
#include "ui_helpers.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

void ui_test_setup(App *a, Ui *u, int days)
{
	Params p = test_params();

	app_init(a, NULL);
	new_test_game(&a->g, &p, days);
	a->has_game = 1;
	ui_init(u, a);
	go(u, SCR_MARKET);
}

void press_seq(Ui *u, const char *seq)
{
	char tok[8];
	size_t n;
	int pad;

	for (;;) {
		while (*seq == ' ')
			seq++;
		if (!*seq)
			return;
		for (n = 0; seq[n] && seq[n] != ' '; n++)
			;
		snprintf(tok, sizeof tok, "%.*s", (int)(n < sizeof tok - 1 ? n : sizeof tok - 1), seq);
		pad = n < sizeof tok ? input_token(tok) : -1;
		if (pad < 0) {
			fprintf(stderr, "press_seq: unknown token \"%s\"\n", tok);
			exit(2);
		}
		ui_press(u, pad);
		seq += n;
	}
}

int hit_find(const Ui *u, int action, int arg)
{
	int i;

	for (i = 0; i < u->nhits; i++)
		if (u->hits[i].action == action && u->hits[i].arg == arg)
			return i;
	return -1;
}

void click_hit(Ui *u, int i)
{
	Hit h;

	if (i < 0 || i >= u->nhits)
		return;
	h = u->hits[i];
	ui_click(u, h.x + h.w / 2, h.y + h.h / 2);
}

int canvas_colors(const Canvas *c, int cap)
{
	uint32_t seen[256];
	int n = 0, i, k;

	if (cap > 256)
		cap = 256;
	for (i = 0; i < CV_W * CV_H && n < cap; i++) {
		for (k = 0; k < n && seen[k] != c->px[i]; k++)
			;
		if (k == n)
			seen[n++] = c->px[i];
	}
	return n;
}
