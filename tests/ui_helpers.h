/* Driving the UI headless: a ready game on the market, scripted presses, hit lookups. */
#ifndef PH_UI_HELPERS_H
#define PH_UI_HELPERS_H

#include "gfx/canvas.h"
#include "ui/ui.h"

/* A memory-only App holding a test_params() game (seed 1, name "Test", every random event
 * off, every good on sale), and a Ui on SCR_MARKET. */
void ui_test_setup(App *a, Ui *u, int days);
/* Presses buttons by token: U D L R A B X Y L1 R1 SEL START, space separated.
 * An unknown token stops the test run. */
void press_seq(Ui *u, const char *seq);
int  hit_find(const Ui *u, int action, int arg);	/* index into u->hits, or -1 */
void click_hit(Ui *u, int i);				/* clicks the middle of hit i */
int  canvas_colors(const Canvas *c, int cap);		/* distinct colors, counted up to cap */

#endif
