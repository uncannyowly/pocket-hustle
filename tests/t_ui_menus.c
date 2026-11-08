/* Title, new game, pause menu, game over and high scores. Text is read through the string
 * builders; the screens are also drawn to check they stay on the canvas. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "helpers.h"
#include "tests.h"
#include "ui_helpers.h"
#include "content/content.h"
#include "core/save.h"
#include "sfx/sfx.h"
#include "ui/input.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

static App app;
static Ui ui;
static Canvas cv;
static Game before;
static char dir[64];

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

static void through_news(void)
{
	int n = 0;

	while (ui.scr == SCR_NEWS && n++ < 40) {
		ui_draw(&ui, &cv);
		press_seq(&ui, "A");
	}
}

static int total_scores(const App *a)
{
	return a->scores.n[0] + a->scores.n[1] + a->scores.n[2] + a->scores.n[3];
}

/* A game in a fresh data directory, on the market. */
static void setup_dir(int days)
{
	system("mkdir -p build");
	snprintf(dir, sizeof dir, "build/menus-test-XXXXXX");
	if (!mkdtemp(dir)) {
		fprintf(stderr, "mkdtemp failed\n");
		exit(2);
	}
	memset(&app, 0, sizeof app);
	app_init(&app, dir);
	app_load(&app);
	{
		Params p = test_params();

		new_test_game(&app.g, &p, days);
	}
	app.has_game = 1;
	ui_init(&ui, &app);
	go(&ui, SCR_MARKET);
}

static int file_exists(const char *name)
{
	char path[600];

	snprintf(path, sizeof path, "%s/%s", dir, name);
	return access(path, F_OK) == 0;
}

static void snap(void)
{
	before = app.g;
}

/* Types one character of the keyboard by aiming the cursor at it and pressing A. */
static void type_ch(char ch)
{
	static const char *const ROWS[3] = {"ABCDEFGHIJ", "KLMNOPQRST", "UVWXYZ.'!?"};
	int r, c;

	for (r = 0; r < 3; r++)
		for (c = 0; ROWS[r][c]; c++)
			if (ROWS[r][c] == ch) {
				ui.ng.zone = 1;
				ui.ng.kr = r;
				ui.ng.kc = c;
				press_seq(&ui, "A");
				return;
			}
	CHECK(0);
}

static void key_at(int r, int c)
{
	ui.ng.zone = 1;
	ui.ng.kr = r;
	ui.ng.kc = c;
	press_seq(&ui, "A");
}

static void test_title_continue_without_game(void)
{
	char s[64];

	app_init(&app, NULL);
	ui_init(&ui, &app);
	CHECK_EQ(ui.scr, SCR_TITLE);
	title_detail(&app, s, sizeof s);
	CHECK_STR(s, "NO SAVED GAME");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_TITLE);
	CHECK(ui.toast[0] != '\0');
	CHECK(heard(SFX_DENY));
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_title_continue_resumes(void)
{
	char s[64], city[32];

	ui_test_setup(&app, &ui, 30);
	app.g.day = 3;
	go(&ui, SCR_TITLE);
	city_label(app.g.loc.city, city, sizeof city);
	title_detail(&app, s, sizeof s);
	{
		char want[64];

		snprintf(want, sizeof want, "DAY 3, %s", city);
		CHECK_STR(s, want);
	}
	snap();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(same_game(&before, &app.g));

	/* a save in the middle of a chase: the chase screen is set up, not blank */
	ui_test_setup(&app, &ui, 30);
	app.g.phase = PH_CHASE;
	app.g.chase.posse = 3;
	app.g.chase.cops_left = 3;
	app.g.nmsgs = 0;
	go(&ui, SCR_TITLE);
	ui.chase_log[0] = '\0';
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_CHASE);
	CHECK_STR(ui.chase_log, "The cops are on you.");
	CHECK_EQ(ui.chase_done, CR_NONE);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(heard(SFX_SIREN));
	chase_title(&app.g, s, sizeof s);
	CHECK(s[0] != '\0');
}

static void test_title_cycles_skyline(void)
{
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	ui.frame = 79;
	CHECK_EQ(title_city(&ui), 0);
	ui.frame = 80;
	CHECK_EQ(title_city(&ui), 1);
	ui.frame = 80 * 7;
	CHECK_EQ(title_city(&ui), 0);
	{
		Canvas other;

		ui.frame = 0;
		ui_draw(&ui, &cv);
		ui.frame = 160;
		ui_draw(&ui, &other);
		CHECK(memcmp(cv.px, other.px, sizeof cv.px) != 0);
	}
}

static void test_title_navigation_and_clicks(void)
{
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	CHECK_EQ(ui.title_row, 0);
	press_seq(&ui, "U");
	CHECK_EQ(ui.title_row, 3);
	press_seq(&ui, "D");
	CHECK_EQ(ui.title_row, 0);
	press_seq(&ui, "D D");
	CHECK_EQ(ui.title_row, 2);
	ui_draw(&ui, &cv);
	CHECK(hit_find(&ui, HIT_TITLE_ROW, 0) >= 0);
	CHECK(hit_find(&ui, HIT_TITLE_ROW, 3) >= 0);
	click_hit(&ui, hit_find(&ui, HIT_TITLE_ROW, 1));
	CHECK_EQ(ui.scr, SCR_NEWGAME);
}

static void test_new_game_flow(void)
{
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D A");
	CHECK_EQ(ui.scr, SCR_NEWGAME);
	CHECK_EQ(ui.ng.zone, 1);
	CHECK_EQ(ui.ng.len, 1);
	CHECK_STR(ui.ng.name, "Test");		/* the last name, ready to keep or clear */
	press_seq(&ui, "B B B B B");		/* B deletes a letter in the keyboard zone */
	CHECK_STR(ui.ng.name, "");
	type_ch('S');
	type_ch('E');
	type_ch('T');
	type_ch('H');
	CHECK_STR(ui.ng.name, "Seth");
	press_seq(&ui, "L1");			/* keyboard to the length chips */
	CHECK_EQ(ui.ng.zone, 0);
	press_seq(&ui, "L");
	CHECK_EQ(ui.ng.len, 0);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "START A");		/* a game is in progress: confirm the replacement */
	CHECK_EQ(app.g.last_day, 15);
	CHECK_STR(app.g.name, "Seth");
	CHECK_EQ(app.g.day, 1);
	CHECK_EQ(app.g.phase, PH_TRADING);
	CHECK(app.has_game);
	CHECK_EQ(app.last_rank, -1);
	CHECK_EQ(ui.scr, SCR_NEWS);		/* the opening card */
	CHECK(strstr(ui.news.cards[0].text, "15 days") != NULL);
	through_news();
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(heard(SFX_OK));
}

static void test_new_game_other_ways(void)
{
	/* the START HUSTLING button, and the length chosen on the way */
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D A");
	press_seq(&ui, "D D D D");		/* four key rows down, then the button */
	CHECK_EQ(ui.ng.zone, 2);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "U");
	CHECK_EQ(ui.ng.zone, 1);
	press_seq(&ui, "D A A");
	CHECK_EQ(app.g.last_day, 30);
	CHECK_STR(app.g.name, "Test");
	CHECK_EQ(app.g.day, 1);

	/* B on the length chips goes back to the title; a blank name is not left blank */
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D A B B B B B L1 B");
	CHECK_EQ(ui.scr, SCR_TITLE);
	press_seq(&ui, "U A");			/* CONTINUE */
	CHECK_EQ(ui.scr, SCR_MARKET);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D A B B B B B START A");
	CHECK(app.g.name[0] != '\0');
	CHECK_EQ(app.g.day, 1);
}

static void test_name_limit_and_case(void)
{
	int i;

	ui_test_setup(&app, &ui, 30);
	ng_open(&ui, 1, "");
	for (i = 0; i < 20; i++)
		type_ch((char)('A' + i % 10));
	CHECK_EQ((int)strlen(ui.ng.name), 15);
	CHECK_STR(ui.ng.name, "Abcdefghijabcde");

	ng_open(&ui, 1, "");
	CHECK(!ng_lower(&ui));
	type_ch('S');
	CHECK(ng_lower(&ui));
	key_at(3, 1);				/* SHIFT: the next letter is upper case */
	CHECK(ui.ng.shift);
	CHECK(!ng_lower(&ui));
	type_ch('E');
	CHECK_STR(ui.ng.name, "SE");
	CHECK(!ui.ng.shift);
	type_ch('T');
	CHECK_STR(ui.ng.name, "SEt");
	key_at(3, 4);				/* SPACE */
	CHECK_STR(ui.ng.name, "SEt ");
	CHECK(!ng_lower(&ui));			/* a word starts upper case */
	type_ch('A');
	CHECK_STR(ui.ng.name, "SEt A");
	key_at(3, 4);
	key_at(3, 0);				/* SHIFT after a space: lower case */
	type_ch('B');
	CHECK_STR(ui.ng.name, "SEt A b");
	key_at(3, 8);				/* DEL */
	CHECK_STR(ui.ng.name, "SEt A ");
	press_seq(&ui, "B B B B B B B");
	CHECK_STR(ui.ng.name, "");
	key_at(3, 8);				/* DEL on nothing */
	CHECK_STR(ui.ng.name, "");

	/* SHIFT stays pending when the name is full */
	ng_open(&ui, 1, "Abcdefghijklmno");
	key_at(3, 0);
	type_ch('A');
	CHECK_EQ((int)strlen(ui.ng.name), 15);
	CHECK(ui.ng.shift);
}

static void test_keyboard_navigation(void)
{
	ui_test_setup(&app, &ui, 30);
	ng_open(&ui, 1, "");
	press_seq(&ui, "U");
	CHECK_EQ(ui.ng.zone, 0);
	press_seq(&ui, "D");
	CHECK_EQ(ui.ng.zone, 1);
	press_seq(&ui, "L");
	CHECK_EQ(ui.ng.kc, 9);
	press_seq(&ui, "R R");
	CHECK_EQ(ui.ng.kc, 1);
	press_seq(&ui, "D D D");
	CHECK_EQ(ui.ng.kr, 3);
	press_seq(&ui, "A");			/* column 1 of row 4 is SHIFT */
	CHECK(ui.ng.shift);
	press_seq(&ui, "R1 R1");
	CHECK_EQ(ui.ng.zone, 0);		/* keyboard, button, then the chips */
	press_seq(&ui, "R1");
	CHECK_EQ(ui.ng.zone, 1);
	press_seq(&ui, "R1");
	CHECK_EQ(ui.ng.zone, 2);
	press_seq(&ui, "B");
	CHECK_EQ(ui.ng.zone, 1);
}

static void test_new_game_clicks(void)
{
	ui_test_setup(&app, &ui, 30);
	ng_open(&ui, 1, "");
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_KEY, 0 * 10 + 7));	/* H */
	CHECK_STR(ui.ng.name, "H");
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_KEY, 0 * 10 + 4));	/* E */
	CHECK_STR(ui.ng.name, "He");
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_LEN, 3));
	CHECK_EQ(ui.ng.len, 3);
	CHECK_EQ(ui.ng.zone, 0);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_NG_GO, 0));
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 1));
	CHECK_EQ(app.g.last_day, 90);
	CHECK_STR(app.g.name, "He");
}

static void test_menu_resume(void)
{
	static const struct { Screen s; } FROM[] = {
		{SCR_MARKET}, {SCR_MOVE}, {SCR_FLY}, {SCR_BANK}, {SCR_LENDER}, {SCR_STORE}, {SCR_CHART},
	};
	size_t i;

	for (i = 0; i < sizeof FROM / sizeof FROM[0]; i++) {
		ui_test_setup(&app, &ui, 30);
		switch (FROM[i].s) {
		case SCR_MOVE: move_open(&ui); break;
		case SCR_FLY: fly_open(&ui); break;
		case SCR_BANK: bank_open(&ui); break;
		case SCR_LENDER: lender_open(&ui); break;
		case SCR_STORE: store_open(&ui); break;
		case SCR_CHART: chart_open(&ui, 1); break;
		default: break;
		}
		CHECK_EQ(ui.scr, FROM[i].s);
		snap();
		drain();
		press_seq(&ui, "START");
		CHECK_EQ(ui.scr, SCR_MENU);
		CHECK_EQ(ui.under, FROM[i].s);
		CHECK_EQ(ui.menu_row, 0);
		CHECK(heard(SFX_OK));
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
		press_seq(&ui, "B");
		CHECK_EQ(ui.scr, FROM[i].s);
		CHECK(same_game(&before, &app.g));
		press_seq(&ui, "START START");		/* START closes it too */
		CHECK_EQ(ui.scr, FROM[i].s);
		press_seq(&ui, "START A");		/* RESUME */
		CHECK_EQ(ui.scr, FROM[i].s);
		CHECK(same_game(&before, &app.g));
	}

	/* no menu from the title, the new game screen, the chase or the end */
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "START");
	CHECK(ui.scr != SCR_MENU);
	ng_open(&ui, 1, "");
	go(&ui, SCR_NEWGAME);
	snap();
	go(&ui, SCR_OVER);
	press_seq(&ui, "START");
	CHECK_EQ(ui.scr, SCR_OVER);
}

static void test_menu_navigation_and_labels(void)
{
	char s[32];

	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "START");
	menu_label(&app, 0, s, sizeof s);
	CHECK_STR(s, "RESUME");
	menu_label(&app, 1, s, sizeof s);
	CHECK_STR(s, "SOUND: ON");
	menu_label(&app, 2, s, sizeof s);
	CHECK_STR(s, "HIGH SCORES");
	menu_label(&app, 3, s, sizeof s);
	CHECK_STR(s, "SAVE AND QUIT");
	menu_label(&app, 4, s, sizeof s);
	CHECK_STR(s, "END GAME NOW");
	press_seq(&ui, "U");
	CHECK_EQ(ui.menu_row, 4);
	press_seq(&ui, "D");
	CHECK_EQ(ui.menu_row, 0);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_MENU_ROW, 1));
	CHECK(!app.sound_on);
	menu_label(&app, 1, s, sizeof s);
	CHECK_STR(s, "SOUND: OFF");
}

static void test_sound_toggle_persists(void)
{
	char path[600], line[64] = "";
	FILE *f;

	setup_dir(30);
	press_seq(&ui, "START D A");
	CHECK(!app.sound_on);
	CHECK_EQ(ui.scr, SCR_MENU);		/* the menu stays open */
	snprintf(path, sizeof path, "%s/options.txt", dir);
	f = fopen(path, "r");
	CHECK(f != NULL);
	if (f) {
		if (!fgets(line, sizeof line, f))
			line[0] = '\0';
		fclose(f);
	}
	CHECK_STR(line, "sound=0\n");
	press_seq(&ui, "A");
	CHECK(app.sound_on);
	f = fopen(path, "r");
	if (f) {
		if (!fgets(line, sizeof line, f))
			line[0] = '\0';
		fclose(f);
	}
	CHECK_STR(line, "sound=1\n");
}

static void test_save_and_quit(void)
{
	App again;

	setup_dir(30);
	app.g.cash = 4321;
	app.g.day = 5;
	CHECK(!file_exists("save.dat"));
	press_seq(&ui, "START D D D A");
	CHECK_EQ(ui.scr, SCR_TITLE);
	CHECK(file_exists("save.dat"));
	CHECK_EQ(ui.title_row, 0);
	CHECK(!ui_quit(&ui));
	app.g.cash = 1;				/* what is on disk is what comes back */
	memset(&again, 0, sizeof again);
	app_init(&again, dir);
	app_load(&again);
	CHECK(again.has_game);
	CHECK_EQ(again.g.day, 5);
	CHECK_EQ(again.g.cash, 4321);
	app.g.cash = 4321;
	snap();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK_EQ(app.g.day, 5);
	CHECK_EQ(app.g.cash, 4321);
	CHECK(same_game(&before, &app.g));
}

static void test_end_game_now(void)
{
	setup_dir(30);
	app_save_now(&app);
	CHECK(file_exists("save.dat"));
	press_seq(&ui, "START U A A");
	CHECK_EQ(ui.menu_row, 4);
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(app.g.phase, PH_OVER);
	CHECK_EQ(app.g.end, END_QUIT);
	CHECK_EQ(total_scores(&app), 1);
	CHECK(!file_exists("save.dat"));
	CHECK(file_exists("scores.dat"));
	CHECK(!app.has_game);
	CHECK(app.last_rank >= 0);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_over_buttons(void)
{
	ui_test_setup(&app, &ui, 60);
	press_seq(&ui, "START U A A");
	CHECK_EQ(ui.scr, SCR_OVER);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_NEWGAME);
	CHECK_EQ(ui.ng.len, 2);			/* 60 days, the one just played */
	CHECK_EQ(ui.ng.zone, 1);
	CHECK_STR(ui.ng.name, "Test");
	go(&ui, SCR_OVER);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_TITLE);
	CHECK_EQ(ui.title_row, 0);
}

static void test_over_texts(void)
{
	char s[64];
	StatCell cells[10];
	int n, i, found = 0;

	ui_test_setup(&app, &ui, 30);
	app.g.phase = PH_OVER;
	app.g.end = END_DAYS_UP;
	over_title(&app.g, s, sizeof s);
	CHECK_STR(s, "TIME'S UP");
	app.g.end = END_DEAD;
	over_title(&app.g, s, sizeof s);
	CHECK_STR(s, "GAME OVER");
	app.g.end = END_QUIT;
	over_title(&app.g, s, sizeof s);
	CHECK_STR(s, "RETIRED EARLY");

	app.last_rank = 3;
	over_rank(&app, s, sizeof s);
	CHECK_STR(s, "HIGH SCORE #4 FOR 30 DAYS");
	app.last_rank = -1;
	over_rank(&app, s, sizeof s);
	CHECK_STR(s, "NO HIGH SCORE THIS TIME");

	app.g.stats.moves = 29;
	app.g.stats.flights = 7;
	app.g.stats.cop_chases = 6;
	app.g.stats.cops_beaten = 4;
	app.g.stats.escapes = 3;
	app.g.stats.times_jailed = 1;
	app.g.stats.times_mugged = 2;
	app.g.stats.goon_visits = 5;
	app.g.stats.bank_fees_paid = 1234;
	n = over_stats(&app.g, cells);
	CHECK_EQ(n, 10);
	CHECK_STR(cells[0].label, "Days");
	CHECK_STR(cells[0].value, "30");
	CHECK_STR(cells[1].label, "Moves");
	CHECK_STR(cells[1].value, "29");
	CHECK_STR(cells[2].value, "7");
	CHECK_STR(cells[3].label, "Chases");
	CHECK_STR(cells[3].value, "6");
	CHECK_STR(cells[4].label, "Beat cops");
	CHECK_STR(cells[4].value, "4");
	CHECK_STR(cells[5].value, "3");
	CHECK_STR(cells[6].label, "Jailed");
	CHECK_STR(cells[6].value, "1");
	CHECK_STR(cells[7].value, "2");
	CHECK_STR(cells[8].label, "Goons");
	CHECK_STR(cells[8].value, "5");
	for (i = 0; i < n; i++)
		if (strcmp(cells[i].label, "ATM fees") == 0) {
			found = 1;
			CHECK_STR(cells[i].value, "$1.2K");
		}
	CHECK(found);
	app.g.stats.bank_fees_paid = 0;
	over_stats(&app.g, cells);
	CHECK_STR(cells[9].value, "$0");
}

static void test_over_draws(void)
{
	static const EndReason ENDS[] = {END_DAYS_UP, END_DEAD, END_QUIT};
	static const int64_t WORTH[] = {612480, -50000, 0, 123456789012LL, -999999999999LL, MONEY_MAX};
	size_t e, w;

	for (e = 0; e < 3; e++)
		for (w = 0; w < sizeof WORTH / sizeof WORTH[0]; w++) {
			ui_test_setup(&app, &ui, 30);
			app.g.phase = PH_OVER;
			app.g.end = ENDS[e];
			app.g.cash = WORTH[w];
			app.g.stats.bank_fees_paid = WORTH[w] < 0 ? 5 : WORTH[w];
			app.g.stats.moves = 99999;
			app.last_rank = w % 2 ? -1 : 0;
			go(&ui, SCR_OVER);
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
			CHECK(canvas_colors(&cv, 20) >= 8);
		}
}

static void seed_scores(void)
{
	int i;

	for (i = 0; i < 10; i++) {
		char nm[16];

		snprintf(nm, sizeof nm, "Bot%d", i);
		hiscore_insert(&app.scores, 30, nm, 900000 - i * 50000, i == 3 ? END_QUIT : END_DAYS_UP);
	}
	hiscore_insert(&app.scores, 15, "Quick", 70000, END_DEAD);
}

static void test_scores_tabs(void)
{
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D D A");
	CHECK_EQ(ui.scr, SCR_SCORES);
	CHECK_EQ(ui.scores_tab, 1);
	press_seq(&ui, "R1");
	CHECK_EQ(ui.scores_tab, 2);
	press_seq(&ui, "R1 R1");
	CHECK_EQ(ui.scores_tab, 0);
	press_seq(&ui, "L1");
	CHECK_EQ(ui.scores_tab, 3);
	press_seq(&ui, "R");
	CHECK_EQ(ui.scores_tab, 0);
	press_seq(&ui, "L");
	CHECK_EQ(ui.scores_tab, 3);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_SCORES_TAB, 1));
	CHECK_EQ(ui.scores_tab, 1);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_TITLE);

	/* from the pause menu: B comes back to the menu over the screen it paused */
	ui_test_setup(&app, &ui, 60);
	bank_open(&ui);
	press_seq(&ui, "START D D A");
	CHECK_EQ(ui.scr, SCR_SCORES);
	CHECK_EQ(ui.scores_tab, 2);		/* this game's length */
	press_seq(&ui, "R1");
	CHECK_EQ(ui.scores_tab, 3);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MENU);
	CHECK_EQ(ui.under, SCR_BANK);
	CHECK_EQ(ui.menu_row, 2);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_BANK);
}

static void test_scores_draw_and_highlight(void)
{
	ui_test_setup(&app, &ui, 30);
	seed_scores();
	app.last_rank = -1;
	CHECK_EQ(scores_hilite(&app, 1), -1);
	go(&ui, SCR_SCORES);
	ui.scores_tab = 1;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	app.last_rank = 2;			/* just recorded, third of this game's length */
	CHECK_EQ(scores_hilite(&app, 1), 2);
	CHECK_EQ(scores_hilite(&app, 0), -1);	/* another length has none */
	app.g.last_day = 15;
	hiscore_insert(&app.scores, 15, "Two", 60000, END_DAYS_UP);
	hiscore_insert(&app.scores, 15, "Three", 50000, END_DAYS_UP);
	CHECK_EQ(scores_hilite(&app, 0), 2);
	CHECK_EQ(scores_hilite(&app, 1), -1);
	app.g.last_day = 30;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	for (ui.scores_tab = 0; ui.scores_tab < 4; ui.scores_tab++) {
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
	}
	app.scores.t[1][0].score = MONEY_MAX;
	app.scores.t[1][1].score = -MONEY_MAX;
	ui.scores_tab = 1;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
}

static void test_scores_after_game(void)
{
	/* the title's HIGH SCORES opens on the tab of the game just finished */
	ui_test_setup(&app, &ui, 60);
	press_seq(&ui, "START U A A");
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK(app.last_rank >= 0);
	press_seq(&ui, "B D D A");
	CHECK_EQ(ui.scr, SCR_SCORES);
	CHECK_EQ(ui.scores_tab, 2);
	CHECK_EQ(scores_hilite(&app, 2), app.last_rank);
}

static void test_quit(void)
{
	ui_test_setup(&app, &ui, 30);
	go(&ui, SCR_TITLE);
	CHECK(!ui_quit(&ui));
	press_seq(&ui, "D D D");
	CHECK_EQ(ui.title_row, 3);
	CHECK(!ui_quit(&ui));
	press_seq(&ui, "A");
	CHECK(ui_quit(&ui));
}

static void test_every_screen_draws(void)
{
	int s;

	for (s = 0; s < NUM_SCREENS; s++) {
		ui_test_setup(&app, &ui, 30);
		CHECK(!ui_stub((Screen)s));
		go(&ui, (Screen)s);
		ui_draw(&ui, &cv);
		CHECK_EQ(cv.overflow, 0);
	}
}

static void test_menu_over_market_dims(void)
{
	Canvas plain;

	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &plain);
	press_seq(&ui, "START");
	ui_draw(&ui, &cv);
	CHECK(memcmp(plain.px, cv.px, sizeof cv.px) != 0);
	CHECK(hit_find(&ui, HIT_MARKET_ROW, 0) < 0);	/* only the menu takes clicks */
	CHECK(hit_find(&ui, HIT_MENU_ROW, 4) >= 0);
}

static void test_end_game_confirm(void)
{
	Canvas cvc;

	setup_dir(30);
	app_save_now(&app);
	snap();
	press_seq(&ui, "START U A");		/* END GAME NOW, once */
	CHECK_EQ(ui.scr, SCR_MENU);
	CHECK_EQ(ui.confirm, CONFIRM_END);
	CHECK(same_game(&before, &app.g));
	CHECK(file_exists("save.dat"));
	CHECK_EQ(total_scores(&app), 0);
	ui_draw(&ui, &cvc);
	CHECK_EQ(cvc.overflow, 0);
	CHECK(hit_find(&ui, HIT_CONFIRM, 1) >= 0);
	CHECK(hit_find(&ui, HIT_CONFIRM, 0) >= 0);
	press_seq(&ui, "B");			/* B cancels and keeps the menu */
	CHECK_EQ(ui.scr, SCR_MENU);
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK(same_game(&before, &app.g));
	CHECK(file_exists("save.dat"));
	press_seq(&ui, "A");			/* asks again */
	CHECK_EQ(ui.confirm, CONFIRM_END);
	press_seq(&ui, "U");			/* moving off the row cancels */
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK_EQ(ui.scr, SCR_MENU);
	CHECK(same_game(&before, &app.g));
	press_seq(&ui, "D A START");		/* START cancels too, and stays */
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK_EQ(ui.scr, SCR_MENU);
	CHECK(same_game(&before, &app.g));
	press_seq(&ui, "A A");			/* the second A ends it */
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(app.g.end, END_QUIT);
	CHECK_EQ(total_scores(&app), 1);
	CHECK(!file_exists("save.dat"));

	/* the mouse: NO, then YES; a click on another row does not end anything */
	setup_dir(30);
	press_seq(&ui, "START U A");
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_MENU_ROW, 3));
	CHECK_EQ(ui.scr, SCR_MENU);
	CHECK_EQ(app.g.phase, PH_TRADING);
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 0));
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK_EQ(app.g.phase, PH_TRADING);
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_MENU_ROW, 4));
	ui_draw(&ui, &cvc);
	CHECK_EQ(ui.confirm, CONFIRM_END);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 1));
	CHECK_EQ(ui.scr, SCR_OVER);
	CHECK_EQ(total_scores(&app), 1);
}

static void test_replace_confirm(void)
{
	Canvas cvc;
	Game old;

	/* a game in progress: the prompt comes first */
	setup_dir(30);
	app.g.cash = 777;
	app_save_now(&app);
	old = app.g;
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D A");			/* NEW GAME */
	press_seq(&ui, "L1 L");			/* 15 days */
	press_seq(&ui, "START");
	CHECK_EQ(ui.scr, SCR_NEWGAME);
	CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
	CHECK(same_game(&old, &app.g));
	ui_draw(&ui, &cvc);
	CHECK_EQ(cvc.overflow, 0);
	CHECK(hit_find(&ui, HIT_CONFIRM, 1) >= 0);
	press_seq(&ui, "B");			/* back to the screen, nothing lost */
	CHECK_EQ(ui.scr, SCR_NEWGAME);
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK_EQ(ui.ng.len, 0);
	CHECK_STR(ui.ng.name, "Test");
	CHECK(same_game(&old, &app.g));
	{
		Game disk;

		CHECK_EQ(app.has_game, 1);
		app_save_now(&app);
		CHECK_EQ(save_read(&disk, app.save_path), 0);
		CHECK_EQ(disk.cash, 777);
		CHECK_EQ(disk.last_day, 30);
	}
	press_seq(&ui, "START");
	CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
	press_seq(&ui, "U D");			/* other buttons do nothing while it is up */
	CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
	CHECK(same_game(&old, &app.g));
	press_seq(&ui, "A");
	CHECK_EQ(app.g.last_day, 15);
	CHECK_EQ(app.g.day, 1);
	CHECK(!same_game(&old, &app.g));
	CHECK_EQ(ui.confirm, CONFIRM_NONE);

	/* the button, then the mouse */
	setup_dir(30);
	ng_open(&ui, 1, "Zed");
	ui.ng.zone = 2;
	press_seq(&ui, "A");
	CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, -1));	/* the blocker eats stray clicks */
	CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 0));
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK_STR(app.g.name, "Test");
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_NG_GO, 0));
	CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
	ui_draw(&ui, &cvc);
	click_hit(&ui, hit_find(&ui, HIT_CONFIRM, 1));
	CHECK_STR(app.g.name, "Zed");

	/* nothing in progress: no prompt */
	app_init(&app, NULL);
	ui_init(&ui, &app);
	press_seq(&ui, "D A START");
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK(app.has_game);
	CHECK_EQ(app.g.day, 1);

	/* after game over the old game is gone, so no prompt either */
	ui_test_setup(&app, &ui, 30);
	press_seq(&ui, "START U A A A");
	CHECK_EQ(ui.scr, SCR_NEWGAME);
	press_seq(&ui, "START");
	CHECK_EQ(ui.confirm, CONFIRM_NONE);
	CHECK(app.has_game);
}

void tests_ui_menus(void)
{
	test_end_game_confirm();
	test_replace_confirm();
	test_title_continue_without_game();
	test_title_continue_resumes();
	test_title_cycles_skyline();
	test_title_navigation_and_clicks();
	test_new_game_flow();
	test_new_game_other_ways();
	test_name_limit_and_case();
	test_keyboard_navigation();
	test_new_game_clicks();
	test_menu_resume();
	test_menu_navigation_and_labels();
	test_sound_toggle_persists();
	test_save_and_quit();
	test_end_game_now();
	test_over_buttons();
	test_over_texts();
	test_over_draws();
	test_scores_tabs();
	test_scores_draw_and_highlight();
	test_scores_after_game();
	test_quit();
	test_every_screen_draws();
	test_menu_over_market_dims();
}
