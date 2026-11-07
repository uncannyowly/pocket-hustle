/* The amount picker, an overlay: the mockup's drawPicker, inPicker and PK table. This is the
 * quantity half (PK_BUY, PK_SELL, PK_HEAL): UP/DOWN step 1, LEFT/RIGHT step 10, Y is the
 * most you can, X is zero, A confirms through the core and B backs out. The money kinds'
 * digit dial comes with the bank (Task 8). */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

enum { BOX_X = 36, BOX_Y = 48, BOX_W = 248, BOX_H = 144,
       INFO_MAX = 30 };		/* 30 cells fit inside the box with a margin */

static int good_ok(int good)
{
	return good >= 0 && good < NUM_GOODS;
}

void picker_open(Ui *u, PickKind kind, int good, Screen back)
{
	u->pk.kind = kind;
	u->pk.good = good;
	u->pk.q = 0;
	u->pk.sel = 0;
	overlay(u, SCR_PICKER, back);
	play(u, SFX_OK);
}

static void picker_close(Ui *u)
{
	go(u, u->back);
}

int64_t picker_max(const Ui *u)
{
	const Game *g = &u->app->g;
	int good = u->pk.good;
	int64_t m;

	switch (u->pk.kind) {
	case PK_BUY:
		return game_max_buy(g, good);
	case PK_SELL:
		return good_ok(good) && g->stash[good] > 0 ? g->stash[good] : 0;
	case PK_HEAL:
		m = 100 - g->health;
		if (g->p.hospital_per_hp > 0 && g->cash / g->p.hospital_per_hp < m)
			m = g->cash / g->p.hospital_per_hp;
		return m > 0 ? m : 0;
	default:
		return 0;
	}
}

static void upper(char *dst, size_t n, const char *src)
{
	size_t i;

	for (i = 0; src[i] && i + 1 < n; i++)
		dst[i] = (char)toupper((unsigned char)src[i]);
	dst[i] = '\0';
}

static void buy_text(const Game *g, int good, int64_t q, PickText *t)
{
	int64_t price = g->price[good], room = (int64_t)g->capacity - game_carried(g);
	int64_t afford = price > 0 && g->cash > 0 ? g->cash / price : 0;
	char name[16], a[28];

	if (room < 0)
		room = 0;
	upper(name, sizeof name, GOODS[good].name);
	snprintf(t->title, sizeof t->title, "BUY %s", name);
	fmt_money_short(price, a, sizeof a);
	snprintf(t->unit, sizeof t->unit, "%s EACH", a);
	fmt_money(q * price, a, sizeof a);
	snprintf(t->total, sizeof t->total, "TOTAL %s", a);
	snprintf(t->info, sizeof t->info, "Room for %lld. Cash for %lld.", (long long)room, (long long)afford);
	if (strlen(t->info) > INFO_MAX)
		snprintf(t->info, sizeof t->info, "Room %lld. Cash for %lld.", (long long)room, (long long)afford);
}

static void sell_text(const Game *g, int good, int64_t q, PickText *t)
{
	int32_t have = g->stash[good];
	int64_t paid = g->stash_paid[good], each = 0;
	char name[16], a[28];

	if (have > 0) {			/* stash_paid is the total: show the average, rounded */
		each = paid / have;
		if ((paid % have) * 2 >= have)
			each++;
	}
	upper(name, sizeof name, GOODS[good].name);
	snprintf(t->title, sizeof t->title, "SELL %s", name);
	fmt_money_short(g->price[good], a, sizeof a);
	snprintf(t->unit, sizeof t->unit, "%s EACH", a);
	fmt_money(q * g->price[good], a, sizeof a);
	snprintf(t->total, sizeof t->total, "YOU GET %s", a);
	fmt_money(each, a, sizeof a);
	snprintf(t->info, sizeof t->info, "You have %d. Paid %s each.", (int)have, a);
	if (strlen(t->info) > INFO_MAX)
		snprintf(t->info, sizeof t->info, "Have %d. Paid %s each.", (int)have, a);
}

static void heal_text(const Game *g, int64_t q, PickText *t)
{
	char a[28];

	snprintf(t->title, sizeof t->title, "THE DOCTOR");
	fmt_money_short(g->p.hospital_per_hp, a, sizeof a);
	snprintf(t->unit, sizeof t->unit, "%s A POINT", a);
	fmt_money(q * g->p.hospital_per_hp, a, sizeof a);
	snprintf(t->total, sizeof t->total, "COST %s", a);
	snprintf(t->info, sizeof t->info, "Health %d > %lld.", (int)g->health, (long long)(g->health + q));
}

void picker_text(const Ui *u, PickText *t)
{
	const Game *g = &u->app->g;

	memset(t, 0, sizeof *t);
	t->total_col = PAL_GOLD;
	if (u->pk.kind == PK_BUY && good_ok(u->pk.good))
		buy_text(g, u->pk.good, u->pk.q, t);
	else if (u->pk.kind == PK_SELL && good_ok(u->pk.good))
		sell_text(g, u->pk.good, u->pk.q, t);
	else if (u->pk.kind == PK_HEAL)
		heal_text(g, u->pk.q, t);
}

/* The number with its four arrows, and their click regions. */
static void draw_quantity(Ui *u, Canvas *c)
{
	int ny = BOX_Y + 30, cx = BOX_X + (BOX_W >> 1), len, sc;
	char num[24];

	snprintf(num, sizeof num, "%lld", (long long)u->pk.q);
	len = (int)strlen(num);
	sc = len <= 6 ? 3 : len <= 9 ? 2 : 1;	/* long numbers shrink to stay between the arrows */
	tx(c, num, cx, ny + (3 - sc) * 4, PAL_WHITE, sc, TX_CENTER);
	cv_tri(c, cx - 6, ny - 4, cx + 6, ny - 4, cx, ny - 10, PAL_GOLD);
	cv_tri(c, cx - 6, ny + 28, cx + 6, ny + 28, cx, ny + 34, PAL_GOLD);
	cv_tri(c, BOX_X + 30, ny + 12, BOX_X + 38, ny + 6, BOX_X + 38, ny + 18, PAL_GOLD);
	cv_tri(c, BOX_X + BOX_W - 30, ny + 12, BOX_X + BOX_W - 38, ny + 6, BOX_X + BOX_W - 38, ny + 18, PAL_GOLD);
	tx(c, "-10", BOX_X + 34, ny + 22, PAL_DIM, 1, TX_CENTER);
	tx(c, "+10", BOX_X + BOX_W - 34, ny + 22, PAL_DIM, 1, TX_CENTER);
	hit(u, BOX_X + 70, ny - 12, BOX_W - 140, 24, HIT_PAD, PAD_UP);
	hit(u, BOX_X + 70, ny + 12, BOX_W - 140, 24, HIT_PAD, PAD_DOWN);
	hit(u, BOX_X + 16, ny - 4, 40, 34, HIT_PAD, PAD_LEFT);
	hit(u, BOX_X + BOX_W - 56, ny - 4, 40, 34, HIT_PAD, PAD_RIGHT);
	tx(c, "Up/Down 1   Left/Right 10", cx, BOX_Y + 108, PAL_DIM, 1, TX_CENTER);
}

/* Y Max, X Zero, A OK, B Back, centered along the bottom; each one clickable. */
static void draw_keys(Ui *u, Canvas *c)
{
	static const Hint keys[4] = {{"Y", "Max"}, {"X", "Zero"}, {"A", "OK"}, {"B", "Back"}};
	static const int pads[4] = {PAD_Y, PAD_X, PAD_A, PAD_B};
	int i, w = 30, x, y = BOX_Y + 124;

	for (i = 0; i < 4; i++)
		w += 11 + 3 + tx_width(keys[i].label, 1);
	x = BOX_X + ((BOX_W - w) >> 1);
	for (i = 0; i < 4; i++) {
		int x0 = x;

		x += badge(c, keys[i].b, x, y) + 3;
		x += tx(c, keys[i].label, x, y, PAL_TEXT, 1, TX_LEFT) + 10;
		hit(u, x0, BOX_Y + 120, x - x0 - 6, 14, HIT_PAD, pads[i]);
	}
}

void picker_draw(Ui *u, Canvas *c)
{
	int64_t max = picker_max(u);
	PickText t;

	if (u->pk.q > max)
		u->pk.q = max;
	if (u->pk.q < 0)
		u->pk.q = 0;
	picker_text(u, &t);
	cv_rect(c, BOX_X, BOX_Y, BOX_W, BOX_H, PAL_PANEL);
	cv_box(c, BOX_X, BOX_Y, BOX_W, BOX_H, PAL_LINE_HI);
	cv_rect(c, BOX_X + 1, BOX_Y + 1, BOX_W - 2, 16, PAL_PANEL2);
	tx(c, t.title, BOX_X + 6, BOX_Y + 5, PAL_GOLD, 1, TX_LEFT);
	if (t.unit[0])
		tx(c, t.unit, BOX_X + BOX_W - 6, BOX_Y + 5, PAL_DIM, 1, TX_RIGHT);
	draw_quantity(u, c);
	tx(c, t.total, BOX_X + (BOX_W >> 1), BOX_Y + 70, t.total_col, 1, TX_CENTER);
	tx(c, t.info, BOX_X + (BOX_W >> 1), BOX_Y + 82, PAL_DIM, 1, TX_CENTER);
	cv_rect(c, BOX_X + 24, BOX_Y + 96, BOX_W - 48, 4, PAL_FAINT);
	if (max > 0) {
		int64_t q = u->pk.q, m = max;

		while (m > ((int64_t)1 << 50)) {	/* keeps the product below in range */
			m >>= 1;
			q >>= 1;
		}
		/* Math.round((w - 48) * q / max) */
		cv_rect(c, BOX_X + 24, BOX_Y + 96, (int)((2 * (BOX_W - 48) * q + m) / (2 * m)), 4, PAL_GOLD);
	}
	draw_keys(u, c);
}

/* The PK table's ok: through the core, then a toast that says what happened. */
static void confirm(Ui *u)
{
	Game *g = &u->app->g;
	int good = u->pk.good, r = E_BAD_ARG;
	int32_t q = (int32_t)u->pk.q;		/* at most the max, which fits */
	int64_t cash = g->cash;
	char msg[96], amt[28];

	msg[0] = '\0';
	if (u->pk.kind == PK_BUY) {
		r = game_buy(g, good, q);
		if (r == OK) {
			fmt_money(cash - g->cash, amt, sizeof amt);
			snprintf(msg, sizeof msg, "Bought %d %s for %s.", (int)q, GOODS[good].name, amt);
		}
	} else if (u->pk.kind == PK_SELL) {
		r = game_sell(g, good, q);
		if (r == OK) {
			fmt_money(g->cash - cash, amt, sizeof amt);
			snprintf(msg, sizeof msg, "Sold %d %s for %s.", (int)q, GOODS[good].name, amt);
		}
	} else if (u->pk.kind == PK_HEAL) {
		r = game_heal(g, q);
		if (r == OK)
			snprintf(msg, sizeof msg, "The doctor patches you up. (+%d health)", (int)q);
	}
	picker_close(u);
	if (r != OK) {
		toast(u, err_text(r));
		play(u, SFX_DENY);
		return;
	}
	toast(u, msg);
	play(u, u->pk.kind == PK_HEAL ? SFX_OK : SFX_CASH);
}

void picker_press(Ui *u, int pad)
{
	int64_t max = picker_max(u), q = u->pk.q;

	if (q > max)
		q = max;
	if (q < 0)
		q = 0;
	u->pk.q = q;
	if (pad == PAD_B) {
		picker_close(u);
		play(u, SFX_BACK);
		return;
	}
	if (pad == PAD_A) {
		if (q > 0) {
			confirm(u);
		} else {
			picker_close(u);
			play(u, SFX_BACK);
		}
		return;
	}
	if (pad == PAD_Y)
		q = max;
	else if (pad == PAD_X)
		q = 0;
	else if (pad == PAD_UP)
		q = q + 1 < max ? q + 1 : max;
	else if (pad == PAD_DOWN)
		q = q > 1 ? q - 1 : 0;
	else if (pad == PAD_RIGHT)
		q = q + 10 < max ? q + 10 : max;
	else if (pad == PAD_LEFT)
		q = q > 10 ? q - 10 : 0;
	else
		return;
	u->pk.q = q;
	play(u, SFX_MOVE);
}
