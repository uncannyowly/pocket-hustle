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
};
static const Hint GO_HINTS[3] = {{"A", "Ride"}, {"B", "Back"}, {"START", "Menu"}};
static const Hint FLY_HINTS[3] = {{"A", "Fly"}, {"B", "Back"}, {"START", "Menu"}};

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
	art_icon(c, ICON_BANK, 112, 84, 1, 0);
	tx(c, "= BANK BRANCH", 122, 84, PAL_DIM, 1, TX_LEFT);
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

static void ride(Ui *u, int hood)
{
	Game *g = &u->app->g;
	int16_t day = g->day;
	Phase ph = g->phase;
	Loc dest = {g->loc.city, (uint8_t)hood};
	int r;

	if (hood == g->loc.hood) {
		toast(u, "You're already here.");
		play(u, SFX_DENY);
		return;
	}
	r = game_move(g, dest);
	if (r != OK) {
		toast(u, err_text(r));
		play(u, SFX_DENY);
		return;
	}
	play(u, SFX_OK);
	ui_after_action(u, day, ph);
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

int fly_city(const Game *g, int row)
{
	if (row < 0 || row >= NUM_CITIES - 1)
		return -1;
	return row < g->loc.city ? row : row + 1;
}

void fly_open(Ui *u)
{
	go(u, SCR_FLY);
	u->row = 0;
}

void fly_draw(Ui *u, Canvas *c)
{
	enum { BX = 8, BY = 158, BW = 304, BH = 60 };
	const Game *g = &u->app->g;
	int from = city_of(g), dest, i;
	char s[64], name[16];
	int64_t fare;

	if (u->row < 0 || u->row >= NUM_CITIES - 1)
		u->row = 0;
	dest = fly_city(g, u->row);
	fare = game_flight_cost(g, dest);
	top_bar(u, c, NULL, NULL);
	art_backdrop(c, dest, 12, 56, u->frame);
	snprintf(s, sizeof s, "Lands at %s", CITIES[dest].hoods[0]);
	place_label(c, dest, s, 68);
	cv_rect(c, 0, 68, CV_W, 156, PAL_PANEL);
	cv_rect(c, 0, 68, CV_W, 1, PAL_LINE);
	snprintf(s, sizeof s, "FLIGHTS FROM %s", CITIES[from].code);
	tx(c, s, 8, 74, PAL_GOLD, 1, TX_LEFT);
	for (i = 0; i < NUM_CITIES - 1; i++) {
		int city = fly_city(g, i), y = 88 + i * 11, sel = u->row == i;
		int64_t f = game_flight_cost(g, city);

		if (sel)
			cv_rect(c, 4, y - 2, 312, 11, PAL_HOT);
		tx(c, CITIES[city].code, 10, y, sel ? PAL_WHITE : PAL_GOLD, 1, TX_LEFT);
		tx(c, CITIES[city].name, 46, y, sel ? PAL_WHITE : PAL_TEXT, 1, TX_LEFT);
		fmt_money(f, s, sizeof s);
		tx(c, s, 310, y, sel ? PAL_WHITE : f > g->cash ? PAL_CORAL : PAL_TEXT, 1, TX_RIGHT);
		hit(u, 4, y - 2, 312, 11, HIT_FLY_ROW, i);
	}
	cv_rect(c, BX, BY, BW, BH, PAL_PAPER);			/* the boarding pass */
	cv_rect(c, BX, BY, BW, 11, PAL_SKY);
	tx(c, "BOARDING PASS", BX + 4, BY + 2, PAL_INK, 1, TX_LEFT);
	tx(c, "1 DAY", BX + BW - 4, BY + 2, PAL_INK, 1, TX_RIGHT);
	for (i = 0; i < 12; i++)
		cv_rect(c, BX + 230, BY + 14 + i * 4, 1, 2, PAL_INK);
	upper(g->name[0] ? g->name : "YOU", name, sizeof name);
	snprintf(s, sizeof s, "PASSENGER %s", name);
	tx(c, s, BX + 4, BY + 16, PAL_INK, 1, TX_LEFT);
	snprintf(s, sizeof s, "FROM %s  TO %s", CITIES[from].code, CITIES[dest].code);
	tx(c, s, BX + 4, BY + 28, PAL_INK, 1, TX_LEFT);
	upper(CITIES[dest].hoods[0], name, sizeof name);
	snprintf(s, sizeof s, "LANDS %s", name);
	tx(c, s, BX + 4, BY + 40, PAL_INK, 1, TX_LEFT);
	tx(c, "FARE", BX + 267, BY + 18, PAL_INK, 1, TX_CENTER);
	fmt_money(fare, s, sizeof s);
	tx(c, s, BX + 267, BY + 32, fare > g->cash ? PAL_CORAL : 0xb0195au, 1, TX_CENTER);
	hints(c, FLY_HINTS, 3, HINTS_Y);
}

static void fly(Ui *u, int city)
{
	Game *g = &u->app->g;
	int16_t day = g->day;
	Phase ph = g->phase;
	Loc dest = {(uint8_t)city, 0};
	int r;

	if (city < 0)
		return;
	if (game_flight_cost(g, city) > g->cash) {
		toast(u, "You can't afford the ticket.");
		play(u, SFX_DENY);
		return;
	}
	r = game_move(g, dest);
	if (r != OK) {
		toast(u, err_text(r));
		play(u, SFX_DENY);
		return;
	}
	play(u, SFX_OK);
	ui_after_action(u, day, ph);
}

void fly_press(Ui *u, int pad)
{
	if (pad == PAD_UP)
		u->row = (u->row + NUM_CITIES - 2) % (NUM_CITIES - 1);
	else if (pad == PAD_DOWN)
		u->row = (u->row + 1) % (NUM_CITIES - 1);
	else if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		fly(u, fly_city(&u->app->g, u->row));
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void fly_click(Ui *u, int action, int arg)
{
	if (action != HIT_FLY_ROW)
		return;
	u->row = arg;
	ui_press(u, PAD_A);
}
