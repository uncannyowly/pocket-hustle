/* The cop chase screen. Text is read through the string builders; the screen itself is
 * checked for drawing without overflow and for what the buttons do. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "check.h"
#include "helpers.h"
#include "tests.h"
#include "ui_helpers.h"
#include "content/content.h"
#include "core/save.h"
#include "gfx/palette.h"
#include "sfx/sfx.h"
#include "ui/input.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

enum { SELFIE = 1 };	/* the Selfie Stick in GEAR: 25% to hit */

static App app;
static Ui ui;
static Canvas cv;
static Game before;

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

static int scores_total(const App *a)
{
	return a->scores.n[0] + a->scores.n[1] + a->scores.n[2] + a->scores.n[3];
}

static void through_news(void)
{
	int n = 0;

	while (ui.scr == SCR_NEWS && n++ < 40) {
		ui_draw(&ui, &cv);
		press_seq(&ui, "A");
	}
}

/* A game where the next arrival is a chase: cops certain, something in the stash. */
static void arm(int days)
{
	ui_test_setup(&app, &ui, days);
	app.g.p.cop_base_permille = 1000;
	app.g.stash[0] = 5;
	app.g.stash_paid[0] = 50;
}

/* Rides to the second hood and clicks through the cards onto the chase. */
static void start_chase(int days)
{
	arm(days);
	press_seq(&ui, "R1 R1 A D A");
	through_news();
	drain();
}

static void test_chase_after_news(void)
{
	arm(30);
	press_seq(&ui, "R1 R1 A D A");
	CHECK_EQ(ui.scr, SCR_NEWS);
	CHECK_EQ(app.g.phase, PH_CHASE);
	CHECK(ui.news.n >= 2);
	CHECK_EQ(ui.news.cards[ui.news.n - 1].kind, CARD_TROUBLE);
	through_news();
	CHECK_EQ(ui.scr, SCR_CHASE);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(!ui_stub(SCR_CHASE));
	CHECK_EQ(ui.row, 0);
}

static void test_run_and_escape(void)
{
	start_chase(30);
	app.g.p.run_permille = 1000;
	press_seq(&ui, "A");
	CHECK_EQ(app.g.phase, PH_TRADING);
	CHECK_EQ(ui.scr, SCR_CHASE);
	CHECK_EQ(ui.chase_done, CR_ESCAPED);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(hit_find(&ui, HIT_PAD, PAD_A) >= 0);
	press_seq(&ui, "A");
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_EQ(app.g.phase, PH_TRADING);
}

static void test_failed_run_stays(void)
{
	start_chase(30);
	app.g.p.run_permille = 0;
	app.g.p.cop_hit_permille = 1000;
	press_seq(&ui, "A");
	CHECK_EQ(app.g.phase, PH_CHASE);
	CHECK_EQ(ui.scr, SCR_CHASE);
	CHECK_EQ(ui.chase_done, CR_NONE);
	CHECK(app.g.health < 100);
	CHECK(strlen(ui.chase_log) > 0);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_fight_without_weapon(void)
{
	start_chase(30);
	app.g.weapon = -1;
	press_seq(&ui, "R");
	CHECK_EQ(ui.row, 1);
	memcpy(&before, &app.g, sizeof before);
	drain();
	press_seq(&ui, "A");
	CHECK_STR(ui.toast, "You need a weapon from the shop.");
	CHECK(same_game(&before, &app.g));
	CHECK_EQ(ui.scr, SCR_CHASE);
	CHECK(heard(SFX_DENY));
}

static void test_fight_with_weapon(void)
{
	start_chase(30);
	app.g.weapon = SELFIE;
	app.g.p.cop_hit_permille = 0;
	press_seq(&ui, "R A");
	CHECK(strlen(ui.chase_log) > 0);
	CHECK(app.g.chase.cops_left >= 1);
	CHECK_EQ(ui.scr, SCR_CHASE);
}

static void test_fight_wins(void)
{
	int64_t cash;

	start_chase(30);
	app.g.weapon = 7;			/* the T-Shirt Cannon; make it certain */
	app.g.chase.cops_left = 1;
	app.g.p.cop_hit_permille = 0;
	cash = app.g.cash;
	/* power is below 1000 so loop until the hit lands */
	press_seq(&ui, "R");
	while (app.g.phase == PH_CHASE && ui.chase_done == CR_NONE)
		press_seq(&ui, "A");
	CHECK_EQ(ui.chase_done, CR_WON);
	CHECK(app.g.cash > cash);
	press_seq(&ui, "A");
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_surrender_jail(void)
{
	int64_t cash;
	int i, day;

	start_chase(30);
	cash = app.g.cash;
	day = app.g.day;
	press_seq(&ui, "R R");
	CHECK_EQ(ui.row, 2);
	press_seq(&ui, "A");
	for (i = 0; i < NUM_GOODS; i++)
		CHECK_EQ(app.g.stash[i], 0);
	CHECK_EQ(app.g.cash, cash - cash * app.g.p.jail_fine_pct / 100);
	CHECK_EQ(app.g.day, day + 1);
	CHECK_EQ(ui.chase_done, CR_JAILED);
	CHECK_EQ(ui.scr, SCR_CHASE);
	press_seq(&ui, "A");
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_EQ(app.g.phase, PH_TRADING);
}

static void test_jail_morning_news(void)
{
	start_chase(30);
	app.g.p.spike_permille = 1000;
	press_seq(&ui, "R R A");
	press_seq(&ui, "A");
	/* the jail morning's market messages become cards when the market rolled any */
	if (app.g.nmsgs > 0 && ui.scr == SCR_NEWS)
		CHECK(ui.news.n >= 1);
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
}

static void test_death_in_chase_records_once(void)
{
	int n0;

	start_chase(30);
	n0 = scores_total(&app);
	app.g.health = 1;
	app.g.p.run_permille = 0;
	app.g.p.cop_hit_permille = 1000;
	press_seq(&ui, "A");
	CHECK_EQ(app.g.phase, PH_OVER);
	CHECK_EQ(ui.chase_done, CR_DEAD);
	CHECK_EQ(ui.scr, SCR_CHASE);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(scores_total(&app), n0 + 1);
}

static void test_jail_on_last_day_ends(void)
{
	int n0;

	start_chase(30);
	n0 = scores_total(&app);
	app.g.day = app.g.last_day;
	press_seq(&ui, "R R A");
	CHECK_EQ(app.g.phase, PH_OVER);
	CHECK_EQ(ui.chase_done, CR_JAILED);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(scores_total(&app), n0 + 1);
}

static void test_resume_mid_chase(void)
{
	static App b;
	static Ui ub;
	char dir[64];

	snprintf(dir, sizeof dir, "build/chase-test-XXXXXX");
	CHECK(mkdtemp(dir) != NULL);
	app_init(&app, dir);
	app_new_game(&app, 30, "Test");
	app.g.phase = PH_CHASE;
	app.g.chase.posse = 2;
	app.g.chase.cops_left = 3;
	CHECK_EQ(app_save_now(&app), 0);
	app_init(&b, dir);
	app_load(&b);
	ui_init(&ub, &b);
	CHECK(b.has_game);
	CHECK_EQ(ui_resume_screen(&ub), SCR_CHASE);
	b.g.phase = PH_TRADING;
	CHECK_EQ(ui_resume_screen(&ub), SCR_MARKET);
	{
		char cmd[96];

		snprintf(cmd, sizeof cmd, "rm -rf %s", dir);
		CHECK_EQ(system(cmd), 0);
	}
}

static void test_odds_line(void)
{
	char s[96];

	start_chase(30);
	app.g.weapon = SELFIE;
	chase_odds(&app.g, 1, s, sizeof s);
	CHECK(strstr(s, "25%") != NULL);
	app.g.weapon = -1;
	chase_odds(&app.g, 1, s, sizeof s);
	CHECK(strstr(s, "weapon") != NULL);
	app.g.p.run_permille = 550;
	chase_odds(&app.g, 0, s, sizeof s);
	CHECK(strstr(s, "55%") != NULL);
	chase_odds(&app.g, 2, s, sizeof s);
	CHECK(strstr(s, "25%") != NULL);
	app.g.weapon = SELFIE;
	press_seq(&ui, "R");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_header(void)
{
	char s[64];

	start_chase(30);
	app.g.chase.cops_left = 3;
	chase_title(&app.g, s, sizeof s);
	CHECK_STR(s, "SGT. BUZZKILL + 2 COPS");
	app.g.chase.cops_left = 2;
	chase_title(&app.g, s, sizeof s);
	CHECK_STR(s, "SGT. BUZZKILL + 1 COP");
	app.g.chase.cops_left = 1;
	chase_title(&app.g, s, sizeof s);
	CHECK_STR(s, "SGT. BUZZKILL");
	app.g.chase.cops_left = 0;
	chase_title(&app.g, s, sizeof s);
	CHECK_STR(s, "NO COPS LEFT");
}

static void test_row_wraps_and_click(void)
{
	int i;

	start_chase(30);
	press_seq(&ui, "L");
	CHECK_EQ(ui.row, 2);
	press_seq(&ui, "R");
	CHECK_EQ(ui.row, 0);
	ui_draw(&ui, &cv);
	i = hit_find(&ui, HIT_CHASE_BTN, 2);
	CHECK(i >= 0);
	app.g.p.run_permille = 1000;
	click_hit(&ui, hit_find(&ui, HIT_CHASE_BTN, 0));
	CHECK_EQ(ui.chase_done, CR_ESCAPED);
}

static void test_no_coming_soon_left(void)
{
	CHECK(!ui_stub(SCR_CHASE));
}

void tests_ui_chase(void)
{
	test_chase_after_news();
	test_run_and_escape();
	test_failed_run_stays();
	test_fight_without_weapon();
	test_fight_with_weapon();
	test_fight_wins();
	test_surrender_jail();
	test_jail_morning_news();
	test_death_in_chase_records_once();
	test_jail_on_last_day_ends();
	test_resume_mid_chase();
	test_odds_line();
	test_header();
	test_row_wraps_and_click();
	test_no_coming_soon_left();
}
