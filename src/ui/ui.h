/* The screen state machine: one Ui on top of an App. It reads the game, changes it only
 * through the core API, queues sounds for the platform to play and draws into a Canvas. */
#ifndef PH_UI_H
#define PH_UI_H

#include <stddef.h>
#include <stdint.h>

#include "core/game.h"
#include "gfx/canvas.h"
#include "gfx/font.h"
#include "ui/app.h"

typedef enum { SCR_TITLE, SCR_NEWGAME, SCR_MARKET, SCR_PICKER, SCR_NEWS, SCR_MOVE, SCR_FLY, SCR_BANK,
	       SCR_LENDER, SCR_STORE, SCR_CHASE, SCR_CHART, SCR_MENU, SCR_OVER, SCR_SCORES, NUM_SCREENS } Screen;
typedef enum { PK_BUY, PK_SELL, PK_HEAL, PK_DEPOSIT, PK_WITHDRAW, PK_BORROW, PK_REPAY } PickKind;
typedef enum { CARD_TRAVEL, CARD_MARKET, CARD_TROUBLE, CARD_RUMOR, CARD_DAY } CardKind;
typedef struct { CardKind kind; int8_t good; char text[200]; } Card;
typedef struct { int x, y, w, h, action, arg; } Hit;	/* a clickable region, canvas pixels */

typedef struct Ui {
	App *app;
	Screen scr, under, back;		/* under: drawn beneath an overlay; back: where B returns */
	int panel, mrow, brow, act;		/* market: 0 market, 1 bag, 2 actions */
	int row;				/* the highlighted row on list screens */
	struct { PickKind kind; int good; int64_t q; int sel; } pk;
	struct { Card cards[MAX_MSGS + 2]; int n, i; Screen after; } news;
	char chase_log[400]; int chase_done;	/* the last action's lines; a CR_* */
	struct { int zone, len, kr, kc, shift; char name[16]; } ng;
	int chart_good, scores_tab, title_row, menu_row;
	int confirm;				/* a yes/no prompt is up: CONFIRM_END (menu) or CONFIRM_REPLACE (new game) */
	Screen menu_back, scores_back;		/* the screen the pause menu covers; where the scores came from */
	char toast[96]; uint32_t toast_until;
	uint32_t frame;
	Hit hits[64]; int nhits;
	int sfx[8], nsfx;
	int quit;
} Ui;

void ui_init(Ui *u, App *a);			/* starts on SCR_TITLE */
void ui_press(Ui *u, int pad);			/* a PAD_* button that fired */
/* Canvas coordinates. A click acts on the regions of the last ui_draw, topmost first; any
 * press or click voids them until the next draw, so a click never lands on a stale screen. */
void ui_click(Ui *u, int x, int y);
/* After a core call that may advance the day or end the trading phase: autosave and score
 * through app_after_action, then show the morning news and go on to the chase, the end or
 * the market. */
void ui_after_action(Ui *u, int16_t day_before, Phase phase_before);
/* Turns g->msgs into news cards and shows them over the market, then goes to `after`; with
 * no cards, goes straight there. */
void ui_collect_news(Ui *u, Screen after);
/* Where Continue on the title goes: SCR_CHASE for a save in a chase, else SCR_MARKET. */
Screen ui_resume_screen(const Ui *u);
int  ui_quit(const Ui *u);			/* the title's QUIT was chosen */
void ui_tick(Ui *u);				/* 20 Hz */
void ui_draw(Ui *u, Canvas *c);			/* rebuilds hits for the topmost screen only */
int  ui_take_sfx(Ui *u);			/* next SfxId, or -1 */
void fmt_money_short(int64_t v, char *buf, size_t n);	/* $9,999,999 then $12.3M, $4.5B, $1.2T */

#endif
