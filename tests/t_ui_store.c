/* The shop, the doctor and the price chart. Positions come from the mockup's drawStore and
 * drawChart, worked out by hand. Text is read through the string builders. */
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "helpers.h"
#include "tests.h"
#include "ui_helpers.h"
#include "content/content.h"
#include "gfx/palette.h"
#include "sfx/sfx.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

static App app;
static Ui ui;
static Canvas cv;

static void drain(void)
{
	while (ui_take_sfx(&ui) >= 0)
		;
}

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

/* Oklahoma City (0) or Houston (1), cash as given, standing in the SHOP. */
static void open_shop(int city, int64_t cash)
{
	ui_test_setup(&app, &ui, 30);
	app.g.loc.city = (uint8_t)city;
	app.g.loc.hood = 0;
	app.g.cash = cash;
	press_seq(&ui, "R1 R1 R R R R A");
	drain();
}

static void test_store_opens_and_draws(void)
{
	open_shop(0, 10000);
	CHECK_EQ(ui.scr, SCR_STORE);
	CHECK_EQ(ui.row, 0);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(hit_find(&ui, HIT_STORE_ROW, 0) >= 0);
	CHECK(hit_find(&ui, HIT_STORE_ROW, NUM_GEAR - 1) >= 0);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_store_navigation(void)
{
	open_shop(0, 10000);
	press_seq(&ui, "U");
	CHECK_EQ(ui.row, NUM_GEAR - 1);
	press_seq(&ui, "D");
	CHECK_EQ(ui.row, 0);
	press_seq(&ui, "D D D");
	CHECK_EQ(ui.row, 3);
	CHECK(heard(SFX_MOVE));
	press_seq(&ui, "L R X Y L1 R1 SEL");	/* nothing here */
	CHECK_EQ(ui.row, 3);
	CHECK_EQ(ui.scr, SCR_STORE);
}

static void test_store_buy_weapon(void)
{
	int64_t cash0;

	open_shop(0, 10000);
	cash0 = app.g.cash;
	press_seq(&ui, "D A");
	CHECK_EQ(app.g.weapon, 1);
	CHECK_EQ(app.g.cash, cash0 - GEAR[1].price);
	CHECK_STR(ui.toast, "You bought the Selfie Stick.");
	CHECK(heard(SFX_CASH));
	CHECK_EQ(ui.scr, SCR_STORE);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_store_room_stacks(void)
{
	int32_t cap0;
	int64_t cash0;

	open_shop(0, 10000);
	cap0 = app.g.capacity;
	cash0 = app.g.cash;
	press_seq(&ui, "U U U U A");		/* row 8, the first room item */
	CHECK_EQ(ui.row, 8);
	CHECK_EQ(app.g.capacity, cap0 + GEAR[8].power);
	press_seq(&ui, "D A");
	CHECK_EQ(app.g.capacity, cap0 + GEAR[8].power + GEAR[9].power);
	CHECK_EQ(app.g.cash, cash0 - GEAR[8].price - GEAR[9].price);
	CHECK(app.g.gear_owned[8] && app.g.gear_owned[9]);
}

static void test_store_other_city_toast(void)
{
	int64_t cash0;
	int i;

	open_shop(0, 100000);			/* the Rodeo Lasso is sold in Houston only */
	cash0 = app.g.cash;
	CHECK_EQ(GEAR[3].city, 1);
	press_seq(&ui, "D D D A");
	CHECK_EQ(ui.row, 3);
	CHECK_STR(ui.toast, "Only sold in Houston.");
	CHECK(heard(SFX_DENY));
	CHECK_EQ(app.g.cash, cash0);
	CHECK_EQ(app.g.weapon, -1);
	for (i = 0; i < MAX_GEAR; i++)
		CHECK_EQ(app.g.gear_owned[i], 0);
	open_shop(1, 100000);			/* and there it is for sale */
	press_seq(&ui, "D D D A");
	CHECK_EQ(app.g.weapon, 3);
}

static void test_store_owned_toast(void)
{
	int64_t cash0;

	open_shop(0, 100000);
	press_seq(&ui, "D A");			/* the Selfie Stick, bought */
	cash0 = app.g.cash;
	ui.toast[0] = '\0';
	drain();
	press_seq(&ui, "A");
	CHECK_STR(ui.toast, "That's what you're packing.");
	CHECK(heard(SFX_DENY));
	CHECK_EQ(app.g.cash, cash0);
	press_seq(&ui, "U U U U A");		/* row 9 bought, then again */
	CHECK_EQ(ui.row, 9);
	CHECK_EQ(app.g.gear_owned[9], 1);
	cash0 = app.g.cash;
	ui.toast[0] = '\0';
	press_seq(&ui, "A");
	CHECK_STR(ui.toast, "You already have one.");
	CHECK_EQ(app.g.cash, cash0);
}

static void test_store_no_cash_toast(void)
{
	open_shop(0, 50);
	press_seq(&ui, "A");			/* Pool Noodle is $80 */
	CHECK_STR(ui.toast, "Not enough cash.");
	CHECK(heard(SFX_DENY));
	CHECK_EQ(app.g.cash, 50);
	CHECK_EQ(app.g.weapon, -1);
}

static void test_store_text(void)
{
	StoreText t;
	int city, row;

	open_shop(0, 1000);
	store_text(&app.g, 0, &t);
	CHECK_STR(t.price, "$80");
	CHECK_STR(t.aim, "15%");
	CHECK_STR(t.status, "Aim in a chase: 15%. Replaces yours.");
	CHECK_EQ(t.status_col, PAL_DIM);
	CHECK_EQ(t.price_col, PAL_TEXT);
	store_text(&app.g, 3, &t);		/* sold in Houston */
	CHECK_STR(t.price, "HOUSTON");
	CHECK_STR(t.aim, "45%");
	CHECK_STR(t.status, "Only sold in Houston.");
	CHECK_EQ(t.status_col, PAL_CORAL);
	CHECK_EQ(t.price_col, PAL_FAINT);
	store_text(&app.g, 7, &t);		/* $15,000 and we have $1,000 */
	CHECK_STR(t.price, "$15,000");
	CHECK_EQ(t.price_col, PAL_CORAL);
	store_text(&app.g, 8, &t);
	CHECK_STR(t.aim, "+20");
	CHECK_STR(t.status, "Adds room for 20. Room items stack.");
	CHECK_EQ(t.price_col, PAL_TEXT);
	app.g.gear_owned[8] = 1;
	store_text(&app.g, 8, &t);
	CHECK_STR(t.status, "You have one. It adds room for 20.");
	CHECK_EQ(t.status_col, PAL_MINT);
	app.g.weapon = 1;
	store_text(&app.g, 1, &t);
	CHECK_STR(t.status, "You're packing it now.");
	CHECK_EQ(t.status_col, PAL_GOLD);
	/* every status line fits the 37 cells of the blurb box, in every city */
	for (city = 0; city < NUM_CITIES; city++) {
		app.g.loc.city = (uint8_t)city;
		for (row = 0; row < NUM_GEAR; row++) {
			store_text(&app.g, row, &t);
			CHECK(strlen(t.status) <= 37);
		}
	}
}

static void test_store_marks(void)
{
	open_shop(0, 10000);
	app.g.weapon = 1;
	app.g.gear_owned[9] = 1;
	ui.row = 5;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	/* the star at (298, 27 + 10 + 1) and the check at (298, 121 + 10 + 1): both draw something */
	{
		int x, y, star = 0, check = 0;

		for (y = 0; y < 8; y++)
			for (x = 0; x < 8; x++) {
				if (px(298 + x, 38 + y) != PAL_PANEL)
					star++;
				if (px(298 + x, 132 + y) != PAL_PANEL)
					check++;
			}
		CHECK(star > 0);
		CHECK(check > 0);
		CHECK_EQ(px(298 + 3, 28 + 20 + 3) == PAL_PANEL, 1);	/* row 2 has no mark */
	}
	/* the highlight bar of row 5 */
	CHECK_EQ(px(5, 27 + 50), PAL_HOT);
	CHECK_EQ(px(315, 27 + 50 + 8), PAL_HOT);
	CHECK_EQ(px(5, 27 + 40), PAL_PANEL);
}

static void test_store_click(void)
{
	int64_t cash0;

	open_shop(0, 10000);
	cash0 = app.g.cash;
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_STORE_ROW, 2));
	CHECK_EQ(ui.row, 2);
	CHECK_EQ(app.g.weapon, 2);
	CHECK_EQ(app.g.cash, cash0 - GEAR[2].price);
	click_hit(&ui, hit_find(&ui, HIT_STORE_ROW, 2));	/* stale: needs a redraw */
	CHECK_EQ(app.g.cash, cash0 - GEAR[2].price);
}

static void test_store_draws_everywhere(void)
{
	int city, row, cash;

	for (city = 0; city < NUM_CITIES; city++)
		for (cash = 0; cash < 2; cash++)
			for (row = 0; row < NUM_GEAR; row++) {
				open_shop(city, cash ? 1000000 : 0);
				ui.row = row;
				app.g.weapon = (int8_t)(row % 3);
				ui_draw(&ui, &cv);
				CHECK_EQ(cv.overflow, 0);
			}
}

static void test_heal(void)
{
	int64_t cash0;
	int hp0;

	ui_test_setup(&app, &ui, 30);
	app.g.health = 60;
	app.g.cash = 100000;
	cash0 = app.g.cash;
	hp0 = app.g.health;
	press_seq(&ui, "R1 R1 R R R R R A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_HEAL);
	CHECK_EQ(picker_max(&ui), 40);
	press_seq(&ui, "U U U U U A");
	CHECK_EQ(app.g.health, hp0 + 5);
	CHECK_EQ(app.g.cash, cash0 - 5 * app.g.p.hospital_per_hp);
	ui_test_setup(&app, &ui, 30);
	CHECK_EQ(app.g.health, 100);
	press_seq(&ui, "R1 R1 R R R R R A");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_STR(ui.toast, "You're in perfect health.");
}

static void test_select_charts_highlight(void)
{
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "D D D");			/* market row 3 */
	CHECK_EQ(ui.mrow, 4);
	press_seq(&ui, "U");
	CHECK_EQ(ui.mrow, 3);
	drain();
	press_seq(&ui, "SEL");
	CHECK_EQ(ui.scr, SCR_CHART);
	CHECK_EQ(ui.chart_good, 3);
	CHECK(heard(SFX_OK));
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
	app.g.stash[5] = 2;
	app.g.stash[7] = 4;
	app.g.stash_paid[5] = 100;
	app.g.stash_paid[7] = 100;
	press_seq(&ui, "R1 D");			/* the bag, second line: good 7 */
	CHECK_EQ(ui.panel, 1);
	press_seq(&ui, "SEL");
	CHECK_EQ(ui.scr, SCR_CHART);
	CHECK_EQ(ui.chart_good, 7);
	press_seq(&ui, "SEL");			/* SELECT also leaves, as in the mockup */
	CHECK_EQ(ui.scr, SCR_MARKET);
	press_seq(&ui, "R1");			/* the actions: SELECT charts the market row */
	press_seq(&ui, "SEL");
	CHECK_EQ(ui.scr, SCR_CHART);
	CHECK_EQ(ui.chart_good, ui.mrow);
}

static void test_chart_cycles(void)
{
	ui_test_setup(&app, &ui, 30);
	ui.mrow = 0;
	press_seq(&ui, "SEL");
	CHECK_EQ(ui.chart_good, 0);
	press_seq(&ui, "L");
	CHECK_EQ(ui.chart_good, 9);
	press_seq(&ui, "R1");
	CHECK_EQ(ui.chart_good, 0);
	press_seq(&ui, "R");
	CHECK_EQ(ui.chart_good, 1);
	press_seq(&ui, "L1");
	CHECK_EQ(ui.chart_good, 0);
	press_seq(&ui, "U D A X Y");	/* nothing here */
	CHECK_EQ(ui.scr, SCR_CHART);
	CHECK_EQ(ui.chart_good, 0);
}

static void test_chart_clicks(void)
{
	ui_test_setup(&app, &ui, 30);
	ui.mrow = 0;
	press_seq(&ui, "SEL");
	ui_draw(&ui, &cv);
	ui_click(&ui, 40, 100);			/* left half */
	CHECK_EQ(ui.chart_good, 9);
	ui_draw(&ui, &cv);
	ui_click(&ui, 280, 100);		/* right half */
	CHECK_EQ(ui.chart_good, 0);
	ui_draw(&ui, &cv);
	ui_click(&ui, 280, 5);			/* the top bar is not a region */
	CHECK_EQ(ui.chart_good, 0);
}

static void test_chart_draws_all_lengths(void)
{
	static const int LENS[4] = {15, 30, 60, 90};
	int k, m, good;

	for (k = 0; k < 4; k++) {
		ui_test_setup(&app, &ui, LENS[k]);
		for (m = 0; m < 6; m++) {
			Loc l = {0, (uint8_t)((m + 1) % HOODS_PER_CITY)};

			CHECK_EQ(game_move(&app.g, l), OK);
		}
		for (good = 0; good < NUM_GOODS; good++) {
			ui.chart_good = good;
			go(&ui, SCR_CHART);
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
		}
	}
	/* the last day of the longest game, holding a lot */
	app.g.stash[9] = 123456;
	app.g.stash_paid[9] = 9999999999LL;
	ui.chart_good = 9;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_chart_no_prices(void)
{
	ui_test_setup(&app, &ui, 30);
	memset(app.g.hist, 0, sizeof app.g.hist);
	ui.chart_good = 2;
	go(&ui, SCR_CHART);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(canvas_colors(&cv, 64) > 4);
}

static void test_kfmt(void)
{
	static const struct { int64_t v; const char *s; } T[] = {
		{0, "$0"}, {999, "$999"}, {1000, "$1K"}, {1200, "$1.2K"}, {1500, "$1.5K"},
		{33000, "$33K"}, {500000, "$500K"}, {999950, "$1M"}, {1000000, "$1M"},
		{1500000, "$1.5M"}, {20000000, "$20M"},
	};
	char b[24];
	size_t i;

	for (i = 0; i < sizeof T / sizeof T[0]; i++) {
		chart_k(T[i].v, b, sizeof b);
		CHECK_STR(b, T[i].s);
	}
}

static void test_nice_top(void)
{
	static const struct { int64_t v, top; } T[] = {
		{0, 1}, {1, 1}, {2, 2}, {7, 8}, {9, 10}, {10, 10}, {11, 15}, {1150, 1500}, {2000, 2000},
		{2001, 2500}, {33000, 40000}, {45000, 50000}, {70000, 80000}, {90000, 100000},
		{3300000, 4000000},
	};
	size_t i;

	for (i = 0; i < sizeof T / sizeof T[0]; i++)
		CHECK_EQ(chart_top(T[i].v), T[i].top);
}

static void test_chart_text(void)
{
	ChartText t;

	ui_test_setup(&app, &ui, 30);
	app.g.day = 3;
	chart_text(&app.g, 1, &t);
	CHECK_STR(t.title, "WEED PRICES");
	CHECK_STR(t.day, "DAY 3/30");
	CHECK_STR(t.range, "USUAL RANGE $300 TO $1.2K");
	CHECK_STR(t.have, "YOU HAVE NONE");
	app.g.stash[1] = 12;
	app.g.stash_paid[1] = 7056;
	chart_text(&app.g, 1, &t);
	CHECK_STR(t.have, "HAVE 12, PAID $588 EACH");	/* the average, and the longest line that fits */
	app.g.stash[1] = 3;
	app.g.stash_paid[1] = 1000;		/* 333.33 each */
	chart_text(&app.g, 1, &t);
	CHECK_STR(t.have, "HAVE 3, PAID $333 EACH");
	app.g.stash_paid[1] = 30;
	chart_text(&app.g, 1, &t);
	CHECK_STR(t.have, "YOU HAVE 3, PAID $10 EACH");	/* 25 cells: still the full line */
	app.g.stash_paid[1] = 1000;
	app.g.stash_paid[1] = 1001;		/* 333.67 rounds up like the sell picker */
	chart_text(&app.g, 1, &t);
	CHECK_STR(t.have, "HAVE 3, PAID $334 EACH");
	app.g.stash[1] = 150;
	app.g.stash_paid[1] = 150LL * 33000;
	chart_text(&app.g, 1, &t);
	CHECK_STR(t.have, "150 AT $33,000 EACH");
	chart_text(&app.g, 9, &t);
	CHECK_STR(t.title, "COCAINE PRICES");
	CHECK_STR(t.range, "USUAL RANGE $12.5K TO $33K");
}

/* Weed (300 to 1150) over 30 days, on day 3, with a spike on day 1, a crash on day 2 and
 * 700 today. The colors follow the recorded events, not the band: a 100 that was no crash
 * and a 2000 that was no spike are plain (checked below). Worked by hand from the mockup: top = niceTop(2000) = 2000, sx(d) = 56 + (d - 1)
 * * 254 / 29, sy(p) = 172 - p * 148 / 2000. */
static void test_chart_pixels(void)
{
	ui_test_setup(&app, &ui, 30);
	memset(app.g.hist, 0, sizeof app.g.hist);
	app.g.day = 3;
	app.g.hist[1][1] = 2000;
	app.g.hist[1][2] = 100;
	app.g.hist[1][3] = 700;
	app.g.hist_ev[1][1] = EV_SPIKE;
	app.g.hist_ev[1][2] = EV_CRASH;
	ui.chart_good = 1;
	go(&ui, SCR_CHART);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK_EQ(px(56, 24), PAL_HOT);		/* above the band: pink, 3x3 around (56, 24) */
	CHECK_EQ(px(55, 23), PAL_HOT);
	CHECK_EQ(px(57, 25), PAL_HOT);
	CHECK_EQ(px(65, 165), PAL_MINT);	/* below it: green */
	CHECK_EQ(px(74, 120), PAL_TEXT);	/* inside it: plain, with the gold ring around */
	CHECK_EQ(px(78, 120), PAL_GOLD);
	app.g.hist_ev[1][1] = EV_NONE;		/* the same prices, no events: all plain */
	app.g.hist_ev[1][2] = EV_NONE;
	ui_draw(&ui, &cv);
	CHECK_EQ(px(56, 24), PAL_TEXT);
	CHECK_EQ(px(65, 165), PAL_TEXT);
	app.g.hist_ev[1][1] = EV_SPIKE;
	app.g.hist_ev[1][2] = EV_CRASH;
	ui_draw(&ui, &cv);
	CHECK_EQ(px(200, 172), PAL_LINE);	/* the zero line */
	CHECK_EQ(px(201, 98), PAL_PANEL2);	/* the half line */
	CHECK_EQ(px(100, 100), PAL_PANEL2);	/* the band's dots: (x + y) even */
	CHECK_EQ(px(101, 100), PAL_PANEL);
	CHECK_EQ(px(101, 160), PAL_PANEL);	/* below the band (y 150) no dots */
	CHECK_EQ(px(100, 160), PAL_PANEL);
	CHECK_EQ(px(14, 191), PAL_DIM);	/* the legend's dotted swatch */
	CHECK_EQ(px(9, 204), PAL_HOT);
	CHECK_EQ(px(65, 204), PAL_MINT);
}

static void test_no_coming_soon_left(void)
{
	int act, i;

	for (act = 0; act < NUM_MKT_ACTS; act++) {
		ui_test_setup(&app, &ui, 30);
		app.g.health = 50;
		press_seq(&ui, "R1 R1");
		for (i = 0; i < act; i++)
			press_seq(&ui, "R");
		press_seq(&ui, "A");
		CHECK(strcmp(ui.toast, "Coming soon.") != 0);
		CHECK(!ui_stub(ui.scr));
	}
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "SEL");
	CHECK(strcmp(ui.toast, "Coming soon.") != 0);
	CHECK(!ui_stub(SCR_STORE));
	CHECK(!ui_stub(SCR_CHART));
}

void tests_ui_store(void)
{
	test_store_opens_and_draws();
	test_store_navigation();
	test_store_buy_weapon();
	test_store_room_stacks();
	test_store_other_city_toast();
	test_store_owned_toast();
	test_store_no_cash_toast();
	test_store_text();
	test_store_marks();
	test_store_click();
	test_store_draws_everywhere();
	test_heal();
	test_select_charts_highlight();
	test_chart_cycles();
	test_chart_clicks();
	test_chart_draws_all_lengths();
	test_chart_no_prices();
	test_kfmt();
	test_nice_top();
	test_chart_text();
	test_chart_pixels();
	test_no_coming_soon_left();
}
