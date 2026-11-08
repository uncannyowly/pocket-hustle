/* The edge sweep: every screen and overlay in the most extreme state the game can reach
 * (maxed money, debts, bags, names, counters), held buttons and click regions. A screen
 * passes when it draws no glyph off the canvas, drops no hit region and keeps every region
 * on the canvas. */
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "helpers.h"
#include "tests.h"
#include "ui_helpers.h"
#include "ui/input.h"
#include "ui/ui.h"
#include "ui/ui_internal.h"

static App app;
static Ui ui;
static Canvas cv;
static const char *tag = "";	/* the state being drawn, named in a failure */

static int city_named(const char *name)
{
	int i;

	for (i = 0; i < NUM_CITIES; i++)
		if (strcmp(CITIES[i].name, name) == 0)
			return i;
	return 0;
}

/* Draws the current screen and checks the three edge rules. */
static void edge(const char *what)
{
	int i, bad = 0;

	ui_draw(&ui, &cv);
	t_checks++;
	if (cv.overflow) {
		fprintf(stderr, "t_ui_edges: %s / %s: %u glyph cells off the canvas\n", tag, what, (unsigned)cv.overflow);
		bad = 1;
	}
	if (ui.hits_dropped) {
		fprintf(stderr, "t_ui_edges: %s / %s: %d hit regions dropped (list full)\n", tag, what, ui.hits_dropped);
		bad = 1;
	}
	if (ui.scr == SCR_PICKER) {		/* the box holds 30 cells a line; a title and a unit share the top one */
		PickText t;

		picker_text(&ui, &t);
		if (strlen(t.total) > 30 || strlen(t.info) > 30 || strlen(t.title) + strlen(t.unit) + 2 > 30) {
			fprintf(stderr, "t_ui_edges: %s / %s: picker text wider than its box: \"%s\" \"%s\" \"%s %s\"\n",
				tag, what, t.total, t.info, t.title, t.unit);
			bad = 1;
		}
	}
	for (i = 0; i < ui.nhits; i++) {
		const Hit *h = &ui.hits[i];

		if (h->x < 0 || h->y < 0 || h->w <= 0 || h->h <= 0 || h->x + h->w > CV_W || h->y + h->h > CV_H) {
			fprintf(stderr, "t_ui_edges: %s / %s: hit %d (%d,%d %dx%d action %d) outside the canvas\n",
				tag, what, i, h->x, h->y, h->w, h->h, h->action);
			bad = 1;
		}
	}
	if (bad)
		t_failed++;
}

/* The most any field can hold. loc: the city and hood. big_price: every price at its top. */
static void extreme(const char *city, int hood, int big_price)
{
	Game *g;
	int i, d;

	ui_test_setup(&app, &ui, 90);
	g = &app.g;
	g->loc.city = (uint8_t)city_named(city);
	g->loc.hood = (uint8_t)hood;
	memset(g->name, 'W', 15);
	g->name[15] = '\0';
	g->cash = g->bank = MONEY_MAX;
	g->day = 90;
	g->health = 100;
	g->capacity = 10000;
	g->weapon = (int8_t)(NUM_GEAR - 1);
	for (i = 0; i < NUM_GEAR; i++)
		g->gear_owned[i] = 1;
	for (i = 0; i < NUM_CITIES; i++) {
		g->debt[i] = MONEY_MAX - 1;
		g->debt_age[i] = 90;
	}
	for (i = 0; i < NUM_GOODS; i++) {
		g->stash[i] = 1000;
		g->stash_paid[i] = MONEY_MAX / 2;
		g->price[i] = big_price ? 2000000000 : 1;
		for (d = 1; d <= MAX_DAYS; d++) {
			g->hist[i][d] = d % 3 ? g->price[i] : (big_price ? 1 : 2000000000);
			g->hist_ev[i][d] = (int8_t)(d % 3);
		}
	}
	/* counters grow by a few a day, never past five digits in 90 days */
	g->stats.moves = g->stats.flights = g->stats.cop_chases = g->stats.cops_beaten =
		g->stats.escapes = g->stats.times_jailed = g->stats.times_mugged = g->stats.times_dropped =
		g->stats.goon_visits = g->stats.spikes_seen = g->stats.crashes_seen = g->stats.rumors_true =
		g->stats.rumors_joke = 99999;
	g->stats.best_net_worth = g->stats.hospital_spent = g->stats.bank_fees_paid = MONEY_MAX;
	g->stats.wild_wins = g->stats.wild_losses = 99999;
	g->nstakes = MAX_STAKES;		/* the longest names, running */
	g->stakes[0].chem = 5;
	g->stakes[1].chem = 2;
	g->stakes[2].chem = 8;
	for (i = 0; i < MAX_STAKES; i++) {
		g->stakes[i].stake = MONEY_MAX / 4;
		g->stakes[i].resolve_day = 90;
	}
	g->banned = 0x3ffffu & ~(1u << g->loc.city);	/* every other city shows BANNED */
	snprintf(app.save_path, sizeof app.save_path, "%s", "");
}

static void all_kinds_msgs(void)
{
	Game *g = &app.g;
	Msg m;
	int k, n = 0;

	for (k = 0; k < NUM_MSG_KINDS && n < MAX_MSGS; k++) {
		memset(&m, 0, sizeof m);
		m.kind = (uint8_t)k;
		m.a = 9;
		m.b = NUM_LOCS - 1;
		m.c = 1;
		m.variant = 7;
		m.amount = MONEY_MAX;
		g->msgs[n++] = m;
	}
	g->nmsgs = (int16_t)n;
}

static void sweep_market(void)
{
	int p, i;

	go(&ui, SCR_MARKET);
	for (p = 0; p < 3; p++) {
		ui.panel = p;
		for (i = 0; i < NUM_GOODS; i++) {
			ui.mrow = ui.brow = i;
			ui.act = i % NUM_MKT_ACTS;
			edge("market");
		}
	}
	ui.toast_until = ui.frame + 40;
	toast(&ui, "Not enough cash to cover the amount you asked for, so nothing happened at all.");
	edge("market with the longest toast");
	ui.toast[0] = '\0';
}

static void sweep_pickers(void)
{
	static const struct { PickKind k; Screen back; } K[] = {
		{PK_BUY, SCR_MARKET}, {PK_SELL, SCR_MARKET}, {PK_HEAL, SCR_MARKET}, {PK_DEPOSIT, SCR_BANK},
		{PK_WITHDRAW, SCR_BANK}, {PK_BORROW, SCR_LENDER}, {PK_REPAY, SCR_LENDER},
	};
	static Game saved;
	size_t k;
	int good, health;

	for (health = 1; health <= 100; health += 99) {
		app.g.health = (int16_t)health;
		for (k = 0; k < sizeof K / sizeof K[0]; k++)
			for (good = 0; good < NUM_GOODS; good++) {
				picker_open(&ui, K[k].k, good, K[k].back);
				edge("picker at zero");
				ui.pk.q = picker_max(&ui);
				edge("picker at max");
				ui.pk.q = picker_max(&ui) / 2;
				edge("picker at half");
				ui.pk.q = picker_max(&ui);	/* the toast of an OK at the max, then the game as it was */
				memcpy(&saved, &app.g, sizeof saved);
				ui_press(&ui, PAD_A);
				edge("toast after OK at max");
				memcpy(&app.g, &saved, sizeof saved);
				ui.toast[0] = '\0';
				if (K[k].k != PK_BUY && K[k].k != PK_SELL)
					break;
			}
	}
	app.g.health = 100;
}

static void sweep_screens(void)
{
	int i, g, d;

	sweep_market();
	sweep_pickers();
	for (i = 0; i < HOODS_PER_CITY; i++) {
		go(&ui, SCR_MOVE);
		ui.row = i;
		edge("move");
	}
	for (g = 0; g < 2; g++) {
		go(&ui, SCR_FLY);
		ui.fly_tab = g;
		for (i = 0; i < fly_count(&ui); i++) {
			ui.row = i;
			edge("fly");
		}
	}
	go(&ui, SCR_BANK);
	for (i = 0; i < 2; i++) {
		ui.row = i;
		edge("bank");
	}
	go(&ui, SCR_LENDER);
	for (i = 0; i < 2; i++) {
		ui.row = i;
		edge("lender");
	}
	for (i = 0; i < NUM_GEAR; i++) {
		go(&ui, SCR_STORE);
		ui.row = i;
		edge("store");
	}
	for (d = 0; d < 4; d++) {
		static const int DAYS[4] = {1, 2, 45, 90};

		app.g.day = (int16_t)DAYS[d];
		for (g = 0; g < NUM_GOODS; g++) {
			char nm[40];

			chart_open(&ui, g);
			snprintf(nm, sizeof nm, "chart good %d day %d", g, DAYS[d]);
			edge(nm);
		}
	}
	app.g.day = 90;
	for (i = 0; i < 4; i++) {
		go(&ui, SCR_TITLE);
		ui.title_row = i;
		edge("title");
	}
	for (i = 0; i < 4; i++) {
		ng_open(&ui, i, "WWWWWWWWWWWWWWW");
		ui.ng.zone = 1;
		for (g = 0; g < 40; g++) {
			ui.ng.kr = g / 10;
			ui.ng.kc = g % 10;
			edge("new game keyboard");
		}
		ui.ng.zone = 0;
		edge("new game lengths");
		ui.ng.zone = 2;
		edge("new game go");
		app.has_game = 1;		/* START HUSTLING over a game in progress asks first */
		ui_press(&ui, PAD_A);
		CHECK_EQ(ui.confirm, CONFIRM_REPLACE);
		CHECK_EQ(ui.scr, SCR_NEWGAME);
		edge("new game replace prompt");
		ui.confirm = CONFIRM_NONE;
	}
}

static void sweep_menu_and_end(void)
{
	static const EndReason R[] = {END_DAYS_UP, END_DEAD, END_QUIT};
	int i, r, tab;

	for (i = 0; i < MENU_ROWS; i++) {
		menu_open(&ui, SCR_MARKET);
		ui.menu_row = i;
		edge("menu");
		ui.confirm = CONFIRM_END;
		edge("menu confirm");
	}
	ui.confirm = CONFIRM_NONE;
	for (r = 0; r < 3; r++) {
		app.g.phase = PH_OVER;
		app.g.end = R[r];
		for (i = 0; i < 4; i++) {
			app.g.last_day = (int16_t)(i == 0 ? 15 : i == 1 ? 30 : i == 2 ? 60 : 90);
			app.last_rank = i == 3 ? -1 : 9;
			go(&ui, SCR_OVER);
			edge("over");
		}
	}
	app.g.phase = PH_TRADING;
	app.g.end = END_NONE;
	for (tab = 0; tab < 4; tab++)
		for (i = 0; i < 10; i++)
			hiscore_insert(&app.scores, tab == 0 ? 15 : tab == 1 ? 30 : tab == 2 ? 60 : 90,
				       "WWWWWWWWWWWWWWW", i % 2 ? MONEY_MAX : -MONEY_MAX + i, R[i % 3]);
	for (tab = 0; tab < 4; tab++) {
		ui.scores_tab = tab;
		ui.scores_back = SCR_TITLE;
		app.last_rank = 3;
		go(&ui, SCR_SCORES);
		edge("scores");
	}
}

static void sweep_news_and_chase(void)
{
	int i;

	all_kinds_msgs();
	ui_collect_news(&ui, SCR_MARKET);
	for (i = 0; i < ui.news.n; i++) {
		ui.news.i = i;
		edge("news");
	}
	{
		int k;

		for (k = OFFER_RC; k <= OFFER_CREW; k++) {	/* the offer cards, with their choices */
			app.g.offer.kind = (uint8_t)k;
			app.g.offer.chem = 8;
			app.g.offer.day = app.g.day;
			all_kinds_msgs();
			ui_collect_news(&ui, SCR_MARKET);
			for (i = 0; i < ui.news.n; i++) {
				ui.news.i = i;
				edge(k == OFFER_RC ? "news with a chemical offer" : "news with a crew offer");
			}
			if (k == OFFER_CREW) {
				for (i = 0; i < ui.news.n; i++)
					if (ui.news.cards[i].choice == CHOICE_CREW) {
						ui.news.i = i;
						ui.news.ask = 1;
						ui.news.yes = 0;
						edge("crew prompt, NO");
						ui.news.yes = 1;
						edge("crew prompt, YES");
					}
				ui.news.ask = 0;
			}
			if (k == OFFER_RC) {
				picker_open(&ui, PK_STAKE, 8, SCR_MARKET);
				edge("stake dial at zero");
				ui.pk.q = picker_max(&ui);
				edge("stake dial at max");
				ui.pk.sel = 15;
				edge("stake dial, last digit");
			}
		}
		app.g.offer.kind = OFFER_NONE;
	}
	app.g.phase = PH_CHASE;
	app.g.chase.posse = 99;
	app.g.chase.cops_left = 99;
	go(&ui, SCR_CHASE);
	chase_begin(&ui);
	for (i = 0; i < 3; i++) {
		ui.row = i;
		edge("chase");
	}
	memset(ui.chase_log, 'W', sizeof ui.chase_log - 1);
	ui.chase_log[sizeof ui.chase_log - 1] = '\0';
	edge("chase with the longest log");
	{
		int k;

		app.g.phase = PH_TRADING;
		for (k = CR_ESCAPED; k <= CR_DEAD; k++) {
			ui.chase_done = k;
			edge("chase result");
		}
	}
	app.g.phase = PH_TRADING;
	ui.chase_done = CR_NONE;
}

static void run_state(const char *label, const char *city, int hood, int big_price)
{
	tag = label;
	extreme(city, hood, big_price);
	sweep_screens();
	sweep_menu_and_end();
	sweep_news_and_chase();
}

static void test_every_screen_extreme(void)
{
	int city, big;

	for (city = 0; city < NUM_CITIES; city++)
		for (big = 0; big < 2; big++) {
			char label[64];

			snprintf(label, sizeof label, "%s, %s prices", CITIES[city].name, big ? "big" : "tiny");
			run_state(label, CITIES[city].name, city % HOODS_PER_CITY, big);
		}
}

static void test_negative_net_worth_over(void)
{
	tag = "net worth -MONEY_MAX";
	extreme("New York City", 4, 1);
	app.g.cash = 0;
	app.g.bank = 0;
	app.g.debt[0] = MONEY_MAX;
	app.g.debt[1] = MONEY_MAX;
	app.g.stats.best_net_worth = -MONEY_MAX;
	app.g.phase = PH_OVER;
	app.g.end = END_DAYS_UP;
	app.last_rank = -1;
	go(&ui, SCR_OVER);
	edge("over");
	app.last_rank = 0;
	edge("over ranked");
	app.g.end = END_DEAD;
	edge("over dead");
}

/* The stats grid has 7 cells of room for the ATM fees value beside its label. */
static void test_over_fees_fit_their_cell(void)
{
	static const int64_t V[] = {0, 999, 12345, 9999999999LL, 10000000000LL, 5000000000000LL, 999000000000000LL, MONEY_MAX};
	StatCell cells[12];
	size_t i;

	ui_test_setup(&app, &ui, 30);
	for (i = 0; i < sizeof V / sizeof V[0]; i++) {
		app.g.stats.bank_fees_paid = V[i];
		CHECK_EQ(over_stats(&app.g, cells), 12);
		CHECK_STR(cells[9].label, "ATM fees");
		CHECK(strlen(cells[9].value) <= 7);
	}
}

static void test_held_picker_stops_at_max(void)
{
	Repeat r;
	uint32_t t, fired;

	ui_test_setup(&app, &ui, 30);
	app.g.price[0] = 1000;
	app.g.cash = 7500;
	app.g.capacity = 100;
	picker_open(&ui, PK_BUY, 0, SCR_MARKET);
	CHECK_EQ(picker_max(&ui), 7);
	memset(&r, 0, sizeof r);
	for (t = 0; t <= 3000; t += 16) {
		fired = input_step(&r, PAD_BIT(PAD_UP), t);
		if (fired & PAD_BIT(PAD_UP))
			ui_press(&ui, PAD_UP);
		CHECK(ui.pk.q <= 7);
	}
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.q, 7);
	/* and held DOWN comes back to exactly 0, held RIGHT (10s) lands on the max */
	memset(&r, 0, sizeof r);
	for (t = 0; t <= 3000; t += 16) {
		fired = input_step(&r, PAD_BIT(PAD_DOWN), t);
		if (fired & PAD_BIT(PAD_DOWN))
			ui_press(&ui, PAD_DOWN);
	}
	CHECK_EQ(ui.pk.q, 0);
	memset(&r, 0, sizeof r);
	for (t = 0; t <= 3000; t += 16) {
		fired = input_step(&r, PAD_BIT(PAD_RIGHT), t);
		if (fired & PAD_BIT(PAD_RIGHT))
			ui_press(&ui, PAD_RIGHT);
	}
	CHECK_EQ(ui.pk.q, 7);
}

static void test_every_hit_inside_canvas(void)
{
	/* the edge() rule, on ordinary values too */
	tag = "ordinary game";
	ui_test_setup(&app, &ui, 30);
	sweep_screens();
	sweep_menu_and_end();
	sweep_news_and_chase();
}

static void test_full_region_list_counts_drops(void)
{
	int i;

	ui_test_setup(&app, &ui, 30);
	ui_draw(&ui, &cv);
	CHECK_EQ(ui.hits_dropped, 0);
	ui.nhits = 0;
	for (i = 0; i < 70; i++)
		hit(&ui, 0, 0, 4, 4, HIT_PAD, PAD_A);
	CHECK_EQ(ui.nhits, 64);
	CHECK_EQ(ui.hits_dropped, 6);
	ui_draw(&ui, &cv);
	CHECK_EQ(ui.hits_dropped, 0);
}

void tests_ui_edges(void)
{
	test_every_screen_extreme();
	test_negative_net_worth_over();
	test_over_fees_fit_their_cell();
	test_held_picker_stops_at_max();
	test_every_hit_inside_canvas();
	test_full_region_list_counts_drops();
}
