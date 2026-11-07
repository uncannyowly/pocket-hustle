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
enum { PF_NORMAL, PF_SPIKE, PF_CRASH };
void market_draw(Ui *u, Canvas *c);
void market_press(Ui *u, int pad);
void market_click(Ui *u, int action, int arg);
int  bag_list(const Game *g, int out[NUM_GOODS]);	/* the goods held, in order; the count */
/* Today's price against what was paid: "+12%", "-99%" (shown range -99..999) or "--" when
 * nobody buys it here or nothing was paid. Returns 1 for gains, -1 for losses, 0 for "--". */
int  bag_pct(const Game *g, int good, char *buf, size_t n);
/* PF_SPIKE above anything an ordinary day here can ask (the good's range with this city's
 * bias), PF_CRASH below it, else PF_NORMAL. */
int  price_flag(const Game *g, int good);
/* What the market draws: today's announced spike or crash (u->ann), else price_flag. */
int  ui_price_flag(const Ui *u, int good);

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

/* The amount picker (scr_picker.c): quantities step 1 and 10, money uses the digit dial. */
typedef struct { char title[32], unit[40], total[64], info[64]; uint32_t total_col; } PickText;
void    picker_open(Ui *u, PickKind kind, int good, Screen back);	/* good: -1 if none */
void    picker_draw(Ui *u, Canvas *c);
void    picker_press(Ui *u, int pad);
int64_t picker_max(const Ui *u);
void    picker_text(const Ui *u, PickText *t);

#endif
