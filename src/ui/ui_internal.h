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
	HIT_FLY_ROW,		/* arg: a place in the flight grid (see fly_city) */
	HIT_FLY_TAB,		/* arg: a COUNTRY_*, the flight screen's tab */
	HIT_MONEY_BTN,		/* arg: 0 or 1, the left or right button of the bank or the lender */
	HIT_DIAL_UP,		/* arg: a digit of the money dial; adds 10^digit */
	HIT_DIAL_DOWN,		/* arg: a digit of the money dial; removes 10^digit */
	HIT_STORE_ROW,		/* arg: a row of the shop (a GEAR index) */
	HIT_CHASE_BTN,		/* arg: 0 RUN, 1 FIGHT, 2 SURRENDER */
	HIT_TITLE_ROW,		/* arg: 0 CONTINUE, 1 NEW GAME, 2 HIGH SCORES, 3 QUIT */
	HIT_LEN,		/* arg: a game length chip, 0..3 */
	HIT_KEY,		/* arg: row * 10 + column of the on-screen keyboard */
	HIT_NG_GO,		/* START HUSTLING */
	HIT_MENU_ROW,		/* arg: a row of the pause menu */
	HIT_CONFIRM,		/* arg: 1 yes, 0 no, -1 the blocker behind a prompt */
	HIT_SCORES_TAB,		/* arg: a game length, 0..3 */
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

/* One core call from a screen: remember the day and phase, call fn, toast the error (and
 * play SFX_DENY) on failure, else play ok_sfx (-1: none) and run the after-action work.
 * news 1: ui_after_action (autosave, score, morning news, on to the next screen); news 0:
 * only the autosave and score, and the screen stays (the chase shows its own result first).
 * Returns the core's result. */
typedef int (*CoreCall)(Game *g, const void *arg);
int ui_core_call(Ui *u, CoreCall fn, const void *arg, int ok_sfx, int news);

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
/* "WILD: 3-MMC, 2C-B" for the stakes running (what does not fit becomes "+N"), at most 17
 * characters; "" when none are. It takes the bag panel's bottom line from the weapon. */
void bag_wild_line(const Game *g, char *buf, size_t n);

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
/* The flight grid: the cities of the open tab (u->fly_tab) except this one, in table order,
 * filled down the left column (seven) and then the right. Places 0..fly_count-1. */
int  fly_count(const Ui *u);
int  fly_city(const Ui *u, int place);	/* -1 past the end */
typedef struct { char passenger[32], route[32], lands[32], fare[24], border[24]; uint32_t fare_col; } PassText;
void fly_pass_text(const Ui *u, int dest, PassText *t);	/* fare is BANNED for a closed city; border is "" without a crossing */
const char *prompt_label(int yes, int glyphs);	/* "A YES"/"B NO" with glyphs, else "YES"/"NO" */
void prompt_ask(Ui *u, Canvas *c, int x, int y, int w, const char *q, int yes_on, int glyphs);	/* a yes/no prompt, either answer highlighted */
void news_answer(Ui *u, int accept, int64_t stake);	/* answers the offer card on screen (stake: the chemical's) */
int  news_ask_hints(const Ui *u, const Hint **h);	/* the hint bar while the crew prompt is up; 0 when it is not */
const char *news_ask_text(const Ui *u);	/* the crew prompt's question */
void news_click(Ui *u, int action, int arg);
int  news_choice_label(const Ui *u, int which, char *buf, size_t n);	/* which 1 = A, 2 = B; 0 if the card asks nothing */

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

/* The cop chase (scr_chase.c). chase_done says how it ended once the phase leaves PH_CHASE. */
enum { CR_NONE, CR_ESCAPED, CR_WON, CR_JAILED, CR_DEAD };
void chase_begin(Ui *u);				/* a chase just started: RUN, siren, the cops' line */
void chase_title(const Game *g, char *buf, size_t n);	/* "SGT. BUZZKILL + 2 COPS" */
void chase_odds(const Game *g, int row, char *buf, size_t n);	/* the line under the buttons */
void chase_draw(Ui *u, Canvas *c);
void chase_press(Ui *u, int pad);
void chase_click(Ui *u, int action, int arg);

/* The price chart (scr_chart.c). */
typedef struct { char title[32], day[24], range[64], have[64]; } ChartText;
void    chart_k(int64_t v, char *buf, size_t n);	/* "$999", "$1.2K", "$20M": the mockup's kfmt */
int64_t chart_top(int64_t v);				/* the mockup's niceTop: 1, 1.5, 2, 2.5, 3, 4, 5, 6, 8, 10 times a power of ten */
void    chart_text(const Game *g, int good, ChartText *t);
void    chart_open(Ui *u, int good);			/* SELECT on the market; B returns to the market */
void    chart_draw(Ui *u, Canvas *c);
void    chart_press(Ui *u, int pad);

/* The title, the new game screen, the pause menu, game over and the high scores
 * (scr_menus.c). */
typedef struct { char label[16], value[24]; } StatCell;
enum { CONFIRM_NONE, CONFIRM_END, CONFIRM_REPLACE };
enum { TITLE_ROWS = 4, MENU_ROWS = 5, NG_NAME_MAX = 15 };
void title_detail(const App *a, char *buf, size_t n);	/* "DAY 3, HOUSTON" or "NO SAVED GAME" */
int  title_city(const Ui *u);				/* the skyline behind the logo: changes every 80 ticks */
void title_draw(Ui *u, Canvas *c);
void title_press(Ui *u, int pad);
void title_click(Ui *u, int action, int arg);
void ng_open(Ui *u, int len, const char *name);		/* the new game screen; len 0..3, the keyboard focused */
int  ng_lower(const Ui *u);				/* the next letter typed is lower case */
void ng_draw(Ui *u, Canvas *c);
void ng_press(Ui *u, int pad);
void ng_click(Ui *u, int action, int arg);
void menu_open(Ui *u, Screen from);			/* START: the pause menu over `from` */
void menu_label(const App *a, int row, char *buf, size_t n);
void menu_draw(Ui *u, Canvas *c);
void menu_press(Ui *u, int pad);
void menu_click(Ui *u, int action, int arg);
void over_title(const Game *g, char *buf, size_t n);	/* "TIME'S UP", "GAME OVER" or "RETIRED EARLY" */
void over_rank(const App *a, char *buf, size_t n);	/* "HIGH SCORE #4 FOR 30 DAYS" or the no-score line */
int  over_stats(const Game *g, StatCell out[12]);	/* the grid cells in reading order; the count */
void over_draw(Ui *u, Canvas *c);
void over_press(Ui *u, int pad);
int  scores_hilite(const App *a, int tab);		/* the row just recorded on that tab, else -1 */
void scores_draw(Ui *u, Canvas *c);
void scores_press(Ui *u, int pad);
void scores_click(Ui *u, int action, int arg);

/* The amount picker (scr_picker.c): quantities step 1 and 10, money uses the digit dial. */
typedef struct { char title[32], unit[48], total[96], info[96]; uint32_t total_col; } PickText;
void    picker_open(Ui *u, PickKind kind, int good, Screen back);	/* good: -1 if none */
void    picker_draw(Ui *u, Canvas *c);
void    picker_press(Ui *u, int pad);
void    picker_click(Ui *u, int action, int arg);
int64_t picker_max(const Ui *u);
void    picker_text(const Ui *u, PickText *t);

#endif
