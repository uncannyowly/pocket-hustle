/* The UI frame, the market screen and the amount picker. Coordinates in the click tests come
 * from the mockup's drawMarket and drawPicker, worked out by hand. */
#include <stdint.h>
#include <string.h>

#include "check.h"
#include "helpers.h"
#include "tests.h"
#include "ui_helpers.h"
#include "gfx/palette.h"
#include "sfx/sfx.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

static App app;
static Ui ui, ui_before;
static Canvas cv;
static Game before;

static void snap(void)
{
	memcpy(&before, &app.g, sizeof before);
}

static void drain(void)
{
	while (ui_take_sfx(&ui) >= 0)
		;
}

/* Takes every queued sound; 1 if id was among them. */
static int heard(int id)
{
	int s, found = 0;

	while ((s = ui_take_sfx(&ui)) >= 0)
		if (s == id)
			found = 1;
	return found;
}

static uint32_t px(int x, int y)
{
	return cv.px[y * CV_W + x];
}

static void test_market_panels_cycle(void)
{
	ui_test_setup(&app, &ui, 30);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_EQ(ui.panel, 0);
	press_seq(&ui, "R1");
	CHECK_EQ(ui.panel, 1);
	press_seq(&ui, "R1");
	CHECK_EQ(ui.panel, 2);
	press_seq(&ui, "R1");
	CHECK_EQ(ui.panel, 0);
	press_seq(&ui, "L1");
	CHECK_EQ(ui.panel, 2);
	press_seq(&ui, "L1");
	CHECK_EQ(ui.panel, 1);
	press_seq(&ui, "L1");
	CHECK_EQ(ui.panel, 0);
	CHECK(heard(SFX_MOVE));

	press_seq(&ui, "R");			/* RIGHT from the market: the bag */
	CHECK_EQ(ui.panel, 1);
	press_seq(&ui, "L");			/* LEFT from the bag: the market */
	CHECK_EQ(ui.panel, 0);

	ui.mrow = 0;
	press_seq(&ui, "U");			/* up from the top wraps to the bottom */
	CHECK_EQ(ui.mrow, 9);
	press_seq(&ui, "D");			/* DOWN from row 9: the actions */
	CHECK_EQ(ui.panel, 2);
	CHECK_EQ(ui.mrow, 9);
	press_seq(&ui, "U");			/* UP from the actions: the market */
	CHECK_EQ(ui.panel, 0);
	press_seq(&ui, "D L");			/* the actions wrap: left of MOVE is DOC */
	CHECK_EQ(ui.act, MKT_DOC);
	press_seq(&ui, "R");
	CHECK_EQ(ui.act, MKT_MOVE);
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_buy_max(void)
{
	Game want;

	ui_test_setup(&app, &ui, 30);
	app.g.price[1] = 300;			/* $2,600 buys 8 Weed */
	ui.mrow = 1;
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_BUY);
	CHECK_EQ(ui.pk.good, 1);
	CHECK_EQ(ui.pk.q, 0);
	CHECK(heard(SFX_OK));
	press_seq(&ui, "Y");
	CHECK_EQ(ui.pk.q, game_max_buy(&app.g, 1));
	CHECK_EQ(ui.pk.q, 8);

	memcpy(&want, &app.g, sizeof want);
	CHECK_EQ(game_buy(&want, 1, 8), OK);
	drain();
	press_seq(&ui, "A");
	CHECK(same_game(&app.g, &want));	/* exactly what game_buy does */
	CHECK_EQ(app.g.stash[1], 8);
	CHECK_EQ(app.g.cash, 200);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "Bought 8 Weed for $2,400.");
	CHECK(heard(SFX_CASH));
}

static void test_picker_steps(void)
{
	int i;

	ui_test_setup(&app, &ui, 30);
	app.g.price[0] = 100;			/* max = min(room 150, $2,600 / $100) = 26 */
	ui.mrow = 0;
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	press_seq(&ui, "U U U");
	CHECK_EQ(ui.pk.q, 3);
	press_seq(&ui, "R");
	CHECK_EQ(ui.pk.q, 13);
	press_seq(&ui, "L L");
	CHECK_EQ(ui.pk.q, 0);
	press_seq(&ui, "D");
	CHECK_EQ(ui.pk.q, 0);
	for (i = 0; i < 30; i++)
		press_seq(&ui, "U");
	CHECK_EQ(ui.pk.q, 26);
	press_seq(&ui, "L R");			/* 16, then +10 stops at the max */
	CHECK_EQ(ui.pk.q, 26);
	press_seq(&ui, "X");
	CHECK_EQ(ui.pk.q, 0);
	press_seq(&ui, "Y");
	CHECK_EQ(ui.pk.q, 26);
	CHECK_EQ(ui.scr, SCR_PICKER);
}

static void test_sell_from_bag(void)
{
	ui_test_setup(&app, &ui, 30);
	app.g.cash = 10000;
	app.g.price[2] = 500;
	app.g.price[5] = 2000;
	CHECK_EQ(game_buy(&app.g, 2, 2), OK);	/* bag line 0 */
	CHECK_EQ(game_buy(&app.g, 5, 3), OK);	/* bag line 1, $6,000 paid */
	press_seq(&ui, "R1 D A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_SELL);
	CHECK_EQ(ui.pk.good, 5);
	drain();
	press_seq(&ui, "X U A");
	CHECK_EQ(app.g.stash[5], 2);
	CHECK_EQ(app.g.stash[2], 2);
	CHECK_EQ(app.g.cash, 5000);		/* 10,000 - 1,000 - 6,000 + 2,000 */
	CHECK_EQ(app.g.stash_paid[5], 4000);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "Sold 1 Acid for $2,000.");
	CHECK(heard(SFX_CASH));
}

static void test_absent_good_toast(void)
{
	ui_test_setup(&app, &ui, 30);
	app.g.price[4] = 0;
	ui.mrow = 4;
	snap();
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "Nobody's selling Molly here today.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));

	app.g.price[4] = 1000;			/* now hold one that nobody buys here */
	CHECK_EQ(game_buy(&app.g, 4, 1), OK);
	app.g.price[4] = 0;
	snap();
	press_seq(&ui, "R1 A");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "Nobody's buying Molly here today.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));
}

static void test_picker_back_changes_nothing(void)
{
	ui_test_setup(&app, &ui, 30);
	CHECK_EQ(game_buy(&app.g, 0, 3), OK);	/* something to sell */
	app.g.health = 80;			/* and work for the doctor */
	snap();

	press_seq(&ui, "A U U U");
	CHECK_EQ(ui.pk.kind, PK_BUY);
	CHECK_EQ(ui.pk.q, 3);
	drain();
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(heard(SFX_BACK));

	press_seq(&ui, "R1 A U U");
	CHECK_EQ(ui.pk.kind, PK_SELL);
	CHECK_EQ(ui.pk.q, 2);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);

	press_seq(&ui, "R1 L A U U");		/* the actions; left of MOVE is DOC */
	CHECK_EQ(ui.pk.kind, PK_HEAL);
	CHECK_EQ(ui.pk.q, 2);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);

	press_seq(&ui, "U A A");		/* A at zero closes the picker too */
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(same_game(&before, &app.g));
}

static void test_click_row_opens_picker(void)
{
	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &cv);
	CHECK(hit_find(&ui, HIT_MARKET_ROW, 3) >= 0);
	ui_click(&ui, 84, 130);			/* good 3's row: (2, 125) 164 x 10 */
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_BUY);
	CHECK_EQ(ui.pk.good, 3);
	CHECK_EQ(ui.mrow, 3);
	CHECK_EQ(ui.panel, 0);

	press_seq(&ui, "B");
	ui.mrow = 0;
	ui_draw(&ui, &cv);
	ui_click(&ui, 140, 126);		/* the row's price column counts too */
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.good, 3);
}

static void test_click_outside_does_nothing(void)
{
	static const int pts[][2] = {
		{160, 40}, {300, 5}, {160, 232},	/* skyline, health bar, hint bar */
		{166, 130}, {1, 130}, {84, 94},		/* just outside good 3's and good 0's rows */
		{-1, -1}, {320, 240}, {5000, -5000},	/* off the canvas */
	};
	size_t k;

	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &cv);
	CHECK(ui.nhits > 0);
	snap();
	memcpy(&ui_before, &ui, sizeof ui);
	for (k = 0; k < sizeof pts / sizeof pts[0]; k++)
		ui_click(&ui, pts[k][0], pts[k][1]);
	CHECK(memcmp(&ui_before, &ui, sizeof ui) == 0);
	CHECK(same_game(&before, &app.g));
}

static void test_overlay_blocks_clicks(void)
{
	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &cv);			/* on the market, (10, 150) is good 5's row */
	ui_click(&ui, 10, 150);
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.good, 5);
	press_seq(&ui, "B");

	ui.mrow = 1;
	press_seq(&ui, "A U");			/* a picker for good 1, q = 1 */
	CHECK_EQ(ui.pk.good, 1);
	ui_draw(&ui, &cv);
	CHECK(ui.nhits > 0);
	CHECK_EQ(hit_find(&ui, HIT_MARKET_ROW, 5), -1);
	snap();
	ui_click(&ui, 10, 150);			/* beside the picker, over the dimmed row */
	ui_click(&ui, 84, 150);			/* under the picker box */
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.good, 1);
	CHECK_EQ(ui.pk.q, 1);
	CHECK(same_game(&before, &app.g));
}

static void test_fmt_money_short(void)
{
	static const struct { int64_t v; const char *want; } T[] = {
		{9999999, "$9,999,999"}, {12345678, "$12.3M"}, {4500000000LL, "$4.5B"},
		{MONEY_MAX, "$1000.0T"}, {-12345678, "-$12.3M"},
		{0, "$0"}, {-5, "-$5"}, {10000000, "$10.0M"}, {12350000, "$12.4M"},
		{999949999, "$999.9M"}, {999950000, "$1.0B"},	/* rounds into the next unit */
		{1000000000000LL, "$1.0T"}, {-MONEY_MAX, "-$1000.0T"},
	};
	char buf[32];
	size_t i;

	for (i = 0; i < sizeof T / sizeof T[0]; i++) {
		fmt_money_short(T[i].v, buf, sizeof buf);
		CHECK_STR(buf, T[i].want);
	}
	fmt_money_short(12345678, buf, 4);	/* truncated, still terminated */
	CHECK_STR(buf, "$12");
	fmt_money_short(INT64_MIN, buf, sizeof buf);	/* no overflow on the way */
	CHECK_EQ(buf[0], '-');
}

static void test_market_draws_clean(void)
{
	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(canvas_colors(&cv, 64) >= 10);
}

static void test_market_states_draw_clean(void)
{
	static const struct { int panel; const char *seq; } pickers[] = {
		{0, "A Y"}, {1, "A Y"}, {2, "L A Y"},	/* buy, sell, the doctor */
	};
	int g, f;
	size_t k;

	ui_test_setup(&app, &ui, 30);
	for (g = 0; g < NUM_GOODS; g++) {	/* a full bag, a line for every good */
		app.g.stash[g] = 15;
		app.g.stash_paid[g] = 15LL * GOODS[g].lo;
	}
	app.g.cash = app.g.bank = app.g.debt[0] = 9999999;	/* the widest unshortened amounts */
	app.g.price[9] = 207900;		/* a spiked Cocaine price */
	app.g.health = 7;
	ui.mrow = 9;
	ui.brow = 9;
	for (f = 0; f < 3; f++) {
		ui.panel = f;
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
	}
	toast(&ui, "Nobody's selling Ketamine here today. Try another hood tomorrow or fly out.");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	for (k = 0; k < sizeof pickers / sizeof pickers[0]; k++) {
		ui.panel = pickers[k].panel;
		ui.act = MKT_MOVE;
		press_seq(&ui, pickers[k].seq);
		CHECK_EQ(ui.scr, SCR_PICKER);
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
		press_seq(&ui, "B");
	}
	for (g = 7; g < NUM_GOODS; g++)		/* seven lines leave room for the weapon line */
		app.g.stash[g] = 0, app.g.stash_paid[g] = 0;
	app.g.weapon = 7;			/* T-Shirt Cannon, the longest weapon name */
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

/* Every screen is built: none falls back to the "Coming soon" stub. */
static void test_stub_screens(void)
{
	int s;

	app_init(&app, NULL);
	ui_init(&ui, &app);
	CHECK_EQ(ui.scr, SCR_TITLE);
	for (s = 0; s < NUM_SCREENS; s++)
		CHECK(!ui_stub((Screen)s));
}

static void test_doc_heals(void)
{
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "R1 R1 L");		/* the actions; left of MOVE is DOC */
	CHECK_EQ(ui.act, MKT_DOC);
	snap();
	drain();
	press_seq(&ui, "A");			/* at 100 there is nothing to fix */
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "You're in perfect health.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));

	app.g.health = 80;
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_HEAL);
	press_seq(&ui, "Y");
	CHECK_EQ(ui.pk.q, 20);			/* min(100 - 80, $2,600 / $60 = 43) */
	press_seq(&ui, "X U U U U U");
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(app.g.health, 85);
	CHECK_EQ(app.g.cash, 2300);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "The doctor patches you up. (+5 health)");
	CHECK(heard(SFX_OK));

	app.g.cash = 150;			/* now the cash runs out first: 2 points */
	press_seq(&ui, "A Y");
	CHECK_EQ(ui.pk.q, 2);
	press_seq(&ui, "A");
	CHECK_EQ(app.g.health, 87);
	CHECK_EQ(app.g.cash, 30);
}

static void test_toast_expires(void)
{
	int i;

	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "R1 R1 R R R R R A");	/* DOC at full health: a toast */
	CHECK(ui.toast[0] != '\0');
	ui_draw(&ui, &cv);
	CHECK_EQ(px(104, 176), PAL_GOLD);	/* the box is 112 x 18, its corner at (104, 176) */
	for (i = 0; i < 39; i++)
		ui_tick(&ui);
	CHECK(ui.toast[0] != '\0');
	ui_tick(&ui);				/* 40 ticks: gone */
	CHECK(ui.toast[0] == '\0');
	ui_draw(&ui, &cv);
	CHECK(px(104, 176) != PAL_GOLD);
}

static void test_sfx_queue(void)
{
	int i, n = 0;

	ui_test_setup(&app, &ui, 30);
	CHECK_EQ(ui_take_sfx(&ui), -1);
	press_seq(&ui, "R1 R1 R R R R R A");	/* seven moves, then DOC says no */
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_MOVE);
	CHECK_EQ(ui_take_sfx(&ui), SFX_DENY);
	CHECK_EQ(ui_take_sfx(&ui), -1);
	for (i = 0; i < 12; i++)
		press_seq(&ui, "R");
	while (ui_take_sfx(&ui) >= 0)
		n++;
	CHECK_EQ(n, 8);				/* it keeps 8 and drops the rest */
}

static void test_bag_percent(void)
{
	static const struct { int32_t price, stash; int64_t paid; const char *want; int dir; } T[] = {
		{150, 10, 1000, "+50%", 1},
		{50, 10, 1000, "-50%", -1},
		{100, 10, 1000, "+0%", 1},
		{126, 8, 1000, "+1%", 1},		/* +0.8% rounds to +1 */
		{124, 8, 1000, "-1%", -1},		/* -0.8% rounds to -1 */
		{1207, 12, 7056, "+105%", 1},		/* 12 bought at $588: stash_paid is the total */
		{10000, 10, 1000, "+999%", 1},		/* +9,900% shows the cap */
		{1, 10, 100000, "-99%", -1},		/* -99.99% shows the floor */
		{0, 10, 1000, "--", 0},			/* nobody buys it here */
		{150, 10, 0, "--", 0},			/* nothing paid on record */
		{INT32_MAX, INT32_MAX, 1, "+999%", 1},	/* no overflow at the extremes */
		{1, 1, INT64_MAX, "-99%", -1},
	};
	char buf[16];
	size_t i;

	ui_test_setup(&app, &ui, 30);
	for (i = 0; i < sizeof T / sizeof T[0]; i++) {
		app.g.price[3] = T[i].price;
		app.g.stash[3] = T[i].stash;
		app.g.stash_paid[3] = T[i].paid;
		CHECK_EQ(bag_pct(&app.g, 3, buf, sizeof buf), T[i].dir);
		CHECK_STR(buf, T[i].want);
	}
}

static void test_picker_texts(void)
{
	PickText t;

	ui_test_setup(&app, &ui, 30);
	app.g.price[0] = 100;
	ui.mrow = 0;
	press_seq(&ui, "A U U U");
	picker_text(&ui, &t);
	CHECK_STR(t.title, "BUY CARTS");
	CHECK_STR(t.unit, "$100 EACH");
	CHECK_STR(t.total, "TOTAL $300");
	CHECK_STR(t.info, "Room for 150. Cash for 26.");
	CHECK_EQ(t.total_col, PAL_GOLD);
	press_seq(&ui, "B");

	app.g.price[1] = 1207;			/* 12 Weed bought at $588 each */
	app.g.stash[1] = 12;
	app.g.stash_paid[1] = 7056;
	press_seq(&ui, "R1 A U U");
	picker_text(&ui, &t);
	CHECK_STR(t.title, "SELL WEED");
	CHECK_STR(t.unit, "$1,207 EACH");
	CHECK_STR(t.total, "YOU GET $2,414");
	CHECK_STR(t.info, "You have 12. Paid $588 each.");
	press_seq(&ui, "B");

	app.g.health = 85;
	press_seq(&ui, "R1 L A U U U U U");
	picker_text(&ui, &t);
	CHECK_STR(t.title, "THE DOCTOR");
	CHECK_STR(t.unit, "$60 A POINT");
	CHECK_STR(t.total, "COST $300");
	CHECK_STR(t.info, "Health 85 > 90.");
}

static void test_picker_info_fits(void)
{
	PickText t;

	ui_test_setup(&app, &ui, 30);
	app.g.price[9] = 40000;			/* 150 Cocaine bought at $33,000 each */
	app.g.stash[9] = 150;
	app.g.stash_paid[9] = 4950000;
	press_seq(&ui, "R1 A");
	picker_text(&ui, &t);
	CHECK(strlen(t.info) <= 30);		/* 30 cells fit inside the 248-pixel box */
	CHECK(strstr(t.info, "150") != NULL);
	CHECK(strstr(t.info, "$33,000") != NULL);
	press_seq(&ui, "B");

	app.g.stash[9] = 0;
	app.g.stash_paid[9] = 0;
	app.g.cash = 100000000;			/* $100M buys 3,333,333 Carts at $30 */
	app.g.price[0] = 30;
	ui.panel = 0;
	ui.mrow = 0;
	press_seq(&ui, "A");
	picker_text(&ui, &t);
	CHECK(strlen(t.info) <= 30);
	CHECK(strstr(t.info, "150") != NULL);
	CHECK(strstr(t.info, "3333333") != NULL);
}

static void test_err_texts(void)
{
	CHECK_STR(err_text(E_NO_CASH), "Not enough cash.");
	CHECK_STR(err_text(E_NO_SPACE), "No room.");
	CHECK_STR(err_text(E_NOT_SOLD), "Nobody's selling that here today.");
	CHECK_STR(err_text(E_LIMIT), "That's over the limit.");
	CHECK_STR(err_text(E_FEE), "That won't even cover the ATM fee.");
	CHECK_STR(err_text(E_NO_STOCK), "You don't have that many.");
	CHECK(err_text(E_PHASE)[0] != '\0');
}

static void test_core_refusal_toasts(void)
{
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "A U");			/* a buy of 1, then the core stops trading */
	app.g.phase = PH_CHASE;
	snap();
	drain();
	press_seq(&ui, "A");
	CHECK(same_game(&before, &app.g));
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, err_text(E_PHASE));
	CHECK(heard(SFX_DENY));
}

/* The flag is what the market roll recorded for today (hist_ev), never a guess from the price. */
static void test_price_flags(void)
{
	ui_test_setup(&app, &ui, 30);		/* Oklahoma City */
	app.g.price[1] = 691;			/* over the usual range, but ordinary today */
	CHECK_EQ(price_flag(&app.g, 1), PF_NORMAL);
	app.g.hist_ev[1][app.g.day] = EV_SPIKE;
	CHECK_EQ(price_flag(&app.g, 1), PF_SPIKE);
	app.g.price[1] = 400;			/* a spike inside the range is still one */
	CHECK_EQ(price_flag(&app.g, 1), PF_SPIKE);
	app.g.hist_ev[1][app.g.day] = EV_CRASH;
	CHECK_EQ(price_flag(&app.g, 1), PF_CRASH);
	app.g.price[1] = 0;			/* not on sale: no flag */
	CHECK_EQ(price_flag(&app.g, 1), PF_NORMAL);
	app.g.price[9] = 44551;			/* far over Cocaine's base $33,000: ordinary */
	CHECK_EQ(price_flag(&app.g, 9), PF_NORMAL);
	CHECK_EQ(price_flag(&app.g, -1), PF_NORMAL);
	CHECK_EQ(price_flag(&app.g, NUM_GOODS), PF_NORMAL);

	app.g.price[1] = 400;
	app.g.hist_ev[1][app.g.day] = EV_NONE;
	app.g.price[3] = 5000;
	app.g.hist_ev[3][app.g.day] = EV_SPIKE;
	app.g.price[5] = 900;
	app.g.hist_ev[5][app.g.day] = EV_CRASH;
	ui_draw(&ui, &cv);
	CHECK_EQ(px(160, 127), PAL_HOT);	/* row 3's up arrow at (158, 127), its tip */
	CHECK_EQ(px(160, 147), PAL_MINT);	/* row 5's down arrow */
	CHECK_EQ(px(160, 137), PAL_PANEL);	/* row 4 is ordinary: no arrow */
}

static void test_click_bag_and_buttons(void)
{
	ui_test_setup(&app, &ui, 30);
	CHECK_EQ(game_buy(&app.g, 0, 1), OK);	/* the bag: Carts, Shrooms */
	CHECK_EQ(game_buy(&app.g, 2, 1), OK);
	ui_draw(&ui, &cv);
	ui_click(&ui, 244, 110);		/* bag line 1: (170, 105) 148 x 10 */
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_SELL);
	CHECK_EQ(ui.pk.good, 2);
	CHECK_EQ(ui.panel, 1);
	CHECK_EQ(ui.brow, 1);
	press_seq(&ui, "B");

	ui_draw(&ui, &cv);
	ui_click(&ui, 27, 210);			/* MOVE: (2, 201) 51 x 18 */
	CHECK_EQ(ui.panel, 2);
	CHECK_EQ(ui.act, MKT_MOVE);
	CHECK_EQ(ui.scr, SCR_MOVE);
	press_seq(&ui, "B");
	ui_draw(&ui, &cv);
	ui_click(&ui, 133, 210);		/* BANK: (108, 201) 51 x 18 */
	CHECK_EQ(ui.act, MKT_BANK);
	CHECK_EQ(ui.scr, SCR_BANK);
	press_seq(&ui, "B");
	ui_draw(&ui, &cv);
	ui_click(&ui, 186, 210);		/* LOAN: (161, 201) */
	CHECK_EQ(ui.act, MKT_LOAN);
	CHECK_EQ(ui.scr, SCR_LENDER);
	press_seq(&ui, "B");

	app.g.health = 50;
	ui_draw(&ui, &cv);
	ui_click(&ui, 292, 210);		/* DOC: (267, 201) 51 x 18 */
	CHECK_EQ(ui.act, MKT_DOC);
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_HEAL);
}

static void test_picker_clicks(void)
{
	ui_test_setup(&app, &ui, 30);
	app.g.price[0] = 100;			/* max 26 */
	ui.mrow = 0;
	press_seq(&ui, "A");
	ui_draw(&ui, &cv);
	ui_click(&ui, 160, 78);			/* over the number: +1 */
	CHECK_EQ(ui.pk.q, 1);
	ui_draw(&ui, &cv);
	ui_click(&ui, 248, 91);			/* the right arrow: +10 */
	CHECK_EQ(ui.pk.q, 11);
	ui_draw(&ui, &cv);
	ui_click(&ui, 72, 91);			/* the left arrow: -10 */
	CHECK_EQ(ui.pk.q, 1);
	ui_draw(&ui, &cv);
	ui_click(&ui, 160, 102);		/* under the number: -1 */
	CHECK_EQ(ui.pk.q, 0);
	ui_draw(&ui, &cv);
	ui_click(&ui, 86, 175);			/* the Y badge: max */
	CHECK_EQ(ui.pk.q, 26);
	ui_draw(&ui, &cv);
	ui_click(&ui, 138, 175);		/* X: zero */
	CHECK_EQ(ui.pk.q, 0);
	ui_draw(&ui, &cv);
	ui_click(&ui, 234, 175);		/* B: back */
	CHECK_EQ(ui.scr, SCR_MARKET);

	press_seq(&ui, "A U U");
	ui_draw(&ui, &cv);
	ui_click(&ui, 186, 175);		/* A: buy */
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_EQ(app.g.stash[0], 2);

	press_seq(&ui, "A");			/* one click, one action: the regions wait for a redraw */
	ui_draw(&ui, &cv);
	ui_click(&ui, 160, 78);
	ui_click(&ui, 160, 78);
	CHECK_EQ(ui.pk.q, 1);
}

static void test_press_voids_regions(void)
{
	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &cv);
	press_seq(&ui, "D");			/* a press: what was drawn is stale now */
	ui_click(&ui, 10, 150);			/* good 5's row as last drawn */
	CHECK_EQ(ui.scr, SCR_MARKET);
	ui_draw(&ui, &cv);
	ui_click(&ui, 10, 150);			/* redrawn, the same click lands */
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.good, 5);
}

static void test_bag_navigation(void)
{
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "R1 U A");		/* an empty bag: UP and A do nothing */
	CHECK_EQ(ui.panel, 1);
	CHECK_EQ(ui.scr, SCR_MARKET);
	press_seq(&ui, "D");			/* DOWN from an empty bag: the actions */
	CHECK_EQ(ui.panel, 2);

	CHECK_EQ(game_buy(&app.g, 0, 1), OK);
	CHECK_EQ(game_buy(&app.g, 2, 1), OK);
	press_seq(&ui, "U R");			/* the market, then RIGHT to the bag */
	CHECK_EQ(ui.panel, 1);
	press_seq(&ui, "U");			/* up from line 0 wraps to line 1 */
	CHECK_EQ(ui.brow, 1);
	press_seq(&ui, "D");			/* DOWN from the last line: the actions */
	CHECK_EQ(ui.panel, 2);

	ui.panel = 1;				/* the bag shrinks under the highlight */
	ui.brow = 1;
	CHECK_EQ(game_sell(&app.g, 2, 1), OK);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.good, 0);
	press_seq(&ui, "B");
	CHECK_EQ(ui.brow, 0);
}

void tests_ui_market(void)
{
	test_market_panels_cycle();
	test_buy_max();
	test_picker_steps();
	test_sell_from_bag();
	test_absent_good_toast();
	test_picker_back_changes_nothing();
	test_click_row_opens_picker();
	test_click_outside_does_nothing();
	test_overlay_blocks_clicks();
	test_fmt_money_short();
	test_market_draws_clean();
	test_market_states_draw_clean();
	test_stub_screens();
	test_doc_heals();
	test_toast_expires();
	test_sfx_queue();
	test_bag_percent();
	test_picker_texts();
	test_picker_info_fits();
	test_err_texts();
	test_core_refusal_toasts();
	test_price_flags();
	test_click_bag_and_buttons();
	test_picker_clicks();
	test_press_voids_regions();
	test_bag_navigation();
}
