/* The UI frame: the dispatch table, overlays, the shared chrome, hit regions, toasts, the
 * sound queue and the money formats. The screens themselves live in scr_*.c. */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "content/text.h"
#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

enum { TOAST_TICKS = 40, TOAST_COLS = 34, TOAST_LINES = 6, TOAST_BOTTOM = 194 };

typedef struct {
	void (*draw)(Ui *u, Canvas *c);
	void (*press)(Ui *u, int pad);
	void (*click)(Ui *u, int action, int arg);
	int overlay;
	const char *name;
} ScreenDef;

/* NULL draw and press: not built yet (see ui_internal.h). */
static const ScreenDef SCREENS[NUM_SCREENS] = {
	[SCR_TITLE]   = {NULL, NULL, NULL, 0, "TITLE"},
	[SCR_NEWGAME] = {NULL, NULL, NULL, 0, "NEW GAME"},
	[SCR_MARKET]  = {market_draw, market_press, market_click, 0, "MARKET"},
	[SCR_PICKER]  = {picker_draw, picker_press, NULL, 1, "PICKER"},
	[SCR_NEWS]    = {NULL, NULL, NULL, 1, "NEWS"},
	[SCR_MOVE]    = {NULL, NULL, NULL, 0, "GET AROUND"},
	[SCR_FLY]     = {NULL, NULL, NULL, 0, "FLY"},
	[SCR_BANK]    = {NULL, NULL, NULL, 0, "BANK"},
	[SCR_LENDER]  = {NULL, NULL, NULL, 0, "LOAN SHARK"},
	[SCR_STORE]   = {NULL, NULL, NULL, 0, "SHOP"},
	[SCR_CHASE]   = {NULL, NULL, NULL, 0, "COP CHASE"},
	[SCR_CHART]   = {NULL, NULL, NULL, 0, "PRICE CHART"},
	[SCR_MENU]    = {NULL, NULL, NULL, 1, "PAUSED"},
	[SCR_OVER]    = {NULL, NULL, NULL, 0, "GAME OVER"},
	[SCR_SCORES]  = {NULL, NULL, NULL, 0, "HIGH SCORES"},
};

static const ScreenDef *screen_def(Screen s)
{
	return (unsigned)s < NUM_SCREENS ? &SCREENS[s] : &SCREENS[SCR_TITLE];
}

int ui_stub(Screen s)
{
	return !screen_def(s)->draw;
}

void ui_init(Ui *u, App *a)
{
	memset(u, 0, sizeof *u);
	u->app = a;
	u->scr = u->under = u->back = SCR_TITLE;
	u->mrow = 1;			/* the mockup's market starts on the second good */
}

/* ---- screens not built yet ---- */

static void stub_draw(Canvas *c, Screen s)
{
	static const Hint back[] = {{"B", "Back"}};

	cv_rect(c, 0, 0, CV_W, CV_H, PAL_NIGHT);
	tx(c, screen_def(s)->name, CV_W / 2, 100, PAL_GOLD, 2, TX_CENTER);
	tx(c, "Coming soon.", CV_W / 2, 124, PAL_DIM, 1, TX_CENTER);
	hints(c, back, 1, HINTS_Y);
}

static void stub_press(Ui *u, int pad)
{
	if (pad == PAD_B) {
		go(u, SCR_MARKET);
		play(u, SFX_BACK);
	}
}

/* ---- dispatch ---- */

static void draw_screen(Ui *u, Canvas *c, Screen s)
{
	const ScreenDef *d = screen_def(s);

	if (d->draw)
		d->draw(u, c);
	else
		stub_draw(c, s);
}

static void draw_toast(Ui *u, Canvas *c)
{
	char lines[TOAST_LINES][TX_LINE_MAX];
	int n, i, w = 0, h, x, y;

	if (!u->toast[0])
		return;
	n = tx_wrap(u->toast, TOAST_COLS, lines, TOAST_LINES);
	for (i = 0; i < n; i++)
		if ((int)strlen(lines[i]) > w)
			w = (int)strlen(lines[i]);
	w = w * GLYPH_W + 16;
	h = n * LINE_H + 8;
	x = (CV_W - w) >> 1;
	y = TOAST_BOTTOM - h;
	cv_rect(c, x, y, w, h, PAL_PANEL2);
	cv_box(c, x, y, w, h, PAL_GOLD);
	for (i = 0; i < n; i++)
		tx(c, lines[i], x + 8, y + 5 + i * LINE_H, PAL_TEXT, 1, TX_LEFT);
}

void ui_draw(Ui *u, Canvas *c)
{
	const ScreenDef *d = screen_def(u->scr);

	cv_clear(c, PAL_NIGHT);
	u->nhits = 0;
	if (d->overlay && u->under != u->scr && !screen_def(u->under)->overlay) {
		draw_screen(u, c, u->under);
		u->nhits = 0;		/* only the topmost screen takes clicks */
		cv_dim(c);
	}
	draw_screen(u, c, u->scr);
	draw_toast(u, c);
}

void ui_press(Ui *u, int pad)
{
	const ScreenDef *d = screen_def(u->scr);

	if (pad < 0 || pad >= PAD_COUNT)
		return;
	u->nhits = 0;			/* the regions belong to the screen as it was drawn */
	if (d->press)
		d->press(u, pad);
	else
		stub_press(u, pad);
}

void ui_click(Ui *u, int x, int y)
{
	int i;

	for (i = u->nhits - 1; i >= 0; i--) {
		Hit h = u->hits[i];

		if (x < h.x || y < h.y || x >= h.x + h.w || y >= h.y + h.h)
			continue;
		u->nhits = 0;		/* used up, even by a hook that only moves a highlight */
		if (h.action == HIT_PAD)
			ui_press(u, h.arg);
		else if (screen_def(u->scr)->click)
			screen_def(u->scr)->click(u, h.action, h.arg);
		return;
	}
}

void ui_tick(Ui *u)
{
	u->frame++;
	if (u->toast[0] && (int32_t)(u->frame - u->toast_until) >= 0)
		u->toast[0] = '\0';
}

int ui_take_sfx(Ui *u)
{
	int s;

	if (u->nsfx <= 0)
		return -1;
	s = u->sfx[0];
	u->nsfx--;
	memmove(u->sfx, u->sfx + 1, (size_t)u->nsfx * sizeof u->sfx[0]);
	return s;
}

/* ---- state ---- */

void toast(Ui *u, const char *text)
{
	snprintf(u->toast, sizeof u->toast, "%s", text);
	u->toast_until = u->frame + TOAST_TICKS;
}

void hit(Ui *u, int x, int y, int w, int h, int action, int arg)
{
	Hit *t;

	if (u->nhits >= (int)(sizeof u->hits / sizeof u->hits[0]) || w <= 0 || h <= 0)
		return;
	t = &u->hits[u->nhits++];
	t->x = x;
	t->y = y;
	t->w = w;
	t->h = h;
	t->action = action;
	t->arg = arg;
}

void play(Ui *u, SfxId s)
{
	if (u->nsfx < (int)(sizeof u->sfx / sizeof u->sfx[0]))
		u->sfx[u->nsfx++] = (int)s;
}

void go(Ui *u, Screen s)
{
	u->scr = s;
	u->under = s;
	u->nhits = 0;
}

void overlay(Ui *u, Screen s, Screen under)
{
	u->scr = s;
	u->under = under;
	u->back = under;
	u->nhits = 0;
}

const char *err_text(int err)
{
	switch (err) {
	case E_NO_CASH: return "Not enough cash.";
	case E_NO_SPACE: return "No room.";
	case E_NOT_SOLD: return "Nobody's selling that here today.";
	case E_LIMIT: return "That's over the limit.";
	case E_FEE: return "That won't even cover the ATM fee.";
	case E_NO_STOCK: return "You don't have that many.";
	default: return "You can't do that right now.";
	}
}

/* ---- chrome ---- */

static uint32_t health_col(int hp)
{
	return hp > 60 ? PAL_MINT : hp > 30 ? PAL_GOLD : PAL_CORAL;
}

static void health_bar(Ui *u, Canvas *c)
{
	int hp = u->app->g.health, x = CV_W - 44, shown = hp < 0 ? 0 : hp;
	uint32_t col = health_col(hp);
	char num[16];

	cv_rect(c, x, 3, 40, 6, PAL_FAINT);
	cv_rect(c, x, 3, (40 * (shown > 100 ? 100 : shown) + 50) / 100, 6, col);
	art_icon(c, ICON_HEART, x - 10, 3, 1, 0);
	snprintf(num, sizeof num, "%d", shown);
	tx(c, num, x - 13, 2, col, 1, TX_RIGHT);
}

void top_bar(Ui *u, Canvas *c, const char *label, const char *right)
{
	const Game *g = &u->app->g;
	char day[32];

	cv_rect(c, 0, 0, CV_W, 12, PAL_NIGHT);
	if (!label) {
		snprintf(day, sizeof day, "DAY %d/%d", g->day, g->last_day);
		label = day;
	}
	tx(c, label, 4, 2, PAL_TEXT, 1, TX_LEFT);
	if (!right)
		health_bar(u, c);
	else if (right[0])
		tx(c, right, CV_W - 4, 2, PAL_GOLD, 1, TX_RIGHT);
}

void money_strip(Ui *u, Canvas *c, int y)
{
	const Game *g = &u->app->g;
	int64_t debt = 0;
	char s[32];
	int i;

	for (i = 0; i < NUM_CITIES; i++)	/* saturating, whatever is stored */
		debt = g->debt[i] > INT64_MAX - debt ? INT64_MAX : debt + g->debt[i];
	cv_rect(c, 0, y, CV_W, 14, PAL_PANEL);
	cv_rect(c, 0, y + 13, CV_W, 1, PAL_LINE);
	art_icon(c, ICON_COIN, 4, y + 3, 1, 0);
	fmt_money_short(g->cash, s, sizeof s);
	tx(c, s, 14, y + 3, PAL_GOLD, 1, TX_LEFT);
	art_icon(c, ICON_BANK, 112, y + 3, 1, 0);
	fmt_money_short(g->bank, s, sizeof s);
	tx(c, s, 122, y + 3, PAL_MINT, 1, TX_LEFT);
	art_icon(c, ICON_FIN, 216, y + 3, 1, 0);
	fmt_money_short(debt, s, sizeof s);
	tx(c, s, 227, y + 3, debt ? PAL_CORAL : PAL_DIM, 1, TX_LEFT);
}

void panel(Canvas *c, int x, int y, int w, int h, const char *title, int focus, const char *right)
{
	cv_rect(c, x, y, w, h, PAL_PANEL);
	cv_box(c, x, y, w, h, focus ? PAL_LINE_HI : PAL_LINE);
	tx(c, title, x + 4, y + 3, focus ? PAL_GOLD : PAL_DIM, 1, TX_LEFT);
	if (right && right[0])
		tx(c, right, x + w - 4, y + 3, focus ? PAL_TEXT : PAL_DIM, 1, TX_RIGHT);
	cv_rect(c, x + 1, y + 12, w - 2, 1, focus ? PAL_LINE : PAL_PANEL2);
}

void button(Ui *u, Canvas *c, int x, int y, int w, int h, const char *label, int on, int action, int arg)
{
	cv_rect(c, x, y, w, h, on ? PAL_HOT : PAL_PANEL2);
	cv_box(c, x, y, w, h, on ? PAL_WHITE : PAL_LINE);
	tx(c, label, x + (w >> 1), y + ((h - GLYPH_H) >> 1) + 1, on ? PAL_WHITE : PAL_TEXT, 1, TX_CENTER);
	if (action)
		hit(u, x, y, w, h, action, arg);
}

int badge(Canvas *c, const char *b, int x, int y)
{
	int w;

	if (b[0] && !b[1]) {
		uint32_t col = b[0] == 'A' ? PAL_GOLD : b[0] == 'B' ? PAL_CORAL :
			       b[0] == 'X' ? PAL_MINT : b[0] == 'Y' ? PAL_SKY : PAL_DIM;

		cv_disc(c, x + 5, y + 3, 5, col);
		tx(c, b, x + 2, y, PAL_NIGHT, 1, TX_LEFT);
		return 11;
	}
	w = tx_width(b, 1) + 4;
	cv_rect(c, x, y - 1, w, 10, PAL_FAINT);
	tx(c, b, x + 2, y, PAL_TEXT, 1, TX_LEFT);
	return w;
}

void hints(Canvas *c, const Hint *list, int n, int y)
{
	int x = 4, i;

	cv_rect(c, 0, y - 4, CV_W, CV_H - (y - 4), PAL_NIGHT);
	for (i = 0; i < n; i++) {
		x += badge(c, list[i].b, x, y) + 3;
		x += tx(c, list[i].label, x, y, PAL_TEXT, 1, TX_LEFT) + 10;
	}
}

void city_label(int city, char *buf, size_t n)
{
	const char *s;
	size_t i;

	if (!n)
		return;
	if (city < 0 || city >= NUM_CITIES) {
		buf[0] = '\0';
		return;
	}
	s = CITIES[city].nickname[0] ? CITIES[city].nickname : CITIES[city].name;
	for (i = 0; s[i] && i + 1 < n; i++)
		buf[i] = (char)toupper((unsigned char)s[i]);
	buf[i] = '\0';
}

void place_label(Canvas *c, int city, const char *line2, int gy)
{
	char name[32];

	city_label(city, name, sizeof name);
	tx_outline(c, name, 6, gy - 24, PAL_GOLD, PAL_BLACK, 1, TX_LEFT);
	tx_outline(c, line2, 6, gy - 13, PAL_WHITE, PAL_BLACK, 1, TX_LEFT);
}

/* ---- money ---- */

void fmt_money_short(int64_t v, char *buf, size_t n)
{
	static const struct { uint64_t unit; char suffix; } U[3] = {
		{1000000u, 'M'}, {1000000000u, 'B'}, {1000000000000u, 'T'},
	};
	uint64_t a = v < 0 ? (uint64_t)0 - (uint64_t)v : (uint64_t)v, tenths;
	int k;

	if (a < 10000000u) {
		fmt_money(v, buf, n);
		return;
	}
	k = a >= U[2].unit ? 2 : a >= U[1].unit ? 1 : 0;
	tenths = (a + U[k].unit / 20) / (U[k].unit / 10);	/* to the nearest tenth */
	if (tenths >= 10000 && k < 2) {				/* $1000.0M reads better as $1.0B */
		k++;
		tenths = (a + U[k].unit / 20) / (U[k].unit / 10);
	}
	snprintf(buf, n, "%s$%llu.%u%c", v < 0 ? "-" : "", (unsigned long long)(tenths / 10),
		 (unsigned)(tenths % 10), U[k].suffix);
}
