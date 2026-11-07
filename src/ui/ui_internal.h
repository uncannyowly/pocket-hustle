/* Shared inside src/ui: the hooks each screen gives the dispatch table, the chrome every
 * screen draws with (the mockup's topBar, moneyStrip, panel, button, badge, hints, placeLabel
 * and drawToast), hit actions, and the string builders the screens draw from. Tests read
 * those strings, since pixels cannot be read back as text. */
#ifndef PH_UI_INTERNAL_H
#define PH_UI_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "gfx/canvas.h"
#include "sfx/sfx.h"
#include "ui/ui.h"

/* The dispatch table (SCREENS in ui.c) has an entry per Screen: draw, press, click and an
 * overlay flag. An overlay is drawn over `under`, dimmed, and only its own regions take
 * clicks. A screen without draw and press hooks is not built yet: it draws its name and
 * only B does anything (back to the market). Building a screen means filling in its hooks.
 *	void draw(Ui *u, Canvas *c);		  adds the screen's hit regions as it draws
 *	void press(Ui *u, int pad);		  a PAD_* button
 *	void click(Ui *u, int action, int arg);  a HIT_* action other than HIT_PAD; NULL if none */
int ui_stub(Screen s);				/* 1 while s is not built */

/* Hit actions. ui_click replays the topmost region under the point through the same code
 * as the buttons: HIT_PAD presses arg; any other action goes to the current screen's click
 * hook, which moves the highlight there and presses A. */
enum {
	HIT_PAD = 1,		/* arg: a PAD_* button */
	HIT_MARKET_ROW,		/* arg: a good */
	HIT_BAG_ROW,		/* arg: a line of the bag */
	HIT_MARKET_ACT,		/* arg: MKT_* */
	HIT_MOVE_ROW,		/* arg: a hood */
	HIT_FLY_ROW,		/* arg: a row of the flight list */
	HIT_MONEY_BTN,		/* arg: 0 or 1, the left or right button of the bank or the lender */
	HIT_DIAL_UP,		/* arg: a digit of the money dial; adds 10^digit */
	HIT_DIAL_DOWN,		/* arg: a digit of the money dial; removes 10^digit */
	HIT_STORE_ROW,		/* arg: a row of the shop (a GEAR index) */
};

/* Chrome. */
typedef struct { const char *b, *label; } Hint;	/* a button badge and what it does */
enum { HINTS_Y = 227 };					/* the hint bar's usual text row */

/* label NULL: "DAY d/n". right NULL: the health bar; "": nothing; else gold text. */
void top_bar(Ui *u, Canvas *c, const char *label, const char *right);
void money_strip(Ui *u, Canvas *c, int y);		/* cash, bank and all debts, 14 tall */
void panel(Canvas *c, int x, int y, int w, int h, const char *title, int focus, const char *right);
/* action 0: no hit region. */
void button(Ui *u, Canvas *c, int x, int y, int w, int h, const char *label, int on, int action, int arg);
int  badge(Canvas *c, const char *b, int x, int y);	/* returns its width */
void hints(Canvas *c, const Hint *list, int n, int y);	/* the bar along the bottom */
void city_label(int city, char *buf, size_t n);		/* "LOUD CITY", "HOUSTON" */
void place_label(Canvas *c, int city, const char *line2, int gy);	/* gy: the skyline's bottom */

/* State. */
void toast(Ui *u, const char *text);			/* shown for 40 ticks */
void hit(Ui *u, int x, int y, int w, int h, int action, int arg);
void play(Ui *u, SfxId s);				/* a full queue drops it */
void go(Ui *u, Screen s);				/* a full screen */
void overlay(Ui *u, Screen s, Screen under);		/* s over under; back = under */
const char *err_text(int err);				/* a core Err as a toast */

/* The market (scr_market.c). */
enum { MKT_MOVE, MKT_FLY, MKT_BANK, MKT_LOAN, MKT_SHOP, MKT_DOC, NUM_MKT_ACTS };	/* its buttons */
enum { PF_NORMAL = EV_NONE, PF_SPIKE = EV_SPIKE, PF_CRASH = EV_CRASH };
void market_draw(Ui *u, Canvas *c);
void market_press(Ui *u, int pad);
void market_click(Ui *u, int action, int arg);
int  bag_list(const Game *g, int out[NUM_GOODS]);	/* the goods held, in order; the count */
/* Today's price against what was paid: "+12%", "-99%" (shown range -99..999) or "--" when
 * nobody buys it here or nothing was paid. Returns 1 for gains, -1 for losses, 0 for "--". */
int  bag_pct(const Game *g, int good, char *buf, size_t n);
/* PF_SPIKE or PF_CRASH when the market roll made today's price one (the core's hist_ev),
 * else PF_NORMAL; PF_NORMAL for a good that is not on sale. */
int  price_flag(const Game *g, int good);

/* News (scr_news.c), getting around and flying (scr_travel.c). */
void news_draw(Ui *u, Canvas *c);
void news_press(Ui *u, int pad);
void move_open(Ui *u);			/* the MOVE button */
void move_draw(Ui *u, Canvas *c);
void move_press(Ui *u, int pad);
void move_click(Ui *u, int action, int arg);
void fly_open(Ui *u);			/* the FLY button */
void fly_draw(Ui *u, Canvas *c);
void fly_press(Ui *u, int pad);
void fly_click(Ui *u, int action, int arg);
int  fly_city(const Game *g, int row);	/* the city on row 0..5 of the flight list: every city but this one */

/* The bank and the loan shark (scr_money.c). The text the screens draw, for tests to read. */
typedef struct { char head[16], tag[64], blurb[72]; uint32_t tag_col; } BankText;
typedef struct { char name[24], rate[48], limit[48], owed[32], paid[48], warn[128]; int overdue; } LenderText;
void rate_text(int32_t bp, char *buf, size_t n);	/* basis points as "9.5%" or "7%" */
void bank_text(const Game *g, BankText *t);
void lender_text(const Game *g, LenderText *t);		/* paid is "" while nothing is owed here */
void bank_open(Ui *u);					/* the BANK button */
void bank_draw(Ui *u, Canvas *c);
void bank_press(Ui *u, int pad);
void bank_click(Ui *u, int action, int arg);
void lender_open(Ui *u);				/* the LOAN button */
void lender_draw(Ui *u, Canvas *c);
void lender_press(Ui *u, int pad);
void lender_click(Ui *u, int action, int arg);

/* The shop (scr_store.c). One row per GEAR entry: weapons first, then the room items. */
typedef struct { char price[24], aim[16], status[64]; uint32_t price_col, status_col; } StoreText;
void store_text(const Game *g, int row, StoreText *t);	/* price is the selling city's name for gear sold elsewhere */
void store_open(Ui *u);					/* the SHOP button */
void store_draw(Ui *u, Canvas *c);
void store_press(Ui *u, int pad);
void store_click(Ui *u, int action, int arg);

/* The price chart (scr_chart.c). */
typedef struct { char title[32], day[24], range[64], have[64]; } ChartText;
void    chart_k(int64_t v, char *buf, size_t n);	/* "$999", "$1.2K", "$20M": the mockup's kfmt */
int64_t chart_top(int64_t v);				/* the mockup's niceTop: 1, 1.5, 2, 2.5, 3, 4, 5, 6, 8, 10 times a power of ten */
void    chart_text(const Game *g, int good, ChartText *t);
void    chart_open(Ui *u, int good);			/* SELECT on the market; B returns to the market */
void    chart_draw(Ui *u, Canvas *c);
void    chart_press(Ui *u, int pad);

/* The amount picker (scr_picker.c): quantities step 1 and 10, money uses the digit dial. */
typedef struct { char title[32], unit[48], total[96], info[96]; uint32_t total_col; } PickText;
void    picker_open(Ui *u, PickKind kind, int good, Screen back);	/* good: -1 if none */
void    picker_draw(Ui *u, Canvas *c);
void    picker_press(Ui *u, int pad);
void    picker_click(Ui *u, int action, int arg);
int64_t picker_max(const Ui *u);
void    picker_text(const Ui *u, PickText *t);

#endif
