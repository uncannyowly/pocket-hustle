/* The price chart: the mockup's drawChart, inChart and niceTop. One good's prices day by day
 * over the whole game, with the good's usual range as a dotted band. A point the market roll
 * made a spike (the core's hist_ev) is pink, a crash green. LEFT/RIGHT (or L1/R1) page through the goods; B or SELECT
 * returns to the market. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

enum { X0 = 56, X1 = 310, Y0 = 24, Y1 = 172 };

static const Hint CHART_HINTS[2] = {{"<>", "Other good"}, {"B", "Back"}};

void chart_k(int64_t v, char *buf, size_t n)
{
	int64_t unit = v >= 1000000 ? 1000000 : v >= 1000 ? 1000 : 0, tenths;
	char suffix = unit == 1000000 ? 'M' : 'K';

	if (!unit) {
		snprintf(buf, n, "$%lld", (long long)v);
		return;
	}
	tenths = (v * 10 + unit / 2) / unit;		/* to one decimal, half up */
	if (tenths >= 10000 && unit == 1000) {		/* 999,950 is $1M, not $1000K */
		unit = 1000000;
		suffix = 'M';
		tenths = (v * 10 + unit / 2) / unit;
	}
	if (tenths % 10)
		snprintf(buf, n, "$%lld.%lld%c", (long long)(tenths / 10), (long long)(tenths % 10), suffix);
	else
		snprintf(buf, n, "$%lld%c", (long long)(tenths / 10), suffix);
}

int64_t chart_top(int64_t v)
{
	static const int M10[] = {10, 15, 20, 25, 30, 40, 50, 60, 80, 100};
	int64_t e = 1;
	size_t i;

	if (v < 1)
		v = 1;
	while (e * 10 <= v)
		e *= 10;
	for (i = 0; i < sizeof M10 / sizeof M10[0]; i++)
		if (e * M10[i] >= v * 10)
			return e * M10[i] / 10;
	return e * 10;
}

void chart_text(const Game *g, int good, ChartText *t)
{
	char a[24], b[24], paid[32];
	int64_t each;
	int i;

	memset(t, 0, sizeof *t);
	for (i = 0; GOODS[good].name[i] && i < 20; i++)
		t->title[i] = (char)toupper((unsigned char)GOODS[good].name[i]);
	snprintf(t->title + i, sizeof t->title - (size_t)i, " PRICES");
	snprintf(t->day, sizeof t->day, "DAY %d/%d", (int)g->day, (int)g->last_day);
	chart_k(GOODS[good].lo, a, sizeof a);
	chart_k(GOODS[good].hi, b, sizeof b);
	snprintf(t->range, sizeof t->range, "USUAL RANGE %s TO %s", a, b);
	if (g->stash[good] <= 0) {
		snprintf(t->have, sizeof t->have, "YOU HAVE NONE");
		return;
	}
	each = g->stash_paid[good] / g->stash[good];	/* stash_paid is the total: the rounded average */
	if ((g->stash_paid[good] % g->stash[good]) * 2 >= g->stash[good])
		each++;
	fmt_money_short(each, paid, sizeof paid);
	/* The longest line that stays clear of the CRASH key (25 cells). */
	snprintf(t->have, sizeof t->have, "YOU HAVE %d, PAID %s EACH", (int)g->stash[good], paid);
	if (strlen(t->have) > 25)
		snprintf(t->have, sizeof t->have, "HAVE %d, PAID %s EACH", (int)g->stash[good], paid);
	if (strlen(t->have) > 25)
		snprintf(t->have, sizeof t->have, "%d AT %s EACH", (int)g->stash[good], paid);
}

void chart_open(Ui *u, int good)
{
	u->chart_good = good;
	go(u, SCR_CHART);
	play(u, SFX_OK);
}

/* x of day d: the plot is x0..x1 over days 1..n, rounded */
static int sx(int d, int days)
{
	int64_t span = days > 1 ? days - 1 : 1;

	return X0 + (int)(((int64_t)(d - 1) * (X1 - X0) * 2 + span) / (2 * span));
}

static int sy(int64_t p, int64_t top)
{
	return Y1 - (int)((p * (Y1 - Y0) * 2 + top) / (2 * top));
}

void chart_draw(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	static const int STEPS[4][4] = {{1, 5, 10, 15}, {1, 10, 20, 30}, {1, 20, 40, 60}, {1, 30, 60, 90}};
	int days = g->last_day > 1 ? g->last_day : 1, good = u->chart_good, last_d = 0, ticks[4], nt, i, xx, yy;
	int lx0 = 0, ly0 = 0;
	int64_t lo, hi, peak, top, v, last_p = 0;
	int today = g->day < MAX_DAYS ? g->day : MAX_DAYS, np = 0;
	char s[32];
	ChartText t;

	if (good < 0 || good >= NUM_GOODS)
		good = u->chart_good = 0;
	lo = GOODS[good].lo;
	hi = GOODS[good].hi;
	chart_text(g, good, &t);
	top_bar(u, c, t.title, t.day);
	cv_rect(c, 0, 12, CV_W, 212, PAL_PANEL);
	peak = hi;
	for (i = 1; i <= today; i++)
		if (g->hist[good][i] > peak)
			peak = g->hist[good][i];
	top = chart_top(peak);
	for (yy = sy(hi, top); yy <= sy(lo, top); yy++)		/* the usual range, dotted */
		for (xx = X0; xx <= X1; xx++)
			if (((xx + yy) & 1) == 0)
				cv_px(c, xx, yy, PAL_PANEL2);
	for (i = 0; i < 3; i++) {				/* 0, half, top */
		v = i == 0 ? 0 : i == 1 ? top / 2 : top;
		yy = sy(v, top);
		cv_rect(c, X0, yy, X1 - X0 + 1, 1, v ? PAL_PANEL2 : PAL_LINE);
		chart_k(v, s, sizeof s);
		tx(c, s, X0 - 4, yy - 3, PAL_DIM, 1, TX_RIGHT);
	}
	nt = 4;
	for (i = 0; i < 4; i++)
		ticks[i] = STEPS[days == 15 ? 0 : days == 30 ? 1 : days == 60 ? 2 : 3][i];
	if (days != 15 && days != 30 && days != 60 && days != 90) {
		ticks[1] = days;
		nt = 2;
	}
	for (i = 0; i < nt; i++) {
		xx = sx(ticks[i], days);
		cv_rect(c, xx, Y1 + 1, 1, 3, PAL_LINE);
		snprintf(s, sizeof s, "%d", ticks[i]);
		tx(c, s, xx, Y1 + 6, PAL_DIM, 1, TX_CENTER);
	}
	for (i = 1; i <= today; i++) {				/* the line, then the points over it */
		int p = g->hist[good][i];

		if (p <= 0)
			continue;
		if (np)
			cv_line(c, lx0, ly0, sx(i, days), sy(p, top), PAL_LINE_HI);
		lx0 = sx(i, days);
		ly0 = sy(p, top);
		np++;
	}
	for (i = 1; i <= today; i++) {
		int p = g->hist[good][i], ev;

		if (p <= 0)
			continue;
		ev = g->hist_ev[good][i];
		cv_rect(c, sx(i, days) - 1, sy(p, top) - 1, 3, 3,
			ev == EV_SPIKE ? PAL_HOT : ev == EV_CRASH ? PAL_MINT : PAL_TEXT);
		last_d = i;
		last_p = p;
	}
	if (np && last_d == g->day) {				/* today, ringed and labeled */
		int lx = sx(last_d, days), ly = sy(last_p, top);
		int right = (lx < X1 ? lx : X1) - 6, ty = ly - 12 > Y0 ? ly - 12 : Y0;

		cv_ring(c, lx, ly, 4, PAL_GOLD);
		fmt_money(last_p, s, sizeof s);
		if (right < (int)strlen(s) * GLYPH_W)		/* near the left edge (day 1): put it right of the point */
			tx(c, s, lx + 6, ty, PAL_GOLD, 1, TX_LEFT);
		else
			tx(c, s, right, ty, PAL_GOLD, 1, TX_RIGHT);
	}
	if (!np)
		tx(c, "No prices seen yet.", 183, 90, PAL_DIM, 1, TX_CENTER);
	for (yy = 0; yy < 7; yy++)				/* the legend */
		for (xx = 0; xx < 9; xx++)
			if (((xx + yy) & 1) == 0)
				cv_px(c, 8 + xx, 191 + yy, PAL_DIM);
	tx(c, t.range, 22, 191, PAL_DIM, 1, TX_LEFT);
	cv_rect(c, 8, 203, 3, 3, PAL_HOT);
	tx(c, "SPIKE", 14, 201, PAL_TEXT, 1, TX_LEFT);
	cv_rect(c, 64, 203, 3, 3, PAL_MINT);
	tx(c, "CRASH", 70, 201, PAL_TEXT, 1, TX_LEFT);
	tx(c, t.have, 312, 201, PAL_DIM, 1, TX_RIGHT);
	hints(c, CHART_HINTS, 2, HINTS_Y);
	hit(u, 0, 12, CV_W / 2, 212, HIT_PAD, PAD_LEFT);
	hit(u, CV_W / 2, 12, CV_W / 2, 212, HIT_PAD, PAD_RIGHT);
}

void chart_press(Ui *u, int pad)
{
	if (u->chart_good < 0 || u->chart_good >= NUM_GOODS)
		u->chart_good = 0;
	if (pad == PAD_LEFT || pad == PAD_L1)
		u->chart_good = (u->chart_good + NUM_GOODS - 1) % NUM_GOODS;
	else if (pad == PAD_RIGHT || pad == PAD_R1)
		u->chart_good = (u->chart_good + 1) % NUM_GOODS;
	else if (pad == PAD_B || pad == PAD_SELECT) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
		return;
	} else
		return;
	play(u, SFX_MOVE);
}
