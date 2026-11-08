/* News cards, the transit map and flights. Positions come from the mockup's drawNews,
 * drawMove and drawFly, worked out by hand. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "helpers.h"
#include "tests.h"
#include "ui_helpers.h"
#include "core/save.h"
#include "gfx/palette.h"
#include "sfx/sfx.h"
#include "ui/input.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

static App app;
static Ui ui;
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

/* A on every card until the news is over; returns the cards seen. */
static int through_news(void)
{
	int n = 0;

	while (ui.scr == SCR_NEWS && n < 40) {
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
		press_seq(&ui, "A");
		n++;
	}
	return n;
}

static void open_move(void)
{
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "R1 R1 A");
}

static void open_fly(void)
{
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "R1 R1 R A");
}

static void test_move_starts_on_current_hood(void)
{
	ui_test_setup(&app, &ui, 30);
	app.g.loc.hood = 4;
	press_seq(&ui, "R1 R1 A");
	CHECK_EQ(ui.scr, SCR_MOVE);
	CHECK_EQ(ui.row, 4);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_move_same_hood_toast(void)
{
	open_move();
	snap();
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_MOVE);
	CHECK_STR(ui.toast, "You're already here.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));
}

static void test_move_navigation(void)
{
	open_move();
	CHECK_EQ(ui.row, 0);
	press_seq(&ui, "U");
	CHECK_EQ(ui.row, 5);
	press_seq(&ui, "D D");
	CHECK_EQ(ui.row, 1);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_move_shows_news(void)
{
	int day;

	open_move();
	day = app.g.day;
	drain();
	press_seq(&ui, "D D A");
	CHECK_EQ(app.g.day, day + 1);
	CHECK_EQ(app.g.loc.hood, 2);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK(ui.news.n >= 1);
	CHECK_EQ(ui.news.cards[0].kind, CARD_TRAVEL);
	CHECK(strstr(ui.news.cards[0].text, CITIES[0].hoods[2]) != NULL);
	CHECK(heard(SFX_OK));
	CHECK(through_news() >= 1);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_EQ(app.g.phase, PH_TRADING);
}

static void test_news_click_advances_one(void)
{
	Msg m;

	ui_test_setup(&app, &ui, 30);
	memset(&m, 0, sizeof m);
	m.kind = M_RODE;
	m.a = 2;
	app.g.msgs[0] = m;
	m.kind = M_LAST_DAY;
	app.g.msgs[1] = m;
	app.g.nmsgs = 2;
	ui_collect_news(&ui, SCR_MARKET);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.n, 2);
	CHECK_EQ(ui.news.i, 0);
	ui_draw(&ui, &cv);
	ui_click(&ui, 100, 100);
	CHECK_EQ(ui.news.i, 1);
	CHECK_EQ(ui.scr, SCR_NEWS);
	ui_draw(&ui, &cv);
	ui_click(&ui, 5, 5);			/* anywhere */
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_news_kinds(void)
{
	static const struct { int kind; int card; } MAP[] = {
		{M_RODE, CARD_TRAVEL}, {M_FLEW, CARD_TRAVEL}, {M_SPIKE, CARD_MARKET}, {M_CRASH, CARD_MARKET},
		{M_GOONS, CARD_TROUBLE}, {M_MUGGED, CARD_TROUBLE}, {M_DEAD, CARD_TROUBLE}, {M_COPS, CARD_TROUBLE},
		{M_RUMOR_TIP, CARD_RUMOR}, {M_RUMOR_JOKE, CARD_RUMOR}, {M_LAST_DAY, CARD_DAY}, {M_GAME_OVER, CARD_DAY},
	};
	size_t i;

	for (i = 0; i < sizeof MAP / sizeof MAP[0]; i++) {
		Msg m;

		ui_test_setup(&app, &ui, 30);
		memset(&m, 0, sizeof m);
		m.kind = (uint8_t)MAP[i].kind;
		m.a = 3;
		m.amount = 1234;
		app.g.msgs[0] = m;
		app.g.nmsgs = 1;
		ui_collect_news(&ui, SCR_MARKET);
		CHECK_EQ(ui.scr, SCR_NEWS);
		CHECK_EQ(ui.news.n, 1);
		CHECK_EQ(ui.news.cards[0].kind, MAP[i].card);
		CHECK(ui.news.cards[0].text[0] != '\0');
		CHECK_EQ(ui.news.cards[0].good, MAP[i].card == CARD_MARKET ? 3 : -1);
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
	}
	ui_test_setup(&app, &ui, 30);		/* nothing to show, or nothing worth a card */
	app.g.nmsgs = 0;
	ui_collect_news(&ui, SCR_MARKET);
	CHECK_EQ(ui.scr, SCR_MARKET);
	app.g.msgs[0].kind = M_ESCAPED;
	app.g.nmsgs = 1;
	ui_collect_news(&ui, SCR_MARKET);
	CHECK_EQ(ui.scr, SCR_MARKET);
	ui_collect_news(&ui, SCR_CHASE);	/* none: straight to `after` */
	CHECK_EQ(ui.scr, SCR_CHASE);
}

static void test_after_action_routing(void)
{
	Msg m;

	ui_test_setup(&app, &ui, 30);
	memset(&m, 0, sizeof m);
	m.kind = M_COPS;
	app.g.msgs[0] = m;
	app.g.nmsgs = 1;
	app.g.phase = PH_CHASE;
	ui_after_action(&ui, app.g.day, PH_TRADING);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.after, SCR_CHASE);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_CHASE);
}

static void test_news_draws_each_kind(void)
{
	int k;

	for (k = 0; k < 5; k++) {
		Msg m;
		static const int KIND[5] = {M_RODE, M_SPIKE, M_GOONS, M_RUMOR_JOKE, M_LAST_DAY};

		ui_test_setup(&app, &ui, 30);
		memset(&m, 0, sizeof m);
		m.kind = (uint8_t)KIND[k];
		m.a = 1;
		app.g.msgs[0] = m;
		app.g.nmsgs = 1;
		app.g.price[1] = 999;
		ui_collect_news(&ui, SCR_MARKET);
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
		CHECK_EQ(px(20, 36), k == 0 ? PAL_SKY : k == 1 ? PAL_GOLD : k == 2 ? PAL_CORAL :
				     k == 3 ? PAL_MINT : PAL_LINE_HI);
		CHECK_EQ(px(30, 100), PAL_PANEL);
	}
}

static void test_news_advance_sounds(void)
{
	Msg m;

	ui_test_setup(&app, &ui, 30);
	memset(&m, 0, sizeof m);
	m.kind = M_RODE;
	app.g.msgs[0] = m;
	m.kind = M_LAST_DAY;
	app.g.msgs[1] = m;
	app.g.nmsgs = 2;
	ui_collect_news(&ui, SCR_MARKET);
	drain();
	press_seq(&ui, "L");			/* only A and B move the news */
	CHECK_EQ(ui.news.i, 0);
	press_seq(&ui, "A");
	CHECK_EQ(ui.news.i, 1);
	CHECK(heard(SFX_MOVE));
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_move_autosaves(void)
{
	char dir[64];
	Game loaded;
	Params p = test_params();

	system("mkdir -p build");
	snprintf(dir, sizeof dir, "build/ui-travel-XXXXXX");
	if (!mkdtemp(dir)) {
		CHECK(0);
		return;
	}
	CHECK_EQ(app_init(&app, dir), 0);
	new_test_game(&app.g, &p, 30);
	app.has_game = 1;
	ui_init(&ui, &app);
	go(&ui, SCR_MARKET);
	press_seq(&ui, "R1 R1 A D A");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(save_read(&loaded, app.save_path), 0);
	CHECK_EQ(loaded.day, 2);
	CHECK_EQ(loaded.loc.hood, 1);
	CHECK_EQ(app.io_failed, 0);
	{
		char cmd[96];

		snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
		system(cmd);
	}
}

static void test_fly_charges_and_lands(void)
{
	int64_t cash, fare;
	int city;

	open_fly();
	app.g.cash = 100000;
	cash = app.g.cash;
	city = fly_city(&ui, 0);
	fare = game_flight_cost(&app.g, city);
	CHECK(fare > 0);
	press_seq(&ui, "A");
	CHECK_EQ(app.g.loc.city, city);
	CHECK_EQ(app.g.loc.hood, 0);
	CHECK_EQ(app.g.cash, cash - fare);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.cards[0].kind, CARD_TRAVEL);
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_fly_too_poor(void)
{
	open_fly();
	app.g.cash = 0;
	snap();
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_FLY);
	CHECK_STR(ui.toast, "You can't afford the ticket.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));
}

static void test_travel_clicks(void)
{
	open_move();
	ui_draw(&ui, &cv);
	CHECK(hit_find(&ui, HIT_MOVE_ROW, 3) >= 0);
	click_hit(&ui, hit_find(&ui, HIT_MOVE_ROW, 3));
	CHECK_EQ(app.g.loc.hood, 3);
	CHECK_EQ(ui.scr, SCR_NEWS);
	through_news();

	ui.panel = 2;
	ui.act = MKT_FLY;
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_FLY);
	app.g.cash = 100000;
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_FLY_ROW, 2));
	CHECK(app.g.loc.city != 0);
	CHECK_EQ(app.g.loc.city, 3);		/* OKC's USA tab, third row: New York City */
	CHECK_EQ(app.g.loc.hood, 0);
	CHECK_EQ(ui.scr, SCR_NEWS);
}

static void test_move_on_last_day_ends(void)
{
	open_move();
	app.g.day = app.g.last_day;
	CHECK_EQ(app.scores.n[1], 0);
	press_seq(&ui, "D A");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.cards[0].kind, CARD_DAY);
	CHECK_EQ(through_news(), 1);
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(app.g.phase, PH_OVER);
	CHECK_EQ(app.scores.n[1], 1);
	CHECK_EQ(app.recorded, 1);
	CHECK_EQ(app.has_game, 0);
}

static void test_goons_kill_on_arrival(void)
{
	open_move();
	app.g.p.enforce_days = 8;
	app.g.health = 1;
	app.g.debt[0] = 5000;
	app.g.debt_age[0] = 20;
	press_seq(&ui, "D A");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.cards[0].kind, CARD_TRAVEL);
	CHECK_EQ(ui.news.cards[1].kind, CARD_TROUBLE);
	CHECK_EQ(app.g.phase, PH_OVER);
	through_news();
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(app.scores.n[1], 1);
	CHECK_EQ(app.has_game, 0);
}

/* The market colors today's prices by the core's record of what the roll applied, so a spike
 * inside the usual range still shows, and a high price that was not one does not. */
static void test_announced_spike_inside_range(void)
{
	int y;

	ui_test_setup(&app, &ui, 30);		/* OKC Weed usually 180..690 */
	app.g.price[1] = 400;
	app.g.price[5] = 3000;			/* Acid, usually 1800..5000 */
	app.g.price[4] = 9999999;		/* far above any range, but nobody's spike */
	CHECK_EQ(price_flag(&app.g, 1), PF_NORMAL);
	CHECK_EQ(price_flag(&app.g, 4), PF_NORMAL);
	app.g.hist_ev[1][app.g.day] = EV_SPIKE;
	app.g.hist_ev[5][app.g.day] = EV_CRASH;
	CHECK_EQ(price_flag(&app.g, 1), PF_SPIKE);
	CHECK_EQ(price_flag(&app.g, 5), PF_CRASH);
	CHECK_EQ(price_flag(&app.g, 4), PF_NORMAL);
	ui.mrow = 0;				/* the highlighted row draws its arrow white */
	ui_draw(&ui, &cv);
	y = 96 + 1 * 10;
	CHECK_EQ(px(160, y + 1), PAL_HOT);	/* the spike's arrow tip */
	y = 96 + 5 * 10;
	CHECK_EQ(px(160, y + 1 + 0), PAL_MINT);
	y = 96 + 4 * 10;
	CHECK(px(160, y + 1) != PAL_HOT && px(160, y + 1) != PAL_MINT);
	app.g.day++;				/* tomorrow's roll has recorded nothing yet */
	CHECK_EQ(price_flag(&app.g, 1), PF_NORMAL);
	CHECK_EQ(price_flag(&app.g, 5), PF_NORMAL);
}

static void test_line_colors_distinct(void)
{
	uint32_t col[NUM_CITIES];
	int city;

	for (city = 0; city < NUM_CITIES; city++) {
		ui_test_setup(&app, &ui, 30);
		app.g.loc.city = (uint8_t)city;
		app.g.loc.hood = 0;
		press_seq(&ui, "R1 R1 A");
		CHECK_EQ(ui.scr, SCR_MOVE);
		ui_draw(&ui, &cv);
		col[city] = px(23, 110);	/* the line between the first two stops */
		CHECK(col[city] != PAL_PANEL && col[city] != 0);
		if (city > 0)
			CHECK(col[city] != col[city - 1]);
	}
	for (city = 0; city < NUM_CITIES; city++) {
		int k;

		for (k = city + 1; k < NUM_CITIES; k++)
			CHECK(col[city] != col[k]);
	}
}


/* ---- the two-tab flight screen ---- */

static unsigned heard_mask(void)
{
	int s;
	unsigned m = 0;

	while ((s = ui_take_sfx(&ui)) >= 0)
		m |= 1u << s;
	return m;
}

static void open_fly_from(int city)
{
	ui_test_setup(&app, &ui, 30);
	app.g.loc.city = (uint8_t)city;
	app.g.loc.hood = 0;
	app.g.cash = 100000;
	press_seq(&ui, "R1 R1 R A");
	CHECK_EQ(ui.scr, SCR_FLY);
}

static int tab_cities(int origin, int tab)
{
	int c, n = 0;

	for (c = 0; c < NUM_CITIES; c++)
		if (c != origin && CITIES[c].country == tab)
			n++;
	return n;
}

static void test_fly_opens_on_current_country(void)
{
	int city;

	for (city = 0; city < NUM_CITIES; city++) {
		open_fly_from(city);
		CHECK_EQ(ui.fly_tab, CITIES[city].country);
		CHECK_EQ(ui.row, 0);
	}
}

static void test_fly_grid_lists_the_tab(void)
{
	int origin, tab, i;

	for (origin = 0; origin < NUM_CITIES; origin++)
		for (tab = 0; tab < 2; tab++) {
			int n, prev = -1;

			open_fly_from(origin);
			ui.fly_tab = tab;
			ui.row = 0;
			n = fly_count(&ui);
			CHECK_EQ(n, tab_cities(origin, tab));
			CHECK(n >= 3 && n <= 14);
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
			CHECK_EQ(ui.hits_dropped, 0);
			for (i = 0; i < n; i++) {
				int city = fly_city(&ui, i), h = hit_find(&ui, HIT_FLY_ROW, i);

				CHECK(city > prev);		/* table order */
				CHECK(city != origin);
				CHECK_EQ(CITIES[city].country, tab);
				prev = city;
				CHECK(h >= 0);
				if (h >= 0) {
					CHECK_EQ(ui.hits[h].x < 160, i < 7);	/* two columns of up to seven */
					CHECK(ui.hits[h].y + ui.hits[h].h <= 168);
				}
			}
			CHECK_EQ(hit_find(&ui, HIT_FLY_ROW, n), -1);
			CHECK_EQ(fly_city(&ui, n), -1);
			CHECK(hit_find(&ui, HIT_FLY_TAB, 0) >= 0);
			CHECK(hit_find(&ui, HIT_FLY_TAB, 1) >= 0);
		}
}

static void test_fly_tabs_switch(void)
{
	open_fly_from(0);
	CHECK_EQ(ui.fly_tab, 0);
	ui.row = 5;
	drain();
	press_seq(&ui, "R1");
	CHECK_EQ(ui.fly_tab, 1);
	CHECK_EQ(ui.row, 0);
	CHECK(heard(SFX_MOVE));
	press_seq(&ui, "R1");
	CHECK_EQ(ui.fly_tab, 1);
	press_seq(&ui, "L1");
	CHECK_EQ(ui.fly_tab, 0);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_FLY_TAB, 1));
	CHECK_EQ(ui.fly_tab, 1);
	CHECK_EQ(ui.scr, SCR_FLY);
	CHECK_EQ(app.g.loc.city, 0);		/* a tab click flies nowhere */
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_FLY_TAB, 0));
	CHECK_EQ(ui.fly_tab, 0);
}

static void test_fly_dpad_columns(void)
{
	open_fly_from(0);			/* 13 cities: 7 + 6 */
	CHECK_EQ(fly_count(&ui), 13);
	press_seq(&ui, "D");
	CHECK_EQ(ui.row, 1);
	press_seq(&ui, "U U");
	CHECK_EQ(ui.row, 6);			/* wraps inside the column */
	press_seq(&ui, "R");
	CHECK_EQ(ui.row, 12);			/* row 6 of the short column is missing: its last */
	press_seq(&ui, "L");
	CHECK_EQ(ui.row, 5);
	press_seq(&ui, "R");
	CHECK_EQ(ui.row, 12);
	press_seq(&ui, "D");
	CHECK_EQ(ui.row, 7);			/* wraps inside the right column */
	press_seq(&ui, "L");
	CHECK_EQ(ui.row, 0);
	press_seq(&ui, "R1");			/* four cities, one column */
	CHECK_EQ(fly_count(&ui), 4);
	press_seq(&ui, "R");
	CHECK_EQ(ui.row, 0);
	press_seq(&ui, "U");
	CHECK_EQ(ui.row, 3);
	press_seq(&ui, "D");
	CHECK_EQ(ui.row, 0);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

/* Every destination can be reached by the D-pad and by a click, from every city. */
static void test_fly_everything_reachable(void)
{
	int origin, tab;

	for (origin = 0; origin < NUM_CITIES; origin++)
		for (tab = 0; tab < 2; tab++) {
			int n, i, seen[14] = {0}, stack[14], sp = 0, reached = 0;
			static const int PADS[4] = {PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT};

			open_fly_from(origin);
			ui.fly_tab = tab;
			n = fly_count(&ui);
			stack[sp++] = 0;
			seen[0] = 1;
			while (sp) {
				int at = stack[--sp], k;

				reached++;
				for (k = 0; k < 4; k++) {
					ui.row = at;
					ui_press(&ui, PADS[k]);
					if (ui.row >= 0 && ui.row < n && !seen[ui.row]) {
						seen[ui.row] = 1;
						stack[sp++] = ui.row;
					}
				}
			}
			CHECK_EQ(reached, n);
			ui.row = 0;
			ui_draw(&ui, &cv);
			for (i = 0; i < n; i++) {
				CHECK(hit_find(&ui, HIT_FLY_ROW, i) >= 0);
			}
		}
}

static void test_fly_a_flies_and_clicks_fly(void)
{
	int64_t cash, fare;

	open_fly_from(0);
	press_seq(&ui, "R1");
	ui_draw(&ui, &cv);
	cash = app.g.cash;
	fare = game_flight_cost(&app.g, 14);
	CHECK_EQ(fly_city(&ui, 0), 14);
	press_seq(&ui, "A");
	CHECK_EQ(app.g.loc.city, 14);
	CHECK_EQ(app.g.cash, cash - fare);
	CHECK(fare >= app.g.p.intl_flight_lo && fare <= app.g.p.intl_flight_hi);
	through_news();
	open_fly_from(0);
	ui.fly_tab = 1;
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_FLY_ROW, 3));
	CHECK_EQ(app.g.loc.city, 17);
}

static void test_fly_boarding_pass_text(void)
{
	PassText t;

	open_fly_from(0);
	fly_pass_text(&ui, 1, &t);
	CHECK(strstr(t.route, "OKC") && strstr(t.route, "IAH"));
	CHECK_STR(t.border, "");
	CHECK(strstr(t.lands, "LANDS") != NULL);
	CHECK(strstr(t.passenger, "TEST") != NULL);
	CHECK(t.fare[0] == '$');
	fly_pass_text(&ui, 14, &t);
	CHECK_STR(t.border, "BORDER CROSSING");
	CHECK(strstr(t.route, "TIJ") != NULL);
	open_fly_from(14);
	fly_pass_text(&ui, 15, &t);		/* Mexico to Mexico is no crossing */
	CHECK_STR(t.border, "");
	fly_pass_text(&ui, 0, &t);		/* and back across is one */
	CHECK_STR(t.border, "BORDER CROSSING");
	/* the pass draws a crossing line only for a crossing */
	ui.fly_tab = 1;
	ui.row = 0;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	{
		Canvas other;
		int x, y, diff = 0;

		ui.fly_tab = 0;
		ui.row = 0;
		ui_draw(&ui, &other);
		ui.fly_tab = 1;
		ui_draw(&ui, &cv);
		for (y = 168; y < 222; y++)
			for (x = 8; x < 232; x++)
				diff += cv.px[y * CV_W + x] != other.px[y * CV_W + x];
		CHECK(diff > 0);
	}
}

static void test_fly_banned_city(void)
{
	PassText t;
	char want[64];

	open_fly_from(0);
	app.g.banned |= 1u << 1;		/* Houston */
	ui.row = 0;
	CHECK_EQ(fly_city(&ui, 0), 1);
	fly_pass_text(&ui, 1, &t);
	CHECK_STR(t.fare, "BANNED");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	snap();
	drain();
	press_seq(&ui, "A");
	snprintf(want, sizeof want, "You're not welcome in %s anymore.", CITIES[1].name);
	CHECK_STR(ui.toast, want);
	CHECK_EQ(ui.scr, SCR_FLY);
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));
	press_seq(&ui, "D A");		/* the next city still flies */
	CHECK_EQ(app.g.loc.city, 2);
}

static void test_fly_unaffordable_fare_is_coral(void)
{
	int h, x, y, coral;

	open_fly_from(0);
	app.g.cash = 0;
	ui.row = 0;
	ui_draw(&ui, &cv);
	h = hit_find(&ui, HIT_FLY_ROW, 1);	/* not the highlighted row */
	CHECK(h >= 0);
	coral = 0;
	for (y = ui.hits[h].y; y < ui.hits[h].y + ui.hits[h].h; y++)
		for (x = ui.hits[h].x; x < ui.hits[h].x + ui.hits[h].w; x++)
			coral += cv.px[y * CV_W + x] == PAL_CORAL;
	CHECK(coral > 20);
	app.g.cash = 1000000;
	ui_draw(&ui, &cv);
	coral = 0;
	for (y = ui.hits[h].y; y < ui.hits[h].y + ui.hits[h].h; y++)
		for (x = ui.hits[h].x; x < ui.hits[h].x + ui.hits[h].w; x++)
			coral += cv.px[y * CV_W + x] == PAL_CORAL;
	CHECK_EQ(coral, 0);
}

static void test_fly_never_off_canvas(void)
{
	int origin, tab, i, mode;

	for (origin = 0; origin < NUM_CITIES; origin++)
		for (mode = 0; mode < 3; mode++) {
			open_fly_from(origin);
			app.g.cash = mode == 1 ? 0 : MONEY_MAX;
			if (mode == 2)
				app.g.banned = 0x3ffffu & ~(1u << origin);
			for (tab = 0; tab < 2; tab++) {
				ui.fly_tab = tab;
				for (i = 0; i < fly_count(&ui); i++) {
					ui.row = i;
					ui_draw(&ui, &cv);
					CHECK_EQ(cv.overflow, 0);
					CHECK_EQ(ui.hits_dropped, 0);
				}
			}
		}
}

static void test_title_cycle_visits_every_skyline(void)
{
	int seen[NUM_CITIES] = {0}, f, i;

	ui_test_setup(&app, &ui, 30);
	for (f = 0; f < NUM_CITIES * 80; f += 80) {
		ui.frame = (uint32_t)f;
		seen[title_city(&ui)]++;
		go(&ui, SCR_TITLE);
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
	}
	for (i = 0; i < NUM_CITIES; i++)
		CHECK_EQ(seen[i], 1);
}

/* ---- offers: the chemical and the crew ---- */

static void offer_cards(int kind, int chem)
{
	Msg m;

	ui_test_setup(&app, &ui, 30);
	memset(&m, 0, sizeof m);
	app.g.offer.kind = (uint8_t)kind;
	app.g.offer.chem = (int8_t)chem;
	app.g.offer.day = app.g.day;
	m.kind = kind == OFFER_RC ? M_RC_OFFER : M_CREW_OFFER;
	m.a = (int16_t)chem;
	app.g.msgs[0] = m;
	app.g.nmsgs = 1;
	ui_collect_news(&ui, SCR_MARKET);
}

static void crew_ready(int od_permille)
{
	offer_cards(OFFER_CREW, 0);
	app.g.p.cohort_od_permille = od_permille;
	app.g.p.cohort_min_stash = 1;
	app.g.stash[0] = 20;
	app.g.stash_paid[0] = 2000;
	app.g.cash = 1000;
}

static void test_rc_offer_card(void)
{
	offer_cards(OFFER_RC, 3);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.n, 1);
	CHECK_EQ(ui.news.cards[0].choice, CHOICE_RC);
	CHECK(strstr(ui.news.cards[0].text, CHEMS[3].name) != NULL);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK_EQ(px(20, 36), PAL_HOT);		/* the card's header has its own color */
	CHECK_EQ(ui.nhits, 2);			/* exactly the two choices, no click-anywhere */
	CHECK(hit_find(&ui, HIT_PAD, PAD_A) >= 0);
	CHECK(hit_find(&ui, HIT_PAD, PAD_B) >= 0);
	{
		char a[24], b[24];

		news_choice_label(&ui, 1, a, sizeof a);
		news_choice_label(&ui, 2, b, sizeof b);
		CHECK_STR(a, "BACK IT");
		CHECK_STR(b, "PASS");
	}
}

static void test_rc_card_stays_until_answered(void)
{
	offer_cards(OFFER_RC, 3);
	press_seq(&ui, "L R U D X Y L1 R1");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_RC);
	CHECK_EQ(ui.news.i, 0);
}

static void test_rc_back_it(void)
{
	int64_t cash;

	offer_cards(OFFER_RC, 3);
	app.g.cash = 1500;
	cash = app.g.cash;
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_STAKE);
	CHECK_EQ(picker_max(&ui), 1500);
	{
		PickText t;

		picker_text(&ui, &t);
		CHECK(strstr(t.title, CHEMS[3].name) != NULL || strstr(t.title, "3-MMC") != NULL);
		CHECK(strlen(t.title) + strlen(t.unit) + 2 <= 30);
		CHECK(strlen(t.total) <= 30 && strlen(t.info) <= 30);
	}
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "Y");
	CHECK_EQ(ui.pk.q, 1500);
	press_seq(&ui, "X");
	ui.pk.q = 500;
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(app.g.cash, cash - 500);
	CHECK_EQ(app.g.nstakes, 1);
	CHECK_EQ(app.g.stakes[0].stake, 500);
	CHECK_EQ(app.g.stakes[0].chem, 3);
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(ui.toast[0] != '\0');
}

static void test_rc_picker_back_returns_to_the_card(void)
{
	offer_cards(OFFER_RC, 3);
	press_seq(&ui, "A B");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_RC);
	CHECK_EQ(app.g.nstakes, 0);
	press_seq(&ui, "A A");			/* a stake of nothing is a way back too */
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_RC);
	ui_draw(&ui, &cv);
	CHECK_EQ(ui.nhits, 2);
}

static void test_rc_pass(void)
{
	int64_t cash;

	offer_cards(OFFER_RC, 3);
	cash = app.g.cash;
	drain();
	press_seq(&ui, "B");
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(app.g.cash, cash);
	CHECK_EQ(app.g.nstakes, 0);
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(heard(SFX_BACK));
}

static void test_rc_offer_clicks(void)
{
	offer_cards(OFFER_RC, 3);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_PAD, PAD_A));
	CHECK_EQ(ui.scr, SCR_PICKER);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_PAD, PAD_B));	/* the picker's Back */
	CHECK_EQ(ui.scr, SCR_NEWS);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_PAD, PAD_B));
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_rc_no_cash(void)
{
	offer_cards(OFFER_RC, 3);
	app.g.cash = 0;
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(picker_max(&ui), 0);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_RC);
}

static void test_offer_among_other_cards(void)
{
	Msg m;

	offer_cards(OFFER_RC, 3);
	memset(&m, 0, sizeof m);
	m.kind = M_RODE;
	app.g.msgs[1] = app.g.msgs[0];
	app.g.msgs[0] = m;
	m.kind = M_RC_FIND;
	m.a = 1;
	m.amount = 500;
	app.g.msgs[2] = m;
	app.g.nmsgs = 3;
	ui_collect_news(&ui, SCR_MARKET);
	CHECK_EQ(ui.news.n, 3);
	CHECK_EQ(ui.news.cards[0].choice, CHOICE_NONE);
	CHECK_EQ(ui.news.cards[1].choice, CHOICE_RC);
	press_seq(&ui, "A");
	CHECK_EQ(ui.news.i, 1);
	press_seq(&ui, "B");			/* pass: the find card is still to come */
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.n, 1);
	CHECK_EQ(ui.news.cards[0].kind, CARD_MARKET);
	CHECK(strstr(ui.news.cards[0].text, CHEMS[1].name) != NULL);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_offer_card_without_an_open_offer_is_skipped(void)
{
	Msg m;

	ui_test_setup(&app, &ui, 30);
	memset(&m, 0, sizeof m);
	m.kind = M_CREW_OFFER;
	app.g.msgs[0] = m;
	app.g.nmsgs = 1;
	ui_collect_news(&ui, SCR_MARKET);
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_crew_card_is_sober(void)
{
	char a[24], b[24];

	crew_ready(0);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.cards[0].kind, CARD_TROUBLE);
	CHECK_EQ(ui.news.cards[0].choice, CHOICE_CREW);
	CHECK(strchr(ui.news.cards[0].text, '!') == NULL);
	news_choice_label(&ui, 1, a, sizeof a);
	news_choice_label(&ui, 2, b, sizeof b);
	CHECK_STR(a, "DO IT");
	CHECK_STR(b, "WALK AWAY");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK_EQ(ui.nhits, 2);
	CHECK_EQ(px(20, 36), PAL_CORAL);
}

static void test_crew_paid(void)
{
	int64_t cash;
	unsigned m;
	int i;

	crew_ready(0);
	cash = app.g.cash;
	drain();
	press_seq(&ui, "A L A");
	m = heard_mask();
	CHECK(m & (1u << SFX_OK));
	CHECK(!(m & (1u << SFX_CASH)));
	CHECK(!(m & (1u << SFX_HURT)));
	CHECK(app.g.cash > cash);
	CHECK_EQ(app.g.stash[0], 0);
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.n, 1);
	CHECK_EQ(ui.news.cards[0].kind, CARD_TROUBLE);
	CHECK_EQ(ui.news.cards[0].choice, CHOICE_NONE);
	CHECK(strchr(ui.news.cards[0].text, '!') == NULL);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	for (i = 0; i < 1; i++)
		press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_crew_overdose(void)
{
	int old;
	int16_t day;
	unsigned m;

	crew_ready(1000);
	old = app.g.loc.city;
	day = app.g.day;
	drain();
	press_seq(&ui, "A L A");
	m = heard_mask();
	CHECK(!(m & (1u << SFX_CASH)));
	CHECK(game_city_banned(&app.g, old));
	CHECK(app.g.loc.city != old);
	CHECK_EQ(app.g.day, day + 1);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.cards[0].kind, CARD_TROUBLE);
	CHECK(strstr(ui.news.cards[0].text, CITIES[old].name) != NULL);
	CHECK(strchr(ui.news.cards[0].text, '!') == NULL);
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_crew_overdose_on_the_last_day_ends_the_game(void)
{
	crew_ready(1000);
	app.g.day = app.g.last_day;
	press_seq(&ui, "A L A");
	CHECK_EQ(app.g.phase, PH_OVER);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.after, SCR_OVER);
	through_news();
	CHECK_EQ(ui.scr, SCR_OVER);
}

static void test_crew_walk_away(void)
{
	int64_t cash;
	unsigned m;

	crew_ready(1000);
	cash = app.g.cash;
	drain();
	press_seq(&ui, "B");
	m = heard_mask();
	CHECK(m & (1u << SFX_BACK));
	CHECK(!(m & (1u << SFX_CASH)));
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(app.g.stash[0], 20);
	CHECK_EQ(app.g.cash, cash);
	CHECK_EQ(app.g.banned, 0);
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_crew_clicks(void)
{
	crew_ready(0);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_PAD, PAD_B));
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(app.g.stash[0], 20);
	crew_ready(0);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_PAD, PAD_A));
	CHECK_EQ(app.g.stash[0], 20);		/* a click on DO IT only asks */
	CHECK(ui.news.ask);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 1));
	CHECK_EQ(app.g.stash[0], 0);
	crew_ready(0);				/* a click on NO goes back to the same card */
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_PAD, PAD_A));
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 0));
	CHECK(!ui.news.ask);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_CREW);
	CHECK_EQ(app.g.stash[0], 20);
}

static void test_crew_stash_gone(void)
{
	crew_ready(0);
	app.g.stash[0] = 0;
	snap();
	drain();
	press_seq(&ui, "A L A");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_CREW);
	CHECK(ui.toast[0] != '\0');
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_crew_confirm(void)
{
	int64_t cash;
	unsigned m;

	crew_ready(0);
	cash = app.g.cash;
	drain();
	press_seq(&ui, "A");			/* DO IT asks first */
	CHECK(ui.news.ask);
	CHECK_EQ(ui.news.yes, 0);		/* NO is where it opens */
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.stash[0], 20);
	CHECK_EQ(app.g.offer.kind, OFFER_CREW);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK_STR(news_ask_text(&ui), "Hand your whole stash to the crew?");
	CHECK_STR(prompt_label(1, 0), "YES");		/* plain words on the crew prompt, glyphs elsewhere */
	CHECK_STR(prompt_label(0, 0), "NO");
	CHECK_STR(prompt_label(1, 1), "A YES");
	CHECK_STR(prompt_label(0, 1), "B NO");
	{
		const Hint *h;

		CHECK_EQ(news_ask_hints(&ui, &h), 2);
		CHECK_STR(h[0].b, "A");
		CHECK_STR(h[0].label, "Choose");
		CHECK_STR(h[1].b, "B");
		CHECK_STR(h[1].label, "Back");
	}
	m = heard_mask();
	CHECK(!(m & (1u << SFX_CASH)));
	press_seq(&ui, "A");			/* A on the default NO: back to the card */
	CHECK(!ui.news.ask);
	{
		const Hint *h;

		CHECK_EQ(news_ask_hints(&ui, &h), 0);
	}
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(ui.news.cards[0].choice, CHOICE_CREW);
	CHECK_EQ(app.g.offer.kind, OFFER_CREW);
	CHECK_EQ(app.g.stash[0], 20);
	CHECK_EQ(app.g.cash, cash);
	press_seq(&ui, "A R");			/* moving off YES and back: R is NO */
	CHECK_EQ(ui.news.yes, 0);
	press_seq(&ui, "L");
	CHECK_EQ(ui.news.yes, 1);
	press_seq(&ui, "B");			/* B is NO whatever is highlighted */
	CHECK(!ui.news.ask);
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.offer.kind, OFFER_CREW);
	CHECK_EQ(app.g.stash[0], 20);
	press_seq(&ui, "A");			/* it reopens on NO again */
	CHECK_EQ(ui.news.yes, 0);
	press_seq(&ui, "L A");			/* YES then A hands it over */
	CHECK_EQ(app.g.stash[0], 0);
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK(!ui.news.ask);
	m = heard_mask();
	CHECK(!(m & (1u << SFX_CASH)));
}

static void test_crew_walk_away_still_b(void)
{
	crew_ready(1000);
	press_seq(&ui, "B");
	CHECK(!ui.news.ask);
	CHECK_EQ(app.g.offer.kind, OFFER_NONE);
	CHECK_EQ(app.g.stash[0], 20);
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_chemist_card_has_no_prompt(void)
{
	offer_cards(OFFER_RC, 3);
	press_seq(&ui, "A");
	CHECK(!ui.news.ask);
	CHECK_EQ(ui.scr, SCR_PICKER);
}

/* ---- the market's running wild cards ---- */

static void test_wild_line(void)
{
	char s[40];
	int i;

	ui_test_setup(&app, &ui, 30);
	bag_wild_line(&app.g, s, sizeof s);
	CHECK_STR(s, "");
	app.g.nstakes = 1;
	app.g.stakes[0].chem = 3;
	bag_wild_line(&app.g, s, sizeof s);
	CHECK_STR(s, "WILD: 3-MMC");
	app.g.nstakes = 2;
	app.g.stakes[1].chem = 0;
	bag_wild_line(&app.g, s, sizeof s);
	CHECK_STR(s, "WILD: 3-MMC, 2C-B");
	app.g.nstakes = 3;
	app.g.stakes[0].chem = 5;
	app.g.stakes[1].chem = 2;
	app.g.stakes[2].chem = 8;
	bag_wild_line(&app.g, s, sizeof s);
	CHECK(strlen(s) <= 17);
	CHECK(strncmp(s, "WILD: ", 6) == 0);
	app.g.stakes[0].chem = 0;
	app.g.stakes[1].chem = 3;
	app.g.stakes[2].chem = 3;
	bag_wild_line(&app.g, s, sizeof s);
	CHECK_STR(s, "WILD: 2C-B +2");		/* what did not fit is counted */
	for (i = 0; i < NUM_CHEMS; i++) {
		int j;

		for (j = 0; j < NUM_CHEMS; j++) {
			app.g.stakes[0].chem = (int8_t)i;
			app.g.stakes[1].chem = (int8_t)j;
			app.g.stakes[2].chem = (int8_t)((i + j) % NUM_CHEMS);
			bag_wild_line(&app.g, s, sizeof s);
			CHECK(strlen(s) <= 17);
			CHECK(strncmp(s, "WILD: ", 6) == 0);
		}
	}
}

static void test_wild_line_replaces_the_weapon_line(void)
{
	Canvas other;
	int x, y, diff = 0;

	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &other);
	app.g.nstakes = 2;
	app.g.stakes[0].chem = 3;
	app.g.stakes[1].chem = 0;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	for (y = 184; y < 196; y++)
		for (x = 170; x < 318; x++)
			diff += cv.px[y * CV_W + x] != other.px[y * CV_W + x];
	CHECK(diff > 0);
	/* no stake line leaks past the panel's right edge */
	for (y = 184; y < 196; y++)
		for (x = 318; x < CV_W; x++)
			CHECK_EQ(cv.px[y * CV_W + x], other.px[y * CV_W + x]);
}

void tests_ui_travel(void)
{
	test_move_starts_on_current_hood();
	test_move_same_hood_toast();
	test_move_navigation();
	test_move_shows_news();
	test_news_click_advances_one();
	test_news_kinds();
	test_after_action_routing();
	test_news_draws_each_kind();
	test_news_advance_sounds();
	test_move_autosaves();
	test_fly_opens_on_current_country();
	test_fly_grid_lists_the_tab();
	test_fly_tabs_switch();
	test_fly_dpad_columns();
	test_fly_everything_reachable();
	test_fly_a_flies_and_clicks_fly();
	test_fly_boarding_pass_text();
	test_fly_banned_city();
	test_fly_unaffordable_fare_is_coral();
	test_fly_never_off_canvas();
	test_title_cycle_visits_every_skyline();
	test_rc_offer_card();
	test_rc_card_stays_until_answered();
	test_rc_back_it();
	test_rc_picker_back_returns_to_the_card();
	test_rc_pass();
	test_rc_offer_clicks();
	test_rc_no_cash();
	test_offer_among_other_cards();
	test_offer_card_without_an_open_offer_is_skipped();
	test_crew_card_is_sober();
	test_crew_paid();
	test_crew_overdose();
	test_crew_overdose_on_the_last_day_ends_the_game();
	test_crew_walk_away();
	test_crew_clicks();
	test_crew_stash_gone();
	test_crew_confirm();
	test_crew_walk_away_still_b();
	test_chemist_card_has_no_prompt();
	test_wild_line();
	test_wild_line_replaces_the_weapon_line();
	test_fly_charges_and_lands();
	test_fly_too_poor();
	test_travel_clicks();
	test_move_on_last_day_ends();
	test_goons_kill_on_arrival();
	test_announced_spike_inside_range();
	test_line_colors_distinct();
}
