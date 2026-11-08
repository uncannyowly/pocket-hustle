/* The bank and the loan shark: the mockup's drawBank/inBank and drawLender/inLender. Each
 * has two buttons that open the money dial (scr_picker.c). The fee, the bank's interest, the
 * clock and the minimum payment all come from the game's params. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

static const Hint MONEY_HINTS[3] = {{"A", "Choose"}, {"B", "Back"}, {"START", "Menu"}};

static int city_of(const Game *g)
{
	return g->loc.city < NUM_CITIES ? g->loc.city : 0;
}

/* basis points as the mockup's (bp / 100) + "%": 950 is 9.5%, 700 is 7%, 125 is 1.25% */
void rate_text(int32_t bp, char *buf, size_t n)
{
	int whole = bp / 100, frac = bp % 100;

	if (frac == 0)
		snprintf(buf, n, "%d%%", whole);
	else if (frac % 10 == 0)
		snprintf(buf, n, "%d.%d%%", whole, frac / 10);
	else
		snprintf(buf, n, "%d.%02d%%", whole, frac);
}

void bank_text(const Game *g, BankText *t)
{
	char rate[16], min[28];

	memset(t, 0, sizeof *t);
	if (game_bank_branch_here(g)) {
		rate_text(g->p.bank_rate_bp, rate, sizeof rate);
		snprintf(t->head, sizeof t->head, "YOUR BRANCH");
		snprintf(t->tag, sizeof t->tag, "NO FEES");
		snprintf(t->blurb, sizeof t->blurb, "Safe from muggers. Pays %s a day.", rate);
		t->tag_col = PAL_MINT;
		return;
	}
	rate_text(g->p.bank_fee_pct * 100, rate, sizeof rate);
	fmt_money(g->p.bank_fee_min, min, sizeof min);
	snprintf(t->head, sizeof t->head, "ATM");
	snprintf(t->tag, sizeof t->tag, "FEE %s, MIN %s", rate, min);
	if (!CITIES[city_of(g)].bank_hoods)
		snprintf(t->blurb, sizeof t->blurb, "No branches across the border. Every ATM here charges the fee.");
	else
		snprintf(t->blurb, sizeof t->blurb, "No branch here. The bank symbol on the map marks free banking.");
	t->tag_col = PAL_CORAL;
}

void lender_text(const Game *g, LenderText *t)
{
	int c = city_of(g), age = g->debt_age[c], left, i;
	int64_t d = g->debt[c];
	const CityDef *L = &CITIES[c];
	char a[28], pct[16];

	memset(t, 0, sizeof *t);
	for (i = 0; L->lender[i] && i + 1 < (int)sizeof t->name; i++)
		t->name[i] = (char)toupper((unsigned char)L->lender[i]);
	rate_text(L->lender_rate_bp, pct, sizeof pct);
	snprintf(t->rate, sizeof t->rate, "%s A DAY", pct);
	fmt_money(L->lender_max, a, sizeof a);
	snprintf(t->limit, sizeof t->limit, "LIMIT %s", a);
	fmt_money(d, t->owed, sizeof t->owed);
	t->overdue = d > 0 && age > g->p.enforce_days;
	if (d <= 0) {
		snprintf(t->warn, sizeof t->warn, "No loan in %s. Each city has its own lender.", L->name);
		return;
	}
	snprintf(t->paid, sizeof t->paid, "LAST PAID %d %s AGO", age, age == 1 ? "DAY" : "DAYS");
	left = g->p.enforce_days + 1 - age;
	if (t->overdue)
		snprintf(t->warn, sizeof t->warn, "%s's crew is out looking for you.", L->lender);
	else
		snprintf(t->warn, sizeof t->warn, "Pay %d%% within %d %s or %s's crew comes looking.",
			 (int)g->p.min_payment_pct, left, left == 1 ? "day" : "days", L->lender);
}

/* ---- the bank ---- */

void bank_open(Ui *u)
{
	u->row = 0;
	go(u, SCR_BANK);
}

void bank_draw(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	int city = city_of(g), hood = g->loc.hood < HOODS_PER_CITY ? g->loc.hood : 0, i, n;
	char lines[4][TX_LINE_MAX], s[32];
	BankText t;

	bank_text(g, &t);
	top_bar(u, c, NULL, NULL);
	art_backdrop(c, city, 12, 56, u->frame);
	place_label(c, city, CITIES[city].hoods[hood], 68);
	cv_rect(c, 0, 68, CV_W, 156, PAL_PANEL);
	cv_rect(c, 0, 68, CV_W, 1, PAL_LINE);
	art_vault(c, 56, 144, u->frame);
	tx(c, t.head, 112, 76, PAL_GOLD, 1, TX_LEFT);
	tx(c, t.tag, 312, 76, t.tag_col, 1, TX_RIGHT);
	n = tx_wrap(t.blurb, 25, lines, 4);
	for (i = 0; i < n; i++)
		tx(c, lines[i], 112, 90 + i * 10, PAL_DIM, 1, TX_LEFT);
	tx(c, "IN THE BANK", 112, 122, PAL_DIM, 1, TX_LEFT);
	fmt_money_short(g->bank, s, sizeof s);
	tx(c, s, 112, 132, PAL_MINT, 2, TX_LEFT);
	tx(c, "CASH", 112, 156, PAL_DIM, 1, TX_LEFT);
	fmt_money_short(g->cash, s, sizeof s);
	tx(c, s, 152, 156, PAL_GOLD, 1, TX_LEFT);
	button(u, c, 112, 176, 96, 18, "DEPOSIT", u->row == 0, HIT_MONEY_BTN, 0);
	button(u, c, 214, 176, 98, 18, "WITHDRAW", u->row == 1, HIT_MONEY_BTN, 1);
	hints(c, MONEY_HINTS, 3, HINTS_Y);
}

void bank_press(Ui *u, int pad)
{
	if (pad == PAD_LEFT || pad == PAD_RIGHT || pad == PAD_UP || pad == PAD_DOWN)
		u->row ^= 1;
	else if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		picker_open(u, u->row ? PK_WITHDRAW : PK_DEPOSIT, -1, SCR_BANK);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void bank_click(Ui *u, int action, int arg)
{
	if (action != HIT_MONEY_BTN)
		return;
	u->row = arg ? 1 : 0;
	ui_press(u, PAD_A);
}

/* ---- the loan shark ---- */

void lender_open(Ui *u)
{
	u->row = 0;
	go(u, SCR_LENDER);
}

void lender_draw(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	int city = city_of(g), hood = g->loc.hood < HOODS_PER_CITY ? g->loc.hood : 0, i, n;
	char lines[4][TX_LINE_MAX];
	LenderText t;

	lender_text(g, &t);
	top_bar(u, c, NULL, NULL);
	art_backdrop(c, city, 12, 56, u->frame);
	place_label(c, city, CITIES[city].hoods[hood], 68);
	cv_rect(c, 0, 68, CV_W, 156, PAL_PANEL);
	cv_rect(c, 0, 68, CV_W, 1, PAL_LINE);
	cv_rect(c, 8, 76, 60, 60, PAL_NIGHT);
	cv_box(c, 8, 76, 60, 60, PAL_LINE);
	art_portrait(c, city, 10, 78);
	tx(c, t.name, 76, 76, PAL_GOLD, 1, TX_LEFT);
	tx(c, t.rate, 76, 90, PAL_TEXT, 1, TX_LEFT);
	tx(c, t.limit, 76, 102, PAL_TEXT, 1, TX_LEFT);
	tx(c, "YOU OWE", 76, 116, PAL_DIM, 1, TX_LEFT);
	tx(c, t.owed, 140, 116, g->debt[city] > 0 ? PAL_CORAL : PAL_DIM, 1, TX_LEFT);
	if (t.paid[0])
		tx(c, t.paid, 76, 128, t.overdue ? PAL_CORAL : PAL_DIM, 1, TX_LEFT);
	n = tx_wrap(t.warn, 37, lines, 4);
	for (i = 0; i < n; i++)
		tx(c, lines[i], 8, 146 + i * 11, PAL_TEXT, 1, TX_LEFT);
	button(u, c, 8, 192, 150, 18, "BORROW", u->row == 0, HIT_MONEY_BTN, 0);
	button(u, c, 162, 192, 150, 18, "REPAY", u->row == 1, HIT_MONEY_BTN, 1);
	hints(c, MONEY_HINTS, 3, HINTS_Y);
}

void lender_press(Ui *u, int pad)
{
	if (pad == PAD_LEFT || pad == PAD_RIGHT || pad == PAD_UP || pad == PAD_DOWN)
		u->row ^= 1;
	else if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else if (pad == PAD_A) {
		if (u->row == 1 && u->app->g.debt[city_of(&u->app->g)] <= 0) {
			toast(u, "You owe nothing here.");
			play(u, SFX_DENY);
			return;
		}
		picker_open(u, u->row ? PK_REPAY : PK_BORROW, -1, SCR_LENDER);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}

void lender_click(Ui *u, int action, int arg)
{
	if (action != HIT_MONEY_BTN)
		return;
	u->row = arg ? 1 : 0;
	ui_press(u, PAD_A);
}
