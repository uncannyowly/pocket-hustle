/* The shop: the mockup's drawStore and inStore. Weapons (AIM) in the top section, room items
 * (ROOM) below, a blurb box at the bottom. Gear sold in one city only shows that city's name
 * in faint where the price would be. Buying goes through game_buy_gear. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

static const Hint STORE_HINTS[3] = {{"A", "Buy"}, {"B", "Back"}, {"START", "Menu"}};

static int row_y(int i)
{
	return i < 8 ? 27 + i * 10 : 121 + (i - 8) * 10;
}

static int sold_here(const Game *g, int row)
{
	return GEAR[row].city < 0 || GEAR[row].city == (int)g->loc.city;
}

static int owned(const Game *g, int row)
{
	return GEAR[row].kind == GEAR_WEAPON ? g->weapon == row : g->gear_owned[row] != 0;
}

void store_text(const Game *g, int row, StoreText *t)
{
	const GearDef *d = &GEAR[row];
	int here = sold_here(g, row), i;

	memset(t, 0, sizeof *t);
	if (here)
		fmt_money(d->price, t->price, sizeof t->price);
	else
		for (i = 0; CITIES[d->city].name[i] && i + 1 < (int)sizeof t->price; i++)
			t->price[i] = (char)toupper((unsigned char)CITIES[d->city].name[i]);
	t->price_col = !here ? PAL_FAINT : d->price > g->cash ? PAL_CORAL : PAL_TEXT;
	if (d->kind == GEAR_WEAPON)
		snprintf(t->aim, sizeof t->aim, "%d%%", d->power / 10);
	else
		snprintf(t->aim, sizeof t->aim, "+%d", d->power);
	/* Each line fits the 37 cells of the blurb box. */
	if (!here) {
		snprintf(t->status, sizeof t->status, "Only sold in %s.", CITIES[d->city].name);
		t->status_col = PAL_CORAL;
	} else if (d->kind == GEAR_WEAPON) {
		if (g->weapon == row) {
			snprintf(t->status, sizeof t->status, "You're packing it now.");
			t->status_col = PAL_GOLD;
		} else {
			snprintf(t->status, sizeof t->status, "Aim in a chase: %d%%. Replaces yours.", d->power / 10);
			t->status_col = PAL_DIM;
		}
	} else if (g->gear_owned[row]) {
		snprintf(t->status, sizeof t->status, "You have one. It adds room for %d.", d->power);
		t->status_col = PAL_MINT;
	} else {
		snprintf(t->status, sizeof t->status, "Adds room for %d. Room items stack.", d->power);
		t->status_col = PAL_DIM;
	}
}

void store_open(Ui *u)
{
	u->row = 0;
	go(u, SCR_STORE);
}

void store_draw(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	char s[32], lines[2][TX_LINE_MAX];
	StoreText t;
	int i, n;

	if (u->row < 0 || u->row >= NUM_GEAR)
		u->row = 0;
	fmt_money_short(g->cash, s, sizeof s);
	top_bar(u, c, "SHOP", s);
	cv_rect(c, 0, 12, CV_W, 212, PAL_PANEL);
	tx(c, "WEAPONS", 8, 16, PAL_DIM, 1, TX_LEFT);
	tx(c, "AIM", 284, 16, PAL_DIM, 1, TX_RIGHT);
	tx(c, "MORE ROOM", 8, 110, PAL_DIM, 1, TX_LEFT);
	tx(c, "ROOM", 284, 110, PAL_DIM, 1, TX_RIGHT);
	for (i = 0; i < NUM_GEAR; i++) {
		int y = row_y(i), sel = u->row == i, here = sold_here(g, i);

		store_text(g, i, &t);
		if (sel)
			cv_rect(c, 4, y - 1, 312, 10, PAL_HOT);
		tx(c, GEAR[i].name, 10, y, sel ? PAL_WHITE : here ? PAL_TEXT : PAL_FAINT, 1, TX_LEFT);
		tx(c, t.price, 240, y, sel ? PAL_WHITE : t.price_col, 1, TX_RIGHT);
		tx(c, t.aim, 284, y, sel ? PAL_WHITE : here ? PAL_MINT : PAL_FAINT, 1, TX_RIGHT);
		if (owned(g, i))
			art_icon(c, GEAR[i].kind == GEAR_WEAPON ? ICON_STAR : ICON_CHECK, 298, y + 1, 1, 0);
		hit(u, 4, y - 1, 312, 10, HIT_STORE_ROW, i);
	}
	store_text(g, u->row, &t);
	cv_rect(c, 4, 164, 312, 58, PAL_NIGHT);
	cv_box(c, 4, 164, 312, 58, PAL_LINE);
	for (i = 0; GEAR[u->row].name[i] && i + 1 < (int)sizeof s; i++)
		s[i] = (char)toupper((unsigned char)GEAR[u->row].name[i]);
	s[i] = '\0';
	tx(c, s, 10, 169, PAL_GOLD, 1, TX_LEFT);
	n = tx_wrap(GEAR[u->row].blurb, 37, lines, 2);
	for (i = 0; i < n; i++)
		tx(c, lines[i], 10, 181 + i * 10, PAL_TEXT, 1, TX_LEFT);
	tx(c, t.status, 10, 206, t.status_col, 1, TX_LEFT);
	hints(c, STORE_HINTS, 3, HINTS_Y);
}

static void buy(Ui *u)
{
	const Game *g = &u->app->g;
	int row = u->row, r;
	char msg[64];

	if (!sold_here(g, row)) {
		snprintf(msg, sizeof msg, "Only sold in %s.", CITIES[GEAR[row].city].name);
		toast(u, msg);
		play(u, SFX_DENY);
		return;
	}
	if (GEAR[row].kind == GEAR_WEAPON && g->weapon == row) {
		toast(u, "That's what you're packing.");
		play(u, SFX_DENY);
		return;
	}
	if (GEAR[row].kind == GEAR_SPACE && g->gear_owned[row]) {
		toast(u, "You already have one.");
		play(u, SFX_DENY);
		return;
	}
	r = game_buy_gear(&u->app->g, row);
	if (r != OK) {
		toast(u, err_text(r));
		play(u, SFX_DENY);
		return;
	}
	snprintf(msg, sizeof msg, "You bought the %s.", GEAR[row].name);
	toast(u, msg);
	play(u, SFX_CASH);
}

void store_press(Ui *u, int pad)
{
	if (u->row < 0 || u->row >= NUM_GEAR)
		u->row = 0;
	if (pad == PAD_UP)
		u->row = (u->row + NUM_GEAR - 1) % NUM_GEAR;
	else if (pad == PAD_DOWN)
		u->row = (u->row + 1) % NUM_GEAR;
	else if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		buy(u);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void store_click(Ui *u, int action, int arg)
{
	if (action != HIT_STORE_ROW)
		return;
	u->row = arg;
	ui_press(u, PAD_A);
}
