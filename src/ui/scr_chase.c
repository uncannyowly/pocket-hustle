/* The cop chase: the mockup's drawChase, inChase, chaseAct and finishChase. The core runs
 * the rules; this screen picks RUN, FIGHT or SURRENDER, shows what happened and, once the
 * phase has left PH_CHASE, waits on CONTINUE before the news, the market or the end. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

enum { NUM_BTNS = 3 };
static const char *const BTN[NUM_BTNS] = {"RUN", "FIGHT", "SURRENDER"};
static const Hint HINTS_CHOOSE[1] = {{"A", "Choose"}};
static const Hint HINTS_GO[1] = {{"A", "Continue"}};

void chase_title(const Game *g, char *buf, size_t n)
{
	int left = g->chase.cops_left;

	if (left <= 0)
		snprintf(buf, n, "NO COPS LEFT");
	else if (left == 1)
		snprintf(buf, n, "SGT. BUZZKILL");
	else
		snprintf(buf, n, "SGT. BUZZKILL + %d COP%s", left - 1, left == 2 ? "" : "S");
}

void chase_odds(const Game *g, int row, char *buf, size_t n)
{
	if (row == 0)
		snprintf(buf, n, "Run: %d%% chance to get away.", g->p.run_permille / 10);
	else if (row == 1 && g->weapon >= 0 && g->weapon < NUM_GEAR)
		snprintf(buf, n, "Fight: %d%% to hit. Then they shoot.", GEAR[g->weapon].power / 10);
	else if (row == 1)
		snprintf(buf, n, "Fight: you need a weapon first.");
	else
		snprintf(buf, n, "Jail: lose the stash and %d%% of cash.", g->p.jail_fine_pct);
}

/* Rebuilds the log from the chase messages of the last core call. */
static void collect(Ui *u)
{
	const Game *g = &u->app->g;
	size_t len = 0;
	int i;

	u->chase_log[0] = '\0';
	u->chase_done = CR_NONE;
	for (i = 0; i < g->nmsgs && i < MAX_MSGS; i++) {
		const Msg *m = &g->msgs[i];
		char t[160];

		switch (m->kind) {
		case M_COPS: case M_ESCAPED: case M_DROPPED: case M_YOU_HIT: case M_YOU_MISSED:
		case M_HIT_YOU: case M_COPS_MISSED: case M_COPS_BEATEN: case M_JAILED: case M_DEAD:
			break;
		default:
			continue;
		}
		if (m->kind == M_ESCAPED)
			u->chase_done = CR_ESCAPED;
		else if (m->kind == M_COPS_BEATEN)
			u->chase_done = CR_WON;
		else if (m->kind == M_JAILED)
			u->chase_done = CR_JAILED;
		text_msg(g, m, t, sizeof t);
		if (len + strlen(t) + 2 >= sizeof u->chase_log)
			continue;
		len += (size_t)snprintf(u->chase_log + len, sizeof u->chase_log - len, "%s%s",
					len ? " " : "", t);
	}
	for (i = 0; i < g->nmsgs && i < MAX_MSGS; i++)
		if (g->msgs[i].kind == M_DEAD)
			u->chase_done = CR_DEAD;
	if (g->phase == PH_CHASE)
		u->chase_done = CR_NONE;
}

void chase_begin(Ui *u)
{
	u->row = 0;
	collect(u);
	play(u, SFX_SIREN);
}

static int chase_call(Game *g, const void *arg)
{
	return game_chase(g, *(const ChaseAction *)arg);
}

static void act(Ui *u, int row)
{
	const Game *g = &u->app->g;
	ChaseAction a = row == 0 ? CH_RUN : row == 1 ? CH_FIGHT : CH_SURRENDER;
	int i, hurt = 0;

	if (a == CH_FIGHT && (g->weapon < 0 || g->weapon >= NUM_GEAR)) {
		toast(u, "You need a weapon from the shop.");
		play(u, SFX_DENY);
		return;
	}
	if (ui_core_call(u, chase_call, &a, -1, 0) != OK)
		return;
	collect(u);
	for (i = 0; i < g->nmsgs; i++)
		hurt |= g->msgs[i].kind == M_HIT_YOU;
	if (u->chase_done == CR_WON || u->chase_done == CR_ESCAPED)
		play(u, SFX_WIN);			/* beat them or outran them */
	else if (u->chase_done == CR_JAILED || u->chase_done == CR_DEAD)
		play(u, SFX_FAIL);
	else if (hurt)
		play(u, SFX_HURT);
	else
		play(u, SFX_MOVE);
}

void chase_draw(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	int fl = (int)(u->frame >> 2) & 1, done = g->phase != PH_CHASE, i;
	char s[96], lines[8][TX_LINE_MAX];
	const char *res = "";
	uint32_t rcol = PAL_TEXT;

	if (u->row < 0 || u->row >= NUM_BTNS)
		u->row = 0;
	top_bar(u, c, NULL, NULL);
	tx(c, "SIRENS!", 150, 2, fl ? PAL_CORAL : PAL_SKY, 1, TX_CENTER);
	art_chase_scene(c, 12, g->chase.cops_left, u->frame);
	cv_rect(c, 0, 108, CV_W, 116, PAL_PANEL);
	cv_rect(c, 0, 108, CV_W, 1, PAL_LINE);
	chase_title(g, s, sizeof s);
	tx(c, s, 8, 114, PAL_CORAL, 1, TX_LEFT);
	{
		char w[32] = "NOTHING";

		if (g->weapon >= 0 && g->weapon < NUM_GEAR) {
			size_t k;

			for (k = 0; GEAR[g->weapon].name[k] && k + 1 < sizeof w; k++)
				w[k] = (char)toupper((unsigned char)GEAR[g->weapon].name[k]);
			w[k] = '\0';
		}
		snprintf(s, sizeof s, "PACKING: %s", w);
		tx(c, s, 8, 126, PAL_DIM, 1, TX_LEFT);
	}
	if (!done) {
		for (i = 0; i < NUM_BTNS; i++)
			button(u, c, 8 + i * 104, 140, 96, 18, BTN[i], u->row == i, HIT_CHASE_BTN, i);
		chase_odds(g, u->row, s, sizeof s);
		tx(c, s, 160, 163, PAL_DIM, 1, TX_CENTER);
	} else {
		switch (u->chase_done) {
		case CR_ESCAPED: res = "YOU GOT AWAY"; rcol = PAL_MINT; break;
		case CR_WON: res = "YOU WON"; rcol = PAL_GOLD; break;
		case CR_JAILED: res = "BUSTED"; rcol = PAL_CORAL; break;
		case CR_DEAD: res = "YOU DIED"; rcol = PAL_CORAL; break;
		default: break;
		}
		button(u, c, 8, 140, 304, 18, "CONTINUE", 1, HIT_PAD, PAD_A);
		tx(c, res, 160, 163, rcol, 1, TX_CENTER);
	}
	cv_rect(c, 4, 176, 312, 46, PAL_NIGHT);
	cv_box(c, 4, 176, 312, 46, PAL_LINE);
	{
		int n = tx_wrap(u->chase_log, 37, lines, 8);

		for (i = 0; i < n && i < 4; i++)
			tx(c, lines[i], 10, 181 + i * 10, PAL_TEXT, 1, TX_LEFT);
	}
	hints(c, done ? HINTS_GO : HINTS_CHOOSE, 1, HINTS_Y);
}

static void finish(Ui *u)
{
	u->chase_done = CR_NONE;
	u->chase_log[0] = '\0';
	if (u->app->g.phase == PH_OVER)
		go(u, SCR_OVER);
	else
		ui_collect_news(u, SCR_MARKET);
}

void chase_press(Ui *u, int pad)
{
	if (u->app->g.phase != PH_CHASE) {
		if (pad == PAD_A) {
			finish(u);
			play(u, SFX_OK);
		}
		return;
	}
	if (pad == PAD_LEFT)
		u->row = (u->row + NUM_BTNS - 1) % NUM_BTNS;
	else if (pad == PAD_RIGHT)
		u->row = (u->row + 1) % NUM_BTNS;
	else if (pad == PAD_A) {
		act(u, u->row);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void chase_click(Ui *u, int action, int arg)
{
	if (action != HIT_CHASE_BTN)
		return;
	u->row = arg;
	ui_press(u, PAD_A);
}
