/* The amount picker, an overlay: the mockup's drawPicker, inPicker and PK table. Quantities
 * (PK_BUY, PK_SELL, PK_HEAL): UP/DOWN step 1, LEFT/RIGHT step 10. Money (PK_DEPOSIT,
 * PK_WITHDRAW, PK_BORROW, PK_REPAY) uses the digit dial: LEFT/RIGHT pick a digit, UP/DOWN
 * add or remove its place value. Y is the most you can, X is zero, A confirms through the
 * core and B backs out. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

enum { BOX_X = 36, BOX_Y = 48, BOX_W = 248, BOX_H = 144,
       INFO_MAX = 30 };		/* 30 cells fit inside the box with a margin */

static int is_money(PickKind k)
{
	return k == PK_DEPOSIT || k == PK_WITHDRAW || k == PK_BORROW || k == PK_REPAY;
}

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

static int city_of(const Game *g)
{
	return g->loc.city < NUM_CITIES ? g->loc.city : 0;
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
	case PK_DEPOSIT:
		return g->cash > 0 ? g->cash : 0;
	case PK_WITHDRAW:
		return g->bank > 0 ? g->bank : 0;
	case PK_BORROW:
		m = CITIES[city_of(g)].lender_max - g->debt[city_of(g)];
		return m > 0 ? m : 0;
	case PK_REPAY:
		m = g->debt[city_of(g)];
		if (g->cash < m)
			m = g->cash;
		return m > 0 ? m : 0;
	}
	return 0;
}

/* The digits of the dial: the decimal length of the most you can pick, at least 1. */
static int dial_digits(int64_t max)
{
	int n = 1;

	while (max >= 10) {
		max /= 10;
		n++;
	}
	return n;
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

/* "BANK $200 > $1,170": the full amounts when they fit the box, else the short forms. */
static void change_line(char *out, size_t n, const char *label, int64_t from, int64_t to)
{
	char a[28], b[28];

	fmt_money(from, a, sizeof a);
	fmt_money(to, b, sizeof b);
	snprintf(out, n, "%s %s > %s", label, a, b);
	if (strlen(out) > INFO_MAX) {
		fmt_money_short(from, a, sizeof a);
		fmt_money_short(to, b, sizeof b);
		snprintf(out, n, "%s %s > %s", label, a, b);
	}
}

static void atm_unit(const Game *g, PickText *t)
{
	char pct[16];

	if (game_bank_branch_here(g)) {
		snprintf(t->unit, sizeof t->unit, "BRANCH, NO FEE");
		return;
	}
	rate_text(g->p.bank_fee_pct * 100, pct, sizeof pct);
	snprintf(t->unit, sizeof t->unit, "ATM FEE %s", pct);
}

static void atm_info(const Game *g, int64_t q, PickText *t)
{
	char pct[16], fee[28], min[28];

	if (game_bank_branch_here(g)) {
		snprintf(t->info, sizeof t->info, "Branch here. No fee.");
		return;
	}
	rate_text(g->p.bank_fee_pct * 100, pct, sizeof pct);
	fmt_money(game_bank_fee(g, q), fee, sizeof fee);
	fmt_money(g->p.bank_fee_min, min, sizeof min);
	snprintf(t->info, sizeof t->info, "Fee %s. %s, at least %s.", fee, pct, min);
	if (strlen(t->info) > INFO_MAX)
		snprintf(t->info, sizeof t->info, "Fee %s. %s, min %s.", fee, pct, min);
}

/* What lands after the ATM fee; nothing when the amount does not cover it. */
static int64_t net_of(const Game *g, int64_t q)
{
	int64_t fee = game_bank_fee(g, q);

	return q > fee ? q - fee : 0;
}

static void money_text(const Game *g, PickKind kind, int64_t q, PickText *t)
{
	int c = city_of(g);
	int64_t owe = g->debt[c], room = CITIES[c].lender_max - owe;
	char a[28], b[28], pct[16];

	if (room < 0)
		room = 0;
	switch (kind) {
	case PK_DEPOSIT:
		snprintf(t->title, sizeof t->title, "DEPOSIT");
		atm_unit(g, t);
		change_line(t->total, sizeof t->total, "BANK", g->bank, g->bank + net_of(g, q));
		atm_info(g, q, t);
		t->total_col = PAL_MINT;
		break;
	case PK_WITHDRAW:
		snprintf(t->title, sizeof t->title, "WITHDRAW");
		atm_unit(g, t);
		change_line(t->total, sizeof t->total, "CASH", g->cash, g->cash + net_of(g, q));
		atm_info(g, q, t);
		break;
	case PK_BORROW:
		snprintf(t->title, sizeof t->title, "BORROW");
		rate_text(CITIES[c].lender_rate_bp, pct, sizeof pct);
		snprintf(t->unit, sizeof t->unit, "%s A DAY", pct);
		change_line(t->total, sizeof t->total, "OWE", owe, owe + q);
		fmt_money(CITIES[c].lender_max, a, sizeof a);
		fmt_money(room, b, sizeof b);
		snprintf(t->info, sizeof t->info, "Limit %s. %s more.", a, b);
		t->total_col = PAL_CORAL;
		break;
	default:
		snprintf(t->title, sizeof t->title, "REPAY");
		change_line(t->total, sizeof t->total, "OWE", owe, owe - q);
		snprintf(t->info, sizeof t->info, "Pay %d%%+ to reset the clock.", (int)g->p.min_payment_pct);
		t->total_col = PAL_CORAL;
		break;
	}
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
	else if (is_money(u->pk.kind))
		money_text(g, u->pk.kind, u->pk.q, t);
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

/* "$3,412" at 2x with the selected digit boxed and arrowed; every digit has an upper half
 * (adds its place value) and a lower half (takes it away) to click. */
static void draw_dial(Ui *u, Canvas *c, int64_t max)
{
	int nd = dial_digits(max), sc, cw, ncell, cx0, ty = BOX_Y + 32, i;
	char s[24], ch[2] = {0, 0};

	if (u->pk.sel > nd - 1)
		u->pk.sel = nd - 1;
	if (u->pk.sel < 0)
		u->pk.sel = 0;
	snprintf(s, sizeof s, "%0*lld", nd, (long long)u->pk.q);
	sc = nd > 9 ? 1 : 2;
	cw = 8 * sc + (sc == 2 ? 2 : 1);
	ncell = 1 + nd + (nd - 1) / 3;		/* the $, the digits and the commas */
	cx0 = BOX_X + ((BOX_W - ncell * cw) >> 1);
	ch[0] = '$';
	tx(c, ch, cx0, ty, PAL_GOLD, sc, TX_LEFT);
	cx0 += cw;
	for (i = 0; i < nd; i++) {
		int d = nd - 1 - i, on = d == u->pk.sel, gw = 8 * sc, hh = (8 * sc + 24) >> 1;

		if (on) {
			cv_rect(c, cx0 - 2, ty - 3, gw + 4, 8 * sc + 6, PAL_HOT);
			cv_tri(c, cx0 + 1, ty - 6, cx0 + gw - 1, ty - 6, cx0 + (gw >> 1), ty - 11, PAL_GOLD);
			cv_tri(c, cx0 + 1, ty + 8 * sc + 5, cx0 + gw - 1, ty + 8 * sc + 5, cx0 + (gw >> 1),
			       ty + 8 * sc + 10, PAL_GOLD);
		}
		ch[0] = s[i];
		tx(c, ch, cx0, ty, on ? PAL_WHITE : PAL_TEXT, sc, TX_LEFT);
		hit(u, cx0 - 2, ty - 12, gw + 4, hh, HIT_DIAL_UP, d);
		hit(u, cx0 - 2, ty - 12 + hh, gw + 4, hh, HIT_DIAL_DOWN, d);
		cx0 += cw;
		if (d > 0 && d % 3 == 0) {
			ch[0] = ',';
			tx(c, ch, cx0, ty, PAL_DIM, sc, TX_LEFT);
			cx0 += cw;
		}
	}
	tx(c, "Left/Right picks a digit", BOX_X + (BOX_W >> 1), BOX_Y + 108, PAL_DIM, 1, TX_CENTER);
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
	if (is_money(u->pk.kind))
		draw_dial(u, c, max);
	else
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

/* The money kinds' ok, through the core. The toast says what moved and what the ATM took. */
static int money_confirm(Ui *u, char *msg, size_t n)
{
	Game *g = &u->app->g;
	int64_t q = u->pk.q, fee = game_bank_fee(g, q);
	int c = city_of(g), r;
	char amt[28], f[28];

	fmt_money(q, amt, sizeof amt);
	fmt_money(fee, f, sizeof f);
	switch (u->pk.kind) {
	case PK_DEPOSIT:
		r = game_deposit(g, q);
		if (r == OK && fee > 0)
			snprintf(msg, n, "Deposited %s, minus a %s ATM fee.", amt, f);
		else if (r == OK)
			snprintf(msg, n, "Deposited %s.", amt);
		break;
	case PK_WITHDRAW:
		r = game_withdraw(g, q);
		if (r == OK && fee > 0)
			snprintf(msg, n, "Took out %s, minus a %s ATM fee.", amt, f);
		else if (r == OK)
			snprintf(msg, n, "Took out %s.", amt);
		break;
	case PK_BORROW:
		r = game_borrow(g, q);
		if (r == OK)
			snprintf(msg, n, "%s hands you %s, smiling.", CITIES[c].lender, amt);
		break;
	default:
		r = game_repay(g, q);
		if (r == OK && g->debt[c] > 0) {
			fmt_money(g->debt[c], f, sizeof f);
			snprintf(msg, n, "Paid %s. %s to go.", amt, f);
		} else if (r == OK) {
			snprintf(msg, n, "Paid in full. %s looks almost sad.", CITIES[c].lender);
		}
		break;
	}
	return r;
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
	} else
		r = money_confirm(u, msg, sizeof msg);
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
	else if (is_money(u->pk.kind)) {
		int nd = dial_digits(max), sel = u->pk.sel > nd - 1 ? nd - 1 : u->pk.sel;
		int64_t step = 1;
		int i;

		for (i = 0; i < sel; i++)
			step *= 10;
		if (pad == PAD_UP)
			q = max - q > step ? q + step : max;
		else if (pad == PAD_DOWN)
			q = q > step ? q - step : 0;
		else if (pad == PAD_LEFT)
			sel = sel + 1 < nd ? sel + 1 : nd - 1;
		else if (pad == PAD_RIGHT)
			sel = sel > 0 ? sel - 1 : 0;
		else
			return;
		u->pk.sel = sel;
	} else if (pad == PAD_UP)
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

/* A click on a dial digit: pick it, then step it up or down like the buttons. */
void picker_click(Ui *u, int action, int arg)
{
	if (action != HIT_DIAL_UP && action != HIT_DIAL_DOWN)
		return;
	u->pk.sel = arg;
	ui_press(u, action == HIT_DIAL_UP ? PAD_UP : PAD_DOWN);
}
