/* Screenshots of every screen, rendered with the real UI code into build/shots/NN-name.ppm
 * (scripts/shots.sh turns them into PNG). Games are the tests' deterministic ones. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/params.h"
#include "core/save.h"
#include "gfx/canvas.h"
#include "helpers.h"
#include "ui/input.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"
#include "ui_helpers.h"

static App app;
static Ui ui;
static Canvas cv;
static uint32_t big[640 * 480];
static const char *outdir;
static int shot_no;

static void shot(const char *name)
{
	char path[600];
	FILE *f;
	int i;

	ui_draw(&ui, &cv);
	cv_scale2x(&cv, big, 640);
	snprintf(path, sizeof path, "%s/%02d-%s.ppm", outdir, ++shot_no, name);
	f = fopen(path, "wb");
	if (!f) {
		perror(path);
		exit(1);
	}
	fprintf(f, "P6\n640 480\n255\n");
	for (i = 0; i < 640 * 480; i++) {
		unsigned char px[3] = {(unsigned char)(big[i] >> 16), (unsigned char)(big[i] >> 8), (unsigned char)big[i]};

		fwrite(px, 1, 3, f);
	}
	fclose(f);
	if (cv.overflow)
		fprintf(stderr, "shots: %s: text overflowed %u times\n", name, (unsigned)cv.overflow);
}

static void through_news(void)
{
	int n = 0;

	while (ui.scr == SCR_NEWS && n++ < 40) {
		ui_draw(&ui, &cv);
		press_seq(&ui, "A");
	}
}

/* A game a few days in with goods in the bag, so the market, the bag and the chart have
 * something to show. */
static void setup(int days)
{
	int i;

	ui_test_setup(&app, &ui, days);
	app.g.cash = 90000;
	for (i = 0; i < 5; i++) {
		Loc d = {app.g.loc.city, (uint8_t)((app.g.loc.hood + 1) % HOODS_PER_CITY)};

		game_buy(&app.g, i, 3 + i);
		game_move(&app.g, d);
	}
	game_buy(&app.g, 0, 4);
	game_buy(&app.g, 3, 2);
	game_buy(&app.g, 6, 5);
	app.g.p.enforce_days = params_default().enforce_days;	/* the tests turn the goons off */
	app.g.cash = 4321;
	app.g.bank = 12000;
	app.g.debt[0] = 5500;
	app.g.nmsgs = 0;
	app.g.phase = PH_TRADING;
	go(&ui, SCR_MARKET);
}

int main(int argc, char **argv)
{
	int i;
	char nm[32];
	static const char *const CITY[NUM_CITIES] = {"okc", "houston", "sanjose", "nyc", "miami", "neworleans", "portland"};

	if (argc != 2) {
		fprintf(stderr, "usage: ph-shots OUTDIR\n");
		return 2;
	}
	outdir = argv[1];

	setup(30);
	go(&ui, SCR_TITLE);
	shot("title");
	ng_open(&ui, 1, "Seth");
	shot("newgame");
	setup(30);
	shot("market");
	ui.mrow = 2;
	press_seq(&ui, "A");
	shot("picker-buy");
	press_seq(&ui, "B");
	press_seq(&ui, "R");
	ui.brow = 0;
	press_seq(&ui, "A");
	shot("picker-sell");
	setup(30);
	press_seq(&ui, "R1 R1 A D A");
	shot("news");
	through_news();

	setup(30);
	move_open(&ui);
	shot("move");
	for (i = 0; i < 6; i++) {
		setup(30);
		fly_open(&ui);
		ui.row = i;
		snprintf(nm, sizeof nm, "fly-%s", CITY[fly_city(&app.g, i)]);
		shot(nm);
	}
	setup(30);
	app.g.loc.city = 1;
	app.g.loc.hood = 0;
	game_move(&app.g, (Loc){1, 0});
	fly_open(&ui);
	for (i = 0; i < 6; i++)
		if (fly_city(&app.g, i) == 0)
			ui.row = i;
	snprintf(nm, sizeof nm, "fly-%s", CITY[0]);
	shot(nm);

	setup(30);
	app.g.loc.city = 0;
	app.g.loc.hood = 0;
	bank_open(&ui);
	shot("bank-branch");
	setup(30);
	app.g.loc.city = 0;
	app.g.loc.hood = 2;
	bank_open(&ui);
	shot("bank-atm");
	setup(30);
	app.g.loc.city = 0;
	app.g.loc.hood = 0;
	picker_open(&ui, PK_DEPOSIT, -1, SCR_BANK);
	press_seq(&ui, "L L L U U U");
	shot("dial");

	for (i = 0; i < NUM_CITIES; i++) {
		setup(30);
		app.g.loc.city = (uint8_t)i;
		app.g.loc.hood = 0;
		app.g.debt[i] = 3000 + 700 * i;
		lender_open(&ui);
		snprintf(nm, sizeof nm, "lender-%s", CITY[i]);
		shot(nm);
	}

	setup(30);
	store_open(&ui);
	shot("store");
	setup(30);
	app.g.loc.hood = 3;
	picker_open(&ui, PK_HEAL, -1, SCR_MARKET);
	shot("doctor");
	setup(30);
	chart_open(&ui, 3);
	shot("chart");

	/* a chase: cops certain, something in the stash */
	setup(30);
	app.g.p.cop_base_permille = 1000;
	app.g.stash[0] = 5;
	app.g.stash_paid[0] = 50;
	press_seq(&ui, "R1 R1 A D A A");
	shot("news-trouble");
	through_news();
	shot("chase");

	setup(30);
	press_seq(&ui, "START");
	shot("menu");
	press_seq(&ui, "U A");
	shot("confirm-end");
	press_seq(&ui, "A");
	shot("gameover");

	setup(30);
	for (i = 0; i < 10; i++) {
		snprintf(nm, sizeof nm, "Bot%d", i);
		hiscore_insert(&app.scores, 30, nm, 900000 - i * 50000, i == 3 ? END_QUIT : END_DAYS_UP);
	}
	hiscore_insert(&app.scores, 15, "Quick", 70000, END_DEAD);
	go(&ui, SCR_TITLE);
	press_seq(&ui, "D D A");
	shot("scores");

	printf("shots: %d written to %s\n", shot_no, outdir);
	return 0;
}
