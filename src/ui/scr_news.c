/* The news cards: the mockup's drawNews and inNews, and the glue that turns the core's
 * messages into cards after every action that moves the day or the phase. */
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

static const char *const KLABEL[] = {
	[CARD_TRAVEL] = "ON THE MOVE", [CARD_MARKET] = "MARKET NEWS", [CARD_TROUBLE] = "TROUBLE",
	[CARD_RUMOR] = "PODCAST PETE", [CARD_DAY] = "NEW DAY",
};
static const IconId KICON[] = {
	[CARD_TRAVEL] = ICON_TRAIN, [CARD_MARKET] = ICON_CHART, [CARD_TROUBLE] = ICON_BANG,
	[CARD_RUMOR] = ICON_MIC, [CARD_DAY] = ICON_SUN,
};

/* The card a message makes, or -1 when it is not news (chase and shop messages). */
static int card_kind(int msg_kind)
{
	switch (msg_kind) {
	case M_RODE: case M_FLEW: return CARD_TRAVEL;
	case M_SPIKE: case M_CRASH: return CARD_MARKET;
	case M_GOONS: case M_MUGGED: case M_DEAD: case M_COPS: return CARD_TROUBLE;
	case M_RUMOR_TIP: case M_RUMOR_JOKE: return CARD_RUMOR;
	case M_LAST_DAY: case M_GAME_OVER: return CARD_DAY;
	default: return -1;
	}
}

void ui_collect_news(Ui *u, Screen after)
{
	const Game *g = &u->app->g;
	int i, n = 0;

	for (i = 0; i < g->nmsgs && i < MAX_MSGS; i++) {
		const Msg *m = &g->msgs[i];
		int k = card_kind(m->kind);
		Card *cd;

		if (k < 0)
			continue;
		cd = &u->news.cards[n++];
		cd->kind = (CardKind)k;
		cd->good = k == CARD_MARKET && m->a >= 0 && m->a < NUM_GOODS ? (int8_t)m->a : -1;
		text_msg(g, m, cd->text, sizeof cd->text);
	}
	if (!n) {
		go(u, after);
		return;
	}
	u->news.n = n;
	u->news.i = 0;
	u->news.after = after;
	overlay(u, SCR_NEWS, SCR_MARKET);
}

void ui_after_action(Ui *u, int16_t day_before, Phase phase_before)
{
	Phase ph;

	app_after_action(u->app, day_before, phase_before);
	ph = u->app->g.phase;
	if (ph == PH_CHASE)
		chase_begin(u);
	ui_collect_news(u, ph == PH_CHASE ? SCR_CHASE : ph == PH_OVER ? SCR_OVER : SCR_MARKET);
}

int ui_core_call(Ui *u, CoreCall fn, const void *arg, int ok_sfx, int news)
{
	Game *g = &u->app->g;
	int16_t day = g->day;
	Phase ph = g->phase;
	int r = fn(g, arg);

	if (r != OK) {
		toast(u, err_text(r));
		play(u, SFX_DENY);
		return r;
	}
	if (ok_sfx >= 0)
		play(u, (SfxId)ok_sfx);
	if (news)
		ui_after_action(u, day, ph);
	else
		app_after_action(u->app, day, ph);
	return OK;
}

void news_draw(Ui *u, Canvas *c)
{
	enum { X = 20, Y = 36, W = 280, H = 168 };
	static const uint32_t KCOL[] = {
		[CARD_TRAVEL] = PAL_SKY, [CARD_MARKET] = PAL_GOLD, [CARD_TROUBLE] = PAL_CORAL,
		[CARD_RUMOR] = PAL_MINT, [CARD_DAY] = PAL_LINE_HI,
	};
	const Card *cd;
	char lines[7][TX_LINE_MAX], s[40];
	int i, nl, last;
	uint32_t kc;

	if (u->news.n <= 0 || u->news.i < 0 || u->news.i >= u->news.n) {
		go(u, SCR_MARKET);
		return;
	}
	cd = &u->news.cards[u->news.i];
	kc = KCOL[cd->kind];
	cv_rect(c, X, Y, W, H, PAL_PANEL);
	cv_box(c, X, Y, W, H, kc);
	cv_rect(c, X + 1, Y + 1, W - 2, 18, kc);
	art_icon(c, KICON[cd->kind], X + 6, Y + 4, 1, 0);
	tx(c, KLABEL[cd->kind], X + 20, Y + 6, PAL_NIGHT, 1, TX_LEFT);
	snprintf(s, sizeof s, "%d/%d", u->news.i + 1, u->news.n);
	tx(c, s, X + W - 6, Y + 6, PAL_NIGHT, 1, TX_RIGHT);
	nl = tx_wrap(cd->text, 32, lines, 7);
	for (i = 0; i < nl; i++)
		tx(c, lines[i], X + 12, Y + 30 + i * 12, PAL_TEXT, 1, TX_LEFT);
	if (cd->good >= 0) {
		snprintf(s, sizeof s, "%s TODAY", GOODS[cd->good].name);
		for (i = 0; s[i]; i++)
			if (s[i] >= 'a' && s[i] <= 'z')
				s[i] = (char)(s[i] - 'a' + 'A');
		tx(c, s, X + 12, Y + H - 52, PAL_DIM, 1, TX_LEFT);
		fmt_money_short(u->app->g.price[cd->good], s, sizeof s);
		tx(c, s, X + 12, Y + H - 40, price_flag(&u->app->g, cd->good) == PF_SPIKE ? PAL_HOT : PAL_MINT,
		   2, TX_LEFT);
	}
	last = u->news.i == u->news.n - 1;
	badge(c, "A", X + W - 12 - 11 - 3 - (last ? 16 : 32), Y + H - 16);
	tx(c, last ? "OK" : "Next", X + W - 12, Y + H - 16, PAL_TEXT, 1, TX_RIGHT);
	hit(u, 0, 0, CV_W, CV_H, HIT_PAD, PAD_A);
}

void news_press(Ui *u, int pad)
{
	if (pad != PAD_A && pad != PAD_B)
		return;
	if (u->news.i < u->news.n - 1) {
		u->news.i++;
		play(u, SFX_MOVE);
		return;
	}
	u->news.n = u->news.i = 0;
	go(u, u->news.after);
}
