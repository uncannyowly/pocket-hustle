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
	[CARD_RUMOR] = "PODCAST PETE", [CARD_DAY] = "NEW DAY", [CARD_WILD] = "WILD CARD",
};
static const IconId KICON[] = {
	[CARD_TRAVEL] = ICON_TRAIN, [CARD_MARKET] = ICON_CHART, [CARD_TROUBLE] = ICON_BANG,
	[CARD_RUMOR] = ICON_MIC, [CARD_DAY] = ICON_SUN, [CARD_WILD] = ICON_BOLT,
};

/* The card a message makes, or -1 when it is not news (chase and shop messages). An offer
 * makes a card that asks (it needs the offer to still be open); the crew's cards, offer and
 * outcomes, are TROUBLE: no jokes, no cheer. */
static int card_kind(const Game *g, int msg_kind)
{
	switch (msg_kind) {
	case M_RODE: case M_FLEW: return CARD_TRAVEL;
	case M_SPIKE: case M_CRASH: return CARD_MARKET;
	case M_GOONS: case M_MUGGED: case M_DEAD: case M_COPS: return CARD_TROUBLE;
	case M_RUMOR_TIP: case M_RUMOR_JOKE: return CARD_RUMOR;
	case M_LAST_DAY: case M_GAME_OVER: return CARD_DAY;
	case M_RC_FIND: case M_RC_HIT: case M_RC_BUST: case M_RC_BUST_FOUND: return CARD_MARKET;
	case M_CREW_PAID: case M_OVERDOSE: case M_DEBT_SOLD: return CARD_TROUBLE;
	case M_RC_OFFER: return g->offer.kind == OFFER_RC ? CARD_WILD : -1;
	case M_CREW_OFFER: return g->offer.kind == OFFER_CREW ? CARD_TROUBLE : -1;
	default: return -1;
	}
}

/* Fills a card from a message already known to make one (kind k). */
static void fill_card(const Game *g, const Msg *m, int k, Card *cd)
{
	cd->kind = (CardKind)k;
	cd->good = (m->kind == M_SPIKE || m->kind == M_CRASH) && m->a >= 0 && m->a < NUM_GOODS ? (int8_t)m->a : -1;
	cd->choice = m->kind == M_RC_OFFER ? CHOICE_RC : m->kind == M_CREW_OFFER ? CHOICE_CREW : CHOICE_NONE;
	text_msg(g, m, cd->text, sizeof cd->text);
}

/* The cards the game's messages make, at most `max`; the count. */
static int make_cards(const Game *g, Card *out, int max)
{
	int i, n = 0;

	for (i = 0; i < g->nmsgs && i < MAX_MSGS && n < max; i++) {
		int k = card_kind(g, g->msgs[i].kind);

		if (k >= 0)
			fill_card(g, &g->msgs[i], k, &out[n++]);
	}
	return n;
}

void ui_show_open_offer(Ui *u, Screen after)
{
	const Game *g = &u->app->g;
	Msg m;
	int k;

	if (g->phase != PH_TRADING || g->offer.kind == OFFER_NONE)
		return;
	memset(&m, 0, sizeof m);
	m.kind = g->offer.kind == OFFER_RC ? M_RC_OFFER : M_CREW_OFFER;
	m.a = g->offer.chem;
	k = card_kind(g, m.kind);
	if (k < 0)
		return;
	fill_card(g, &m, k, &u->news.cards[0]);
	u->news.n = 1;
	u->news.i = 0;
	u->news.after = after;
	u->news.ask = 0;
	overlay(u, SCR_NEWS, SCR_MARKET);
}

void ui_collect_news(Ui *u, Screen after)
{
	int n = make_cards(&u->app->g, u->news.cards, MAX_MSGS + 2);

	if (!n) {
		go(u, after);
		return;
	}
	u->news.n = n;
	u->news.i = 0;
	u->news.after = after;
	u->news.ask = 0;
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
		[CARD_RUMOR] = PAL_MINT, [CARD_DAY] = PAL_LINE_HI, [CARD_WILD] = PAL_HOT,
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
	if (cd->choice) {			/* two choices, and nothing else answers */
		int k;

		for (k = 1; k <= 2; k++) {
			char lab[24];
			int bx = X + 12 + (k - 1) * 134, by = Y + H - 26;

			news_choice_label(u, k, lab, sizeof lab);
			button(u, c, bx, by, 122, 18, lab, cd->choice == CHOICE_RC && k == 1, HIT_PAD,
			       k == 1 ? PAD_A : PAD_B);
			badge(c, k == 1 ? "A" : "B", bx + 5, by + 5);
		}
		if (u->news.ask && cd->choice == CHOICE_CREW) {
			const Hint *h;
			int n;

			prompt_ask(u, c, 8, 114, 304, news_ask_text(u), u->news.yes, 0);
			cv_rect(c, 0, HINTS_Y - 4, CV_W, CV_H - HINTS_Y + 4, PAL_NIGHT);
			n = news_ask_hints(u, &h);
			hints(c, h, n, HINTS_Y);
		}
		return;
	}
	last = u->news.i == u->news.n - 1;
	badge(c, "A", X + W - 12 - 11 - 3 - (last ? 16 : 32), Y + H - 16);
	tx(c, last ? "OK" : "Next", X + W - 12, Y + H - 16, PAL_TEXT, 1, TX_RIGHT);
	hit(u, 0, 0, CV_W, CV_H, HIT_PAD, PAD_A);
}

int news_ask_hints(const Ui *u, const Hint **h)
{
	static const Hint ASK[2] = {{"A", "Choose"}, {"B", "Back"}};

	*h = ASK;
	return u->news.ask && u->news.n > 0 && u->news.i >= 0 && u->news.i < u->news.n &&
	       u->news.cards[u->news.i].choice == CHOICE_CREW ? 2 : 0;
}

const char *news_ask_text(const Ui *u)
{
	(void)u;
	return "Hand your whole stash to the crew?";
}

int news_choice_label(const Ui *u, int which, char *buf, size_t n)
{
	static const char *const RC[2] = {"BACK IT", "PASS"}, *const CREW[2] = {"DO IT", "WALK AWAY"};
	int ch;

	if (n)
		buf[0] = '\0';
	if (u->news.n <= 0 || u->news.i < 0 || u->news.i >= u->news.n || which < 1 || which > 2)
		return 0;
	ch = u->news.cards[u->news.i].choice;
	if (ch == CHOICE_NONE)
		return 0;
	if (n)
		snprintf(buf, n, "%s", (ch == CHOICE_RC ? RC : CREW)[which - 1]);
	return 1;
}

/* Answers the offer card on screen. The cards still to come stay, after the outcome's own. */
void news_answer(Ui *u, int accept, int64_t stake)
{
	Game *g = &u->app->g;
	Card rest[MAX_MSGS + 2], out[MAX_MSGS + 2];
	int choice, nrest, nout, r, i, n;
	int16_t day = g->day;
	Phase ph = g->phase;

	if (u->news.n <= 0 || u->news.i < 0 || u->news.i >= u->news.n)
		return;
	choice = u->news.cards[u->news.i].choice;
	if (choice == CHOICE_NONE)
		return;
	nrest = u->news.n - u->news.i - 1;
	memcpy(rest, &u->news.cards[u->news.i + 1], (size_t)nrest * sizeof rest[0]);
	u->news.ask = 0;
	if (!accept)
		r = game_offer_pass(g);
	else if (choice == CHOICE_RC)
		r = game_rc_back(g, stake);
	else
		r = game_crew_accept(g);
	if (r == E_NO_STOCK) {
		toast(u, "You don't have enough in the stash for that anymore.");
		play(u, SFX_DENY);
		return;
	}
	if (r != OK && r != E_PHASE) {		/* E_PHASE: the offer is gone already, so drop the card */
		toast(u, err_text(r));
		play(u, SFX_DENY);
		return;
	}
	if (r == OK) {
		app_after_action(u->app, day, ph);
		if (accept && choice == CHOICE_RC) {
			char amt[28], msg[96];

			fmt_money(stake, amt, sizeof amt);
			snprintf(msg, sizeof msg, "You back %s with %s.", CHEMS[g->stakes[g->nstakes - 1].chem].name, amt);
			toast(u, msg);
		}
		play(u, accept ? SFX_OK : SFX_BACK);	/* never the cash register on the crew's path */
	}
	nout = r == OK ? make_cards(g, out, MAX_MSGS + 2) : 0;
	n = 0;
	for (i = 0; i < nout; i++)
		u->news.cards[n++] = out[i];
	for (i = 0; i < nrest && n < MAX_MSGS + 2; i++)
		u->news.cards[n++] = rest[i];
	if (g->phase == PH_OVER)
		u->news.after = SCR_OVER;
	if (!n) {
		u->news.n = u->news.i = 0;
		go(u, u->news.after);
		return;
	}
	u->news.n = n;
	u->news.i = 0;
	u->news.ask = 0;
	overlay(u, SCR_NEWS, SCR_MARKET);
}


/* A click on the crew prompt: YES hands the stash over, NO goes back to the card. */
void news_click(Ui *u, int action, int arg)
{
	if (action != HIT_CONFIRM || arg < 0)
		return;
	u->news.yes = arg;
	ui_press(u, arg ? PAD_A : PAD_B);
}

void news_press(Ui *u, int pad)
{
	int have = u->news.n > 0 && u->news.i >= 0 && u->news.i < u->news.n;

	if (have && u->news.ask && u->news.cards[u->news.i].choice == CHOICE_CREW) {
		if (pad == PAD_A && u->news.yes) {
			news_answer(u, 1, 0);
		} else if (pad == PAD_A || pad == PAD_B) {
			u->news.ask = 0;
			play(u, SFX_BACK);
		} else if (pad == PAD_LEFT || pad == PAD_RIGHT) {
			u->news.yes = pad == PAD_LEFT;
			play(u, SFX_MOVE);
		}
		return;
	}
	if (pad != PAD_A && pad != PAD_B)
		return;
	if (have && u->news.cards[u->news.i].choice) {
		const Card *cd = &u->news.cards[u->news.i];

		if (pad == PAD_A && cd->choice == CHOICE_RC)
			picker_open(u, PK_STAKE, u->app->g.offer.chem, SCR_MARKET);
		else if (pad == PAD_A && cd->choice == CHOICE_CREW) {
			u->news.ask = 1;
			u->news.yes = 0;
			play(u, SFX_MOVE);
		} else
			news_answer(u, pad == PAD_A, 0);
		return;
	}
	if (u->news.i < u->news.n - 1) {
		u->news.i++;
		play(u, SFX_MOVE);
		return;
	}
	u->news.n = u->news.i = 0;
	go(u, u->news.after);
}
