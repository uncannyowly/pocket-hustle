/* The title, the new game screen, the pause menu, game over and the high scores: the
 * mockup's drawTitle/inTitle, drawNewGame/inNew, drawMenu/inMenu, drawOver/inOver and
 * drawScores/inScores. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "core/save.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

static const int LENGTHS[4] = {15, 30, 60, 90};

static int length_index(int days)
{
	int i = days_index(days);

	return i < 0 ? 1 : i;
}

static const char *city_name(int city)
{
	if (city < 0 || city >= NUM_CITIES)
		city = 0;
	return CITIES[city].nickname[0] ? CITIES[city].nickname : CITIES[city].name;
}

/* A yes/no prompt: a blocker so clicks behind it do nothing, a box, a question, YES and NO. */
static void prompt(Ui *u, Canvas *c, int x, int y, int w, const char *q)
{
	int bw = (w - 24) / 2;

	hit(u, 0, 0, CV_W, CV_H, HIT_CONFIRM, -1);
	cv_rect(c, x, y, w, 52, PAL_PANEL2);
	cv_box(c, x, y, w, 52, PAL_GOLD);
	tx(c, q, x + (w >> 1), y + 8, PAL_GOLD, 1, TX_CENTER);
	button(u, c, x + 8, y + 26, bw, 18, "A YES", 1, HIT_CONFIRM, 1);
	button(u, c, x + 16 + bw, y + 26, bw, 18, "B NO", 0, HIT_CONFIRM, 0);
}

/* ---- the title ---- */

enum { T_CONTINUE, T_NEW, T_SCORES, T_QUIT };
static const char *const TITLE_ITEMS[TITLE_ROWS] = {"CONTINUE", "NEW GAME", "HIGH SCORES", "QUIT"};
static const int TITLE_Y[TITLE_ROWS] = {124, 152, 168, 184};

void title_detail(const App *a, char *buf, size_t n)
{
	char city[32];

	if (!a->has_game) {
		snprintf(buf, n, "NO SAVED GAME");
		return;
	}
	city_label(a->g.loc.city, city, sizeof city);
	snprintf(buf, n, "DAY %d, %s", a->g.day, city);
}

int title_city(const Ui *u)
{
	return (int)((u->frame / 80) % NUM_CITIES);
}

void title_draw(Ui *u, Canvas *c)
{
	int ci = title_city(u), i;
	char s[48];

	art_backdrop(c, ci, 0, 240, u->frame);
	tx_outline(c, "POCKET", 160, 22, PAL_WHITE, PAL_NIGHT, 3, TX_CENTER);
	tx(c, "HUSTLE", 163, 57, PAL_HOT_DK, 4, TX_CENTER);
	tx_outline(c, "HUSTLE", 160, 54, PAL_GOLD, PAL_NIGHT, 4, TX_CENTER);
	tx_outline(c, "Buy low. Sell high. Don't get caught.", 160, 96, PAL_WHITE, PAL_NIGHT, 1, TX_CENTER);
	cv_rect(c, 72, 116, 176, 84, PAL_NIGHT);
	cv_box(c, 72, 116, 176, 84, PAL_LINE);
	for (i = 0; i < TITLE_ROWS; i++) {
		int y = TITLE_Y[i], on = u->title_row == i, h = i == T_CONTINUE ? 25 : 15;

		if (on)
			cv_rect(c, 80, y - 4, 160, h, PAL_HOT);
		tx(c, TITLE_ITEMS[i], 160, y, on ? PAL_WHITE : (i == T_CONTINUE && !u->app->has_game ? PAL_FAINT : PAL_TEXT),
		   1, TX_CENTER);
		hit(u, 80, y - 4, 160, h, HIT_TITLE_ROW, i);
	}
	title_detail(u->app, s, sizeof s);
	tx(c, s, 160, 135, u->title_row == T_CONTINUE ? PAL_WHITE : PAL_DIM, 1, TX_CENTER);
	city_label(ci, s, sizeof s);
	tx_outline(c, s, 6, 226, PAL_GOLD, PAL_NIGHT, 1, TX_LEFT);
}

static void title_continue(Ui *u)
{
	Screen s;

	if (!u->app->has_game) {
		toast(u, "No saved game. Start a new one.");
		play(u, SFX_DENY);
		return;
	}
	s = ui_resume_screen(u);
	if (s == SCR_CHASE) {
		chase_begin(u);			/* the cops and the siren, as when the chase began */
		if (!u->chase_log[0])
			snprintf(u->chase_log, sizeof u->chase_log, "The cops are on you.");
	} else {
		play(u, SFX_OK);
	}
	go(u, s);
	if (s == SCR_MARKET)
		ui_show_open_offer(u, SCR_MARKET);	/* a pending offer comes back as its card */
}

static void scores_open(Ui *u, Screen from)
{
	u->scores_back = from;
	go(u, SCR_SCORES);
}

void title_press(Ui *u, int pad)
{
	if (pad == PAD_UP)
		u->title_row = (u->title_row + TITLE_ROWS - 1) % TITLE_ROWS;
	else if (pad == PAD_DOWN)
		u->title_row = (u->title_row + 1) % TITLE_ROWS;
	else if (pad == PAD_A || pad == PAD_START) {
		switch (u->title_row) {
		case T_CONTINUE:
			title_continue(u);
			return;
		case T_NEW:
			ng_open(u, 1, u->app->g.name);
			break;
		case T_SCORES:
			/* the table of the game just finished, if it made one, else the 30 day table */
			u->scores_tab = u->app->last_rank >= 0 ? length_index(u->app->g.last_day) : 1;
			scores_open(u, SCR_TITLE);
			break;
		default:
			u->quit = 1;
			break;
		}
		play(u, SFX_OK);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void title_click(Ui *u, int action, int arg)
{
	if (action != HIT_TITLE_ROW)
		return;
	u->title_row = arg;
	ui_press(u, PAD_A);
}

/* ---- the new game screen ---- */

enum { K_SHIFT = -1, K_SPACE = -2, K_DEL = -3 };
static const char *const KEYROWS[3] = {"ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ.'!?"};

/* The letter on the key, or K_SHIFT, K_SPACE or K_DEL on the bottom row. */
static int key_at(int r, int c)
{
	return r < 3 ? KEYROWS[r][c] : c < 3 ? K_SHIFT : c < 7 ? K_SPACE : K_DEL;
}

void ng_open(Ui *u, int len, const char *name)
{
	memset(&u->ng, 0, sizeof u->ng);
	u->confirm = CONFIRM_NONE;
	u->ng.zone = 1;
	u->ng.len = len >= 0 && len < 4 ? len : 1;
	snprintf(u->ng.name, sizeof u->ng.name, "%s", name ? name : "");
	go(u, SCR_NEWGAME);
}

int ng_lower(const Ui *u)
{
	size_t n = strlen(u->ng.name);
	int lower = n > 0 && u->ng.name[n - 1] != ' ';

	return u->ng.shift ? !lower : lower;
}

static void ng_type(Ui *u)
{
	int k = key_at(u->ng.kr, u->ng.kc);
	size_t n = strlen(u->ng.name);

	if (k == K_SHIFT) {
		u->ng.shift = !u->ng.shift;
		return;
	}
	if (k == K_DEL) {
		if (n)
			u->ng.name[n - 1] = '\0';
		return;
	}
	if (n >= NG_NAME_MAX)
		return;
	u->ng.name[n] = k == K_SPACE ? ' ' : ng_lower(u) ? (char)tolower(k) : (char)k;
	u->ng.name[n + 1] = '\0';
	u->ng.shift = 0;
}

int ui_wants_text(const Ui *u)
{
	return u->scr == SCR_NEWGAME && u->confirm != CONFIRM_REPLACE;
}

void ui_text(Ui *u, const char *s)
{
	size_t n;
	int typed = 0;

	if (!ui_wants_text(u))
		return;
	n = strlen(u->ng.name);
	for (; *s; s++) {
		unsigned char ch = (unsigned char)*s;

		if (ch < 0x20 || ch > 0x7e)
			continue;
		typed = 1;
		if (n < NG_NAME_MAX) {
			u->ng.name[n++] = (char)ch;
			u->ng.name[n] = '\0';
		}
	}
	if (typed)
		u->ng.zone = 2;
}

void ng_draw(Ui *u, Canvas *c)
{
	static const Hint H[] = {{"A", "Type"}, {"B", "Delete"}, {"L1R1", "Field"}, {"START", "Go"}};
	const Params p = params_default();
	int r, col, i, nl, lower = ng_lower(u);
	char s[96], money[24], lines[3][TX_LINE_MAX];

	top_bar(u, c, "NEW GAME", "");
	cv_rect(c, 0, 12, CV_W, 212, PAL_PANEL);
	tx(c, "HOW MANY DAYS?", 16, 20, u->ng.zone == 0 ? PAL_GOLD : PAL_DIM, 1, TX_LEFT);
	for (i = 0; i < 4; i++) {
		int x = 16 + i * 74, on = u->ng.len == i;

		cv_rect(c, x, 32, 66, 16, on ? (u->ng.zone == 0 ? PAL_HOT : PAL_PANEL2) : PAL_NIGHT);
		cv_box(c, x, 32, 66, 16, on ? PAL_WHITE : PAL_LINE);
		snprintf(s, sizeof s, "%d", LENGTHS[i]);
		tx(c, s, x + 33, 36, on ? PAL_WHITE : PAL_TEXT, 1, TX_CENTER);
		hit(u, x, 32, 66, 16, HIT_LEN, i);
	}
	tx(c, "YOUR NAME", 16, 56, u->ng.zone == 1 ? PAL_GOLD : PAL_DIM, 1, TX_LEFT);
	cv_rect(c, 16, 66, 288, 16, PAL_NIGHT);
	cv_box(c, 16, 66, 288, 16, PAL_LINE);
	snprintf(s, sizeof s, "%s%s", u->ng.name, u->ng.zone == 1 && ((u->frame >> 3) & 1) ? "_" : "");
	tx(c, s, 22, 70, PAL_WHITE, 1, TX_LEFT);
	for (r = 0; r < 4; r++)
		for (col = 0; col < 10; col++) {
			int k, x, y, w, on;
			const char *lab;
			char one[2];

			if (r == 3 && col != 0 && col != 3 && col != 7)
				continue;
			k = key_at(r, col);
			x = 36 + col * 25;
			y = 90 + r * 19;
			w = r < 3 ? 23 : col == 3 ? 98 : 73;
			on = u->ng.zone == 1 && u->ng.kr == r && (r < 3 ? u->ng.kc == col : key_at(r, u->ng.kc) == k);
			cv_rect(c, x, y, w, 17, on ? PAL_HOT : PAL_NIGHT);
			cv_box(c, x, y, w, 17, on ? PAL_WHITE : PAL_LINE);
			if (r < 3) {
				one[0] = lower ? (char)tolower(k) : (char)k;
				one[1] = '\0';
				lab = one;
			} else {
				lab = k == K_SHIFT ? "SHIFT" : k == K_SPACE ? "SPACE" : "DEL";
			}
			tx(c, lab, x + (w >> 1), y + 5,
			   on ? PAL_WHITE : (k == K_SHIFT && u->ng.shift ? PAL_GOLD : PAL_TEXT), 1, TX_CENTER);
			hit(u, x, y, w, 17, HIT_KEY, r * 10 + col);
		}
	button(u, c, 70, 172, 180, 20, "START HUSTLING", u->ng.zone == 2, HIT_NG_GO, 0);
	fmt_money(p.start_cash, money, sizeof money);
	snprintf(s, sizeof s, "You start in %s with %s", city_name(p.start_city), money);
	fmt_money(p.start_debt, money, sizeof money);
	snprintf(s + strlen(s), sizeof s - strlen(s), " and owe %s %s.", CITIES[p.start_city].lender, money);
	nl = tx_wrap(s, 38, lines, 3);
	for (i = 0; i < nl; i++)
		tx(c, lines[i], 16, 200 + i * 10, PAL_DIM, 1, TX_LEFT);
	hints(c, H, 4, 229);
	if (u->confirm == CONFIRM_REPLACE)
		prompt(u, c, 40, 90, 240, "REPLACE YOUR SAVED GAME?");
}

static void ng_start(Ui *u)
{
	Game *g = &u->app->g;
	char name[16] = "";
	const char *a = u->ng.name;
	size_t n;
	Card first;

	while (*a == ' ')
		a++;
	snprintf(name, sizeof name, "%s", a);
	for (n = strlen(name); n && name[n - 1] == ' '; n--)
		name[n - 1] = '\0';
	if (!name[0])
		snprintf(name, sizeof name, "Player");
	if (app_new_game(u->app, LENGTHS[u->ng.len], name) != 0) {
		toast(u, "Couldn't start a game.");
		play(u, SFX_DENY);
		return;
	}
	app_save_now(u->app);
	play(u, SFX_OK);
	/* the opening card, then whatever the first market rolled */
	memset(&first, 0, sizeof first);
	first.kind = CARD_DAY;
	first.good = -1;
	{
		char debt[24];

		fmt_money(g->debt[g->loc.city], debt, sizeof debt);
		snprintf(first.text, sizeof first.text,
			 "Day 1 in %s. You owe %s %s and you have %d days. Make it back, and then some.",
			 city_name(g->loc.city), CITIES[g->loc.city].lender, debt, g->last_day);
	}
	ui_collect_news(u, SCR_MARKET);
	if (u->scr == SCR_NEWS) {
		memmove(&u->news.cards[1], &u->news.cards[0], (size_t)u->news.n * sizeof u->news.cards[0]);
		u->news.n++;
	} else {
		u->news.n = 1;
		u->news.i = 0;
		u->news.after = SCR_MARKET;
		overlay(u, SCR_NEWS, SCR_MARKET);
	}
	u->news.cards[0] = first;
}

/* With a game in progress, ask before replacing it. */
static void ng_begin(Ui *u)
{
	if (u->app->has_game) {
		u->confirm = CONFIRM_REPLACE;
		play(u, SFX_MOVE);
		return;
	}
	ng_start(u);
}

void ng_press(Ui *u, int pad)
{
	int *zone = &u->ng.zone;

	if (u->confirm == CONFIRM_REPLACE) {
		if (pad == PAD_A) {
			u->confirm = CONFIRM_NONE;
			ng_start(u);
		} else if (pad == PAD_B) {
			u->confirm = CONFIRM_NONE;
			play(u, SFX_BACK);
		}
		return;
	}

	if (pad == PAD_L1) {
		*zone = (*zone + 2) % 3;
	} else if (pad == PAD_R1) {
		*zone = (*zone + 1) % 3;
	} else if (pad == PAD_START) {
		ng_begin(u);
		return;
	} else if (*zone == 0) {
		if (pad == PAD_LEFT)
			u->ng.len = (u->ng.len + 3) % 4;
		else if (pad == PAD_RIGHT)
			u->ng.len = (u->ng.len + 1) % 4;
		else if (pad == PAD_DOWN || pad == PAD_A)
			*zone = 1;
		else if (pad == PAD_B) {
			go(u, SCR_TITLE);
			play(u, SFX_BACK);
			return;
		} else
			return;
	} else if (*zone == 1) {
		if (pad == PAD_UP) {
			if (u->ng.kr == 0)
				*zone = 0;
			else
				u->ng.kr--;
		} else if (pad == PAD_DOWN) {
			if (u->ng.kr == 3)
				*zone = 2;
			else
				u->ng.kr++;
		} else if (pad == PAD_LEFT)
			u->ng.kc = (u->ng.kc + 9) % 10;
		else if (pad == PAD_RIGHT)
			u->ng.kc = (u->ng.kc + 1) % 10;
		else if (pad == PAD_A)
			ng_type(u);
		else if (pad == PAD_B) {
			size_t n = strlen(u->ng.name);

			if (n)
				u->ng.name[n - 1] = '\0';
		} else
			return;
	} else {
		if (pad == PAD_UP || pad == PAD_B)
			*zone = 1;
		else if (pad == PAD_A) {
			ng_begin(u);
			return;
		} else
			return;
	}
	play(u, SFX_MOVE);
}

void ng_click(Ui *u, int action, int arg)
{
	if (action == HIT_CONFIRM) {
		if (arg >= 0)
			ui_press(u, arg ? PAD_A : PAD_B);
	} else if (action == HIT_LEN) {
		u->ng.zone = 0;
		u->ng.len = arg;
	} else if (action == HIT_KEY) {
		u->ng.zone = 1;
		u->ng.kr = arg / 10;
		u->ng.kc = arg % 10;
		ng_type(u);
		play(u, SFX_MOVE);
	} else if (action == HIT_NG_GO) {
		u->ng.zone = 2;
		ng_begin(u);
	}
}

/* ---- the pause menu ---- */

enum { M_RESUME, M_SOUND, M_SCORES, M_SAVEQUIT, M_END };

void menu_label(const App *a, int row, char *buf, size_t n)
{
	static const char *const L[MENU_ROWS] = {"RESUME", NULL, "HIGH SCORES", "SAVE AND QUIT", "END GAME NOW"};

	if (row == M_SOUND)
		snprintf(buf, n, "SOUND: %s", a->sound_on ? "ON" : "OFF");
	else
		snprintf(buf, n, "%s", row >= 0 && row < MENU_ROWS ? L[row] : "");
}

void menu_open(Ui *u, Screen from)
{
	u->menu_row = 0;
	u->confirm = CONFIRM_NONE;
	u->menu_back = from;
	overlay(u, SCR_MENU, from);
	play(u, SFX_OK);
}

void menu_draw(Ui *u, Canvas *c)
{
	enum { X = 72, Y = 50, W = 176, H = 140 };
	char s[40];
	int i;

	cv_rect(c, X, Y, W, H, PAL_PANEL);
	cv_box(c, X, Y, W, H, PAL_LINE_HI);
	cv_rect(c, X + 1, Y + 1, W - 2, 16, PAL_PANEL2);
	tx(c, "PAUSED", X + W / 2, Y + 5, PAL_GOLD, 1, TX_CENTER);
	for (i = 0; i < MENU_ROWS; i++) {
		int y = Y + 26 + i * 18, on = u->menu_row == i;

		if (on)
			cv_rect(c, X + 8, y - 4, W - 16, 15, PAL_HOT);
		menu_label(u->app, i, s, sizeof s);
		tx(c, s, X + W / 2, y, on ? PAL_WHITE : PAL_TEXT, 1, TX_CENTER);
		hit(u, X + 8, y - 4, W - 16, 15, HIT_MENU_ROW, i);
	}
	if (u->confirm == CONFIRM_END) {
		cv_rect(c, X + 1, Y + 110, W - 2, H - 111, PAL_PANEL);
		hit(u, 0, 0, CV_W, CV_H, HIT_CONFIRM, -1);
		tx(c, "END THE GAME?", X + W / 2, Y + 114, PAL_CORAL, 1, TX_CENTER);
		button(u, c, X + 8, Y + 122, 76, 16, "A YES", 1, HIT_CONFIRM, 1);
		button(u, c, X + W - 84, Y + 122, 76, 16, "B NO", 0, HIT_CONFIRM, 0);
		return;
	}
	snprintf(s, sizeof s, "Autosaved on day %d.", u->app->g.day);
	tx(c, s, X + W / 2, Y + H - 14, PAL_DIM, 1, TX_CENTER);
}

static int end_call(Game *g, const void *arg)
{
	(void)arg;
	return game_end(g);
}

static void menu_act(Ui *u)
{
	App *a = u->app;

	switch (u->menu_row) {
	case M_RESUME:
		go(u, u->menu_back);
		break;
	case M_SOUND:
		a->sound_on = !a->sound_on;
		if (app_options_write(a) != 0)
			toast(u, "Couldn't save the setting.");
		break;
	case M_SCORES:
		u->scores_tab = length_index(a->g.last_day);
		scores_open(u, SCR_MENU);
		break;
	case M_SAVEQUIT:
		if (app_save_now(a) != 0) {
			toast(u, "Couldn't save the game.");
			play(u, SFX_DENY);
			return;
		}
		u->title_row = 0;
		go(u, SCR_TITLE);
		break;
	default:
		if (u->confirm != CONFIRM_END) {	/* ask first; the second A ends it */
			u->confirm = CONFIRM_END;
			play(u, SFX_MOVE);
			return;
		}
		u->confirm = CONFIRM_NONE;
		if (ui_core_call(u, end_call, NULL, -1, 0) != OK)
			return;
		go(u, SCR_OVER);
		break;
	}
	play(u, SFX_OK);
}

void menu_press(Ui *u, int pad)
{
	if (u->confirm == CONFIRM_END) {
		if (pad == PAD_A) {
			menu_act(u);
			return;
		}
		u->confirm = CONFIRM_NONE;		/* B, START or moving away cancels */
		if (pad == PAD_B || pad == PAD_START) {
			play(u, SFX_BACK);
			return;
		}
	}
	if (pad == PAD_UP)
		u->menu_row = (u->menu_row + MENU_ROWS - 1) % MENU_ROWS;
	else if (pad == PAD_DOWN)
		u->menu_row = (u->menu_row + 1) % MENU_ROWS;
	else if (pad == PAD_B || pad == PAD_START) {
		go(u, u->menu_back);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		menu_act(u);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void menu_click(Ui *u, int action, int arg)
{
	if (action == HIT_CONFIRM) {
		if (arg >= 0)
			ui_press(u, arg ? PAD_A : PAD_B);
		return;
	}
	if (action != HIT_MENU_ROW)
		return;
	u->menu_row = arg;
	ui_press(u, PAD_A);
}

/* ---- game over ---- */

void over_title(const Game *g, char *buf, size_t n)
{
	snprintf(buf, n, "%s", g->end == END_DAYS_UP ? "TIME'S UP" : g->end == END_DEAD ? "GAME OVER" :
		 "RETIRED EARLY");
}

void over_rank(const App *a, char *buf, size_t n)
{
	if (a->last_rank >= 0)
		snprintf(buf, n, "HIGH SCORE #%d FOR %d DAYS", a->last_rank + 1, a->g.last_day);
	else
		snprintf(buf, n, "NO HIGH SCORE THIS TIME");
}

int over_stats(const Game *g, StatCell out[12])
{
	const Stats *s = &g->stats;
	const struct { const char *label; int32_t v; } N[9] = {
		{"Days", g->last_day}, {"Moves", s->moves}, {"Flights", s->flights}, {"Chases", s->cop_chases},
		{"Beat cops", s->cops_beaten}, {"Escapes", s->escapes}, {"Jailed", s->times_jailed},
		{"Mugged", s->times_mugged}, {"Goons", s->goon_visits},
	};
	int i;

	for (i = 0; i < 9; i++) {
		snprintf(out[i].label, sizeof out[i].label, "%s", N[i].label);
		snprintf(out[i].value, sizeof out[i].value, "%d", (int)N[i].v);
	}
	snprintf(out[9].label, sizeof out[9].label, "ATM fees");
	/* the mockup's $12.3K form; past $10B a form that stays in its 7 cells */
	if (s->bank_fees_paid < 10000000000LL)
		chart_k(s->bank_fees_paid, out[9].value, sizeof out[9].value);
	else if (s->bank_fees_paid < 1000000000000LL)
		fmt_money_short(s->bank_fees_paid, out[9].value, sizeof out[9].value);
	else
		snprintf(out[9].value, sizeof out[9].value, "$%lldT", (long long)(s->bank_fees_paid / 1000000000000LL));
	snprintf(out[10].label, sizeof out[10].label, "Wild wins");
	snprintf(out[10].value, sizeof out[10].value, "%d", (int)s->wild_wins);
	snprintf(out[11].label, sizeof out[11].label, "Wild busts");
	snprintf(out[11].value, sizeof out[11].value, "%d", (int)s->wild_losses);
	return 12;
}

void over_draw(Ui *u, Canvas *c)
{
	static const Hint H[] = {{"A", "New game"}, {"B", "Title"}};
	const Game *g = &u->app->g;
	int64_t worth = game_net_worth(g);
	char s[64];
	StatCell cells[12];
	int i, n, city = g->loc.city < NUM_CITIES ? g->loc.city : 0;

	art_backdrop(c, city, 0, 240, u->frame);
	cv_dim(c);
	cv_dim(c);
	over_title(g, s, sizeof s);
	tx_outline(c, s, 160, 16, g->end == END_DEAD ? PAL_CORAL : PAL_GOLD, PAL_BLACK, 2, TX_CENTER);
	tx(c, "FINAL NET WORTH", 160, 44, PAL_DIM, 1, TX_CENTER);
	fmt_money_short(worth, s, sizeof s);
	tx_outline(c, s, 160, 58, worth >= 0 ? PAL_GOLD : PAL_CORAL, PAL_BLACK, strlen(s) > 12 ? 2 : 3, TX_CENTER);
	over_rank(u->app, s, sizeof s);
	tx(c, s, 160, 90, u->app->last_rank >= 0 ? PAL_HOT : PAL_DIM, 1, TX_CENTER);
	cv_rect(c, 24, 104, 272, 96, PAL_PANEL);
	cv_box(c, 24, 104, 272, 96, PAL_LINE);
	n = over_stats(g, cells);
	for (i = 0; i < n; i++) {
		int x = i % 2 ? 168 : 32, y = 110 + (i / 2) * 14;

		tx(c, cells[i].label, x, y, PAL_DIM, 1, TX_LEFT);
		tx(c, cells[i].value, x + 120, y, PAL_TEXT, 1, TX_RIGHT);
	}
	hints(c, H, 2, HINTS_Y);
}

void over_press(Ui *u, int pad)
{
	if (pad == PAD_A) {
		ng_open(u, length_index(u->app->g.last_day), u->app->g.name);
		play(u, SFX_OK);
	} else if (pad == PAD_B) {
		u->title_row = 0;
		go(u, SCR_TITLE);
		play(u, SFX_BACK);
	}
}

/* ---- the high scores ---- */

int scores_hilite(const App *a, int tab)
{
	if (tab < 0 || tab > 3 || a->last_rank < 0 || days_index(a->g.last_day) != tab ||
	    a->last_rank >= a->scores.n[tab])
		return -1;
	return a->last_rank;
}

void scores_draw(Ui *u, Canvas *c)
{
	static const Hint H[] = {{"L1R1", "Length"}, {"B", "Back"}};
	const App *a = u->app;
	int tab = u->scores_tab >= 0 && u->scores_tab < 4 ? u->scores_tab : 1;
	int i, hi = scores_hilite(a, tab);
	char s[32];

	top_bar(u, c, "HIGH SCORES", "");
	cv_rect(c, 0, 12, CV_W, 212, PAL_PANEL);
	for (i = 0; i < 4; i++) {
		int x = 24 + i * 70, on = tab == i;

		cv_rect(c, x, 18, 62, 14, on ? PAL_HOT : PAL_NIGHT);
		cv_box(c, x, 18, 62, 14, on ? PAL_WHITE : PAL_LINE);
		snprintf(s, sizeof s, "%d DAYS", LENGTHS[i]);
		tx(c, s, x + 31, 21, on ? PAL_WHITE : PAL_TEXT, 1, TX_CENTER);
		hit(u, x, 18, 62, 14, HIT_SCORES_TAB, i);
	}
	tx(c, "#", 16, 42, PAL_DIM, 1, TX_LEFT);
	tx(c, "NAME", 40, 42, PAL_DIM, 1, TX_LEFT);
	tx(c, "NET WORTH", 304, 42, PAL_DIM, 1, TX_RIGHT);
	cv_rect(c, 12, 52, 296, 1, PAL_LINE);
	if (!a->scores.n[tab])
		tx(c, "No scores yet.", 160, 80, PAL_DIM, 1, TX_CENTER);
	for (i = 0; i < a->scores.n[tab] && i < 10; i++) {
		const HiScore *e = &a->scores.t[tab][i];
		int y = 58 + i * 14, me = i == hi;

		if (me)
			cv_rect(c, 12, y - 3, 296, 13, PAL_PANEL2);
		snprintf(s, sizeof s, "%d", i + 1);
		tx(c, s, 16, y, me ? PAL_GOLD : PAL_TEXT, 1, TX_LEFT);
		tx(c, e->name, 40, y, me ? PAL_GOLD : PAL_TEXT, 1, TX_LEFT);
		if (e->end == END_DEAD)
			tx(c, "DEAD", 168, y, PAL_CORAL, 1, TX_LEFT);
		else if (e->end == END_QUIT)
			tx(c, "QUIT", 168, y, PAL_DIM, 1, TX_LEFT);
		fmt_money_short(e->score, s, sizeof s);
		tx(c, s, 304, y, e->score < 0 ? PAL_CORAL : me ? PAL_GOLD : PAL_TEXT, 1, TX_RIGHT);
	}
	hints(c, H, 2, HINTS_Y);
}

void scores_press(Ui *u, int pad)
{
	if (pad == PAD_LEFT || pad == PAD_L1)
		u->scores_tab = (u->scores_tab + 3) % 4;
	else if (pad == PAD_RIGHT || pad == PAD_R1)
		u->scores_tab = (u->scores_tab + 1) % 4;
	else if (pad == PAD_B || pad == PAD_A) {
		if (u->scores_back == SCR_MENU)
			overlay(u, SCR_MENU, u->menu_back);
		else
			go(u, SCR_TITLE);
		play(u, SFX_BACK);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void scores_click(Ui *u, int action, int arg)
{
	if (action == HIT_SCORES_TAB)
		u->scores_tab = arg;
}
