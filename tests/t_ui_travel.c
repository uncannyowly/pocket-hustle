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

static void test_fly_lists_other_cities(void)
{
	int i, seen[NUM_CITIES] = {0};

	open_fly();
	CHECK_EQ(ui.scr, SCR_FLY);
	CHECK_EQ(ui.row, 0);
	for (i = 0; i < 6; i++) {
		int city = fly_city(&app.g, i);

		CHECK(city >= 0 && city < NUM_CITIES);
		CHECK(city != app.g.loc.city);
		seen[city]++;
	}
	for (i = 0; i < NUM_CITIES; i++)
		CHECK_EQ(seen[i], i == app.g.loc.city ? 0 : 1);
	press_seq(&ui, "U");
	CHECK_EQ(ui.row, 5);
	press_seq(&ui, "D D");
	CHECK_EQ(ui.row, 1);
	for (i = 0; i < 6; i++) {
		ui.row = i;
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
	}
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_fly_charges_and_lands(void)
{
	int64_t cash, fare;
	int city;

	open_fly();
	app.g.cash = 100000;
	cash = app.g.cash;
	city = fly_city(&app.g, 0);
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
	test_fly_lists_other_cities();
	test_fly_charges_and_lands();
	test_fly_too_poor();
	test_travel_clicks();
	test_move_on_last_day_ends();
	test_goons_kill_on_arrival();
	test_announced_spike_inside_range();
}
