/* Getting around and flying: the mockup's drawMove/inMove and drawFly/inFly. Moving inside
 * a city is a free ride on the local line, a flight costs a fare; either one is a day. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

/* Each city's transit line color (art, so it lives with the screen). */
static const uint32_t LINE_COL[NUM_CITIES] = {
	0xd94f8au, 0xe0463cu, 0x3c8fe0u, 0xf2a33au, 0x3cc47au, 0x2fa36au, 0xe0b43cu,
	0x8a5ce0u, 0x2fc4c8u, 0xb4e03cu, 0x3c6ae0u, 0xe03c5au, 0x9ad0f0u,
	0xf0e08au, 0xe05ac8u, 0x3ce0b0u, 0xf09a5au, 0xc0a0f0u,
	0xc07a3au, 0xb8c4d4u,	/* Chicago's brown L, Detroit's chrome */
};
static const Hint GO_HINTS[3] = {{"A", "Ride"}, {"B", "Back"}, {"START", "Menu"}};

static int city_of(const Game *g)
{
	return g->loc.city < NUM_CITIES ? g->loc.city : 0;
}

static void upper(const char *s, char *out, size_t n)
{
	size_t i;

	for (i = 0; s[i] && i + 1 < n; i++)
		out[i] = (char)toupper((unsigned char)s[i]);
	out[i] = '\0';
}

/* ---- getting around ---- */

void move_open(Ui *u)
{
	go(u, SCR_MOVE);
	u->row = u->app->g.loc.hood < HOODS_PER_CITY ? u->app->g.loc.hood : 0;
}

void move_draw(Ui *u, Canvas *c)
{
	enum { TX = 212, TY = 96, TW = 100, TH = 72 };
	const Game *g = &u->app->g;
	int city = city_of(g), here = g->loc.hood, i;
	const CityDef *cd = &CITIES[city];
	uint32_t lc = LINE_COL[city];
	const char *transit = cd->transit;
	char s[48];

	if (u->row < 0 || u->row >= HOODS_PER_CITY)
		u->row = 0;
	top_bar(u, c, NULL, NULL);
	art_backdrop(c, city, 12, 56, u->frame);
	place_label(c, city, cd->hoods[here < HOODS_PER_CITY ? here : 0], 68);
	cv_rect(c, 0, 68, CV_W, 156, PAL_PANEL);
	cv_rect(c, 0, 68, CV_W, 1, PAL_LINE);
	if (!strncmp(transit, "the ", 4))
		transit += 4;
	upper(transit, s, sizeof s);
	tx(c, s, 8, 74, PAL_GOLD, 1, TX_LEFT);
	tx(c, "PICK A STOP", 8, 84, PAL_DIM, 1, TX_LEFT);
	if (cd->bank_hoods) {		/* across the border there are no branches to point out */
		art_icon(c, ICON_BANK, 112, 84, 1, 0);
		tx(c, "= BANK BRANCH", 122, 84, PAL_DIM, 1, TX_LEFT);
	}
	cv_rect(c, 22, 100, 4, 100, lc);
	for (i = 0; i < HOODS_PER_CITY; i++) {
		int y = 100 + i * 20, sel = u->row == i, at = here == i;

		if (sel)
			cv_rect(c, 34, y - 6, 166, 13, PAL_HOT);
		cv_disc(c, 24, y, 5, lc);
		cv_disc(c, 24, y, 3, at ? PAL_GOLD : PAL_WHITE);
		tx(c, cd->hoods[i], 40, y - 3, sel ? PAL_WHITE : at ? PAL_GOLD : PAL_TEXT, 1, TX_LEFT);
		if (cd->bank_hoods >> i & 1)
			art_icon(c, ICON_BANK, 6, y - 3, 1, 0);
		if (at)
			tx(c, "HERE", 196, y - 3, sel ? PAL_WHITE : PAL_GOLD, 1, TX_RIGHT);
		hit(u, 10, y - 8, 192, 16, HIT_MOVE_ROW, i);
	}
	cv_rect(c, TX, TY, TW, TH, PAL_PAPER);			/* the ticket */
	cv_disc(c, TX, TY + 36, 5, PAL_PANEL);
	cv_disc(c, TX + TW, TY + 36, 5, PAL_PANEL);
	for (i = 0; i < 23; i++)
		cv_rect(c, TX + 4 + i * 4, TY + 14, 2, 1, PAL_INK);
	tx(c, "ADMIT ONE", TX + TW / 2, TY + 4, PAL_INK, 1, TX_CENTER);
	tx(c, "FREE RIDE", TX + TW / 2, TY + 20, PAL_INK, 1, TX_CENTER);
	tx(c, "TAKES 1 DAY", TX + TW / 2, TY + 31, PAL_INK, 1, TX_CENTER);
	tx(c, cd->code, TX + TW / 2, TY + 46, lc, 2, TX_CENTER);
	hints(c, GO_HINTS, 3, HINTS_Y);
}

static int move_call(Game *g, const void *arg)
{
	return game_move(g, *(const Loc *)arg);
}

static void ride(Ui *u, int hood)
{
	Game *g = &u->app->g;
	Loc dest = {g->loc.city, (uint8_t)hood};

	if (hood == g->loc.hood) {
		toast(u, "You're already here.");
		play(u, SFX_DENY);
		return;
	}
	ui_core_call(u, move_call, &dest, SFX_OK, 1);
}

void move_press(Ui *u, int pad)
{
	if (pad == PAD_UP)
		u->row = (u->row + HOODS_PER_CITY - 1) % HOODS_PER_CITY;
	else if (pad == PAD_DOWN)
		u->row = (u->row + 1) % HOODS_PER_CITY;
	else if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		ride(u, u->row);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void move_click(Ui *u, int action, int arg)
{
	if (action != HIT_MOVE_ROW)
		return;
	u->row = arg;
	ui_press(u, PAD_A);
}

/* ---- flying ---- */

/* Two columns of up to eight rows (sixteen US cities seen from Mexico), between the tabs
 * (TAB_Y..84) and the boarding pass (from y 168). A row's band runs from y - 1 for GRID_STEP. */
enum { GRID_ROWS = 8, GRID_Y = 88, GRID_STEP = 10, GRID_COL_W = 156, TAB_Y = 72 };

int fly_count(const Ui *u)
{
	const Game *g = &u->app->g;
	int c, n = 0;

	for (c = 0; c < NUM_CITIES; c++)
		if (c != city_of(g) && CITIES[c].country == u->fly_tab)
			n++;
	return n;
}

int fly_city(const Ui *u, int place)
{
	const Game *g = &u->app->g;
	int c;

	if (place < 0)
		return -1;
	for (c = 0; c < NUM_CITIES; c++)
		if (c != city_of(g) && CITIES[c].country == u->fly_tab && place-- == 0)
			return c;
	return -1;
}

void fly_open(Ui *u)
{
	go(u, SCR_FLY);
	u->fly_tab = CITIES[city_of(&u->app->g)].country;
	u->row = 0;
}

void fly_pass_text(const Ui *u, int dest, PassText *t)
{
	const Game *g = &u->app->g;
	int from = city_of(g);
	char name[16], hood[16];

	memset(t, 0, sizeof *t);
	if (dest < 0 || dest >= NUM_CITIES)
		return;
	upper(g->name[0] ? g->name : "YOU", name, sizeof name);
	snprintf(t->passenger, sizeof t->passenger, "PASSENGER %s", name);
	snprintf(t->route, sizeof t->route, "FROM %s  TO %s", CITIES[from].code, CITIES[dest].code);
	upper(CITIES[dest].hoods[0], hood, sizeof hood);
	snprintf(t->lands, sizeof t->lands, "LANDS %s", hood);
	if (game_city_banned(g, dest)) {
		snprintf(t->fare, sizeof t->fare, "BANNED");
		t->fare_col = PAL_CORAL;
	} else {
		int64_t fare = game_flight_cost(g, dest);

		fmt_money_short(fare, t->fare, sizeof t->fare);
		t->fare_col = fare > g->cash ? PAL_CORAL : 0xb0195au;
	}
	if (game_flight_crosses(g, dest))
		snprintf(t->border, sizeof t->border, "BORDER CROSSING");
}

static void draw_tabs(Ui *u, Canvas *c)
{
	static const char *const NAME[2] = {"USA", "MEXICO"};
	static const int X[2] = {8, 64}, W[2] = {52, 68};
	int t, from = city_of(&u->app->g);
	char s[32];

	for (t = 0; t < 2; t++) {
		int on = u->fly_tab == t;

		cv_rect(c, X[t], TAB_Y, W[t], 13, on ? PAL_HOT : PAL_NIGHT);
		cv_box(c, X[t], TAB_Y, W[t], 13, on ? PAL_WHITE : PAL_LINE);
		tx(c, NAME[t], X[t] + (W[t] >> 1), TAB_Y + 3, on ? PAL_WHITE : PAL_TEXT, 1, TX_CENTER);
		hit(u, X[t], TAB_Y, W[t], 13, HIT_FLY_TAB, t);
	}
	if (u->fly_tab != CITIES[from].country)
		snprintf(s, sizeof s, "CROSS THE BORDER");
	else
		snprintf(s, sizeof s, "FLIGHTS FROM %s", CITIES[from].code);
	tx(c, s, 312, TAB_Y + 3, PAL_GOLD, 1, TX_RIGHT);
}

static void draw_grid(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	int n = fly_count(u), i;
	char s[32];

	for (i = 0; i < n; i++) {
		int city = fly_city(u, i), col = i / GRID_ROWS, x = 4 + col * GRID_COL_W;
		int y = GRID_Y + (i % GRID_ROWS) * GRID_STEP, sel = u->row == i;
		int banned = game_city_banned(g, city);
		int64_t fare = game_flight_cost(g, city);

		if (sel)
			cv_rect(c, x, y - 1, GRID_COL_W - 4, GRID_STEP, PAL_HOT);
		tx(c, CITIES[city].code, x + 4, y, sel ? PAL_WHITE : PAL_GOLD, 1, TX_LEFT);
		tx(c, CITIES[city].short_name, x + 32, y, sel ? PAL_WHITE : banned ? PAL_DIM : PAL_TEXT, 1, TX_LEFT);
		if (banned) {
			snprintf(s, sizeof s, "BANNED");
		} else {
			fmt_money_short(fare, s, sizeof s);
		}
		tx(c, s, x + GRID_COL_W - 8, y, sel ? PAL_WHITE : banned || fare > g->cash ? PAL_CORAL : PAL_TEXT,
		   1, TX_RIGHT);
		hit(u, x, y - 1, GRID_COL_W - 4, GRID_STEP, HIT_FLY_ROW, i);
	}
}

static void draw_pass(Canvas *c, const PassText *t)
{
	enum { BX = 8, BY = 168, BW = 304, BH = 54 };
	int i;

	cv_rect(c, BX, BY, BW, BH, PAL_PAPER);
	cv_rect(c, BX, BY, BW, 11, PAL_SKY);
	tx(c, "BOARDING PASS", BX + 4, BY + 2, PAL_INK, 1, TX_LEFT);
	tx(c, "1 DAY", BX + BW - 4, BY + 2, PAL_INK, 1, TX_RIGHT);
	for (i = 0; i < 10; i++)
		cv_rect(c, BX + 230, BY + 14 + i * 4, 1, 2, PAL_INK);
	tx(c, t->passenger, BX + 4, BY + 14, PAL_INK, 1, TX_LEFT);
	tx(c, t->route, BX + 4, BY + 24, PAL_INK, 1, TX_LEFT);
	tx(c, t->lands, BX + 4, BY + 34, PAL_INK, 1, TX_LEFT);
	if (t->border[0])
		tx(c, t->border, BX + 4, BY + 44, 0xb0195au, 1, TX_LEFT);
	tx(c, "FARE", BX + 267, BY + 18, PAL_INK, 1, TX_CENTER);
	tx(c, t->fare, BX + 267, BY + 32, t->fare_col, 1, TX_CENTER);
}

void fly_draw(Ui *u, Canvas *c)
{
	static const Hint H[4] = {{"A", "Fly"}, {"B", "Back"}, {"L1R1", "Country"}, {"START", "Menu"}};
	const Game *g = &u->app->g;
	int dest;
	char s[64];
	PassText t;

	if (u->fly_tab < 0 || u->fly_tab > 1)
		u->fly_tab = 0;
	if (u->row < 0 || u->row >= fly_count(u))
		u->row = 0;
	dest = fly_city(u, u->row);
	if (dest < 0)
		dest = city_of(g) ? 0 : 1;
	top_bar(u, c, NULL, NULL);
	art_backdrop(c, dest, 12, 56, u->frame);
	snprintf(s, sizeof s, "Lands at %s", CITIES[dest].hoods[0]);
	place_label(c, dest, s, 68);
	cv_rect(c, 0, 68, CV_W, 156, PAL_PANEL);
	cv_rect(c, 0, 68, CV_W, 1, PAL_LINE);
	draw_tabs(u, c);
	draw_grid(u, c);
	fly_pass_text(u, dest, &t);
	draw_pass(c, &t);
	hints(c, H, 4, HINTS_Y);
}

static void fly(Ui *u, int city)
{
	Game *g = &u->app->g;
	Loc dest = {(uint8_t)city, 0};
	char msg[64];

	if (city < 0)
		return;
	if (game_city_banned(g, city)) {
		snprintf(msg, sizeof msg, "You're not welcome in %s anymore.", CITIES[city].name);
		toast(u, msg);
		play(u, SFX_DENY);
		return;
	}
	if (game_flight_cost(g, city) > g->cash) {
		toast(u, "You can't afford the ticket.");
		play(u, SFX_DENY);
		return;
	}
	ui_core_call(u, move_call, &dest, SFX_OK, 1);
}

/* The grid fills down the left column, then the right: place = col * GRID_ROWS + row. */
static int grid_move(const Ui *u, int pad)
{
	int n = fly_count(u), col = u->row / GRID_ROWS, row = u->row % GRID_ROWS;
	int in_col = n - col * GRID_ROWS > GRID_ROWS ? GRID_ROWS : n - col * GRID_ROWS;
	int to;

	if (pad == PAD_UP)
		return col * GRID_ROWS + (row + in_col - 1) % in_col;
	if (pad == PAD_DOWN)
		return col * GRID_ROWS + (row + 1) % in_col;
	if (pad != PAD_LEFT && pad != PAD_RIGHT)
		return u->row;
	to = pad == PAD_RIGHT ? col + 1 : col - 1;
	if (to < 0 || to * GRID_ROWS >= n)
		return u->row;
	return to * GRID_ROWS + (row < GRID_ROWS && to * GRID_ROWS + row < n ? row : n - 1 - to * GRID_ROWS);
}

void fly_press(Ui *u, int pad)
{
	if (pad == PAD_UP || pad == PAD_DOWN || pad == PAD_LEFT || pad == PAD_RIGHT) {
		int to = grid_move(u, pad);

		if (to == u->row)
			return;
		u->row = to;
	} else if (pad == PAD_L1 || pad == PAD_R1) {
		int tab = pad == PAD_R1 ? 1 : 0;

		if (tab == u->fly_tab)
			return;
		u->fly_tab = tab;
		u->row = 0;
	} else if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		fly(u, fly_city(u, u->row));
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void fly_click(Ui *u, int action, int arg)
{
	if (action == HIT_FLY_TAB) {
		if (arg != u->fly_tab) {
			u->fly_tab = arg;
			u->row = 0;
			play(u, SFX_MOVE);
		}
		return;
	}
	if (action != HIT_FLY_ROW)
		return;
	u->row = arg;
	ui_press(u, PAD_A);
}
