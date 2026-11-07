/* The bank, the loan shark and the money dial. Positions come from the mockup's drawBank,
 * drawLender and drawPicker, worked out by hand. Text is read through the string builders. */
#include <stdio.h>
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

/* Oklahoma City: branches in hoods 0 and 1, none in hood 2. */
enum { BRANCH_HOOD = 0, ATM_HOOD = 2 };

static void open_bank(int hood, int64_t cash, int64_t bank)
{
	ui_test_setup(&app, &ui, 30);
	app.g.loc.city = 0;
	app.g.loc.hood = (uint8_t)hood;
	app.g.cash = cash;
	app.g.bank = bank;
	press_seq(&ui, "R1 R1 R R A");
	drain();
}

static void open_lender(int64_t cash, int64_t debt)
{
	ui_test_setup(&app, &ui, 30);
	app.g.loc.city = 0;
	app.g.cash = cash;
	app.g.debt[0] = debt;
	app.g.debt_age[0] = 0;
	press_seq(&ui, "R1 R1 R R R A");
	drain();
}

static void test_bank_opens(void)
{
	open_bank(BRANCH_HOOD, 500, 700);
	CHECK_EQ(ui.scr, SCR_BANK);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK(hit_find(&ui, HIT_MONEY_BTN, 0) >= 0);
	CHECK(hit_find(&ui, HIT_MONEY_BTN, 1) >= 0);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_MARKET);
	CHECK(heard(SFX_BACK));
}

static void test_bank_texts(void)
{
	BankText t;
	Params p;

	open_bank(BRANCH_HOOD, 0, 0);
	bank_text(&app.g, &t);
	CHECK_STR(t.head, "YOUR BRANCH");
	CHECK_STR(t.tag, "NO FEES");
	CHECK_STR(t.blurb, "Safe from muggers. Pays 4% a day.");
	CHECK_EQ(t.tag_col, PAL_MINT);

	app.g.loc.hood = ATM_HOOD;
	bank_text(&app.g, &t);
	CHECK_STR(t.head, "ATM");
	CHECK_STR(t.tag, "FEE 3%, MIN $5");
	CHECK_STR(t.blurb, "No branch here. The bank symbol on the map marks free banking.");
	CHECK_EQ(t.tag_col, PAL_CORAL);

	p = app.g.p;			/* the numbers come from the params */
	app.g.p.bank_fee_pct = 5;
	app.g.p.bank_fee_min = 20;
	app.g.p.bank_rate_bp = 250;
	bank_text(&app.g, &t);
	CHECK_STR(t.tag, "FEE 5%, MIN $20");
	app.g.loc.hood = BRANCH_HOOD;
	bank_text(&app.g, &t);
	CHECK_STR(t.blurb, "Safe from muggers. Pays 2.5% a day.");
	app.g.p = p;
}

static void test_bank_draws_vault_and_balances(void)
{
	open_bank(BRANCH_HOOD, 1234, 56789);
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK_EQ(px(56, 144 - 40 + 1), 0x3d4352u);	/* the vault door's rim, top */
	CHECK_EQ(px(8, 100), PAL_PANEL);			/* the panel under the skyline */
	/* DEPOSIT is highlighted at first, WITHDRAW is not */
	CHECK_EQ(px(113, 177), PAL_HOT);
	CHECK(px(215, 177) != PAL_HOT);
	press_seq(&ui, "R");
	ui_draw(&ui, &cv);
	CHECK(px(113, 177) != PAL_HOT);
	CHECK_EQ(px(215, 177), PAL_HOT);
}

static void test_bank_row_and_pickers(void)
{
	PickText t;

	open_bank(BRANCH_HOOD, 500, 700);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_DEPOSIT);
	CHECK_EQ(picker_max(&ui), 500);
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_BANK);
	press_seq(&ui, "D A");			/* any direction flips the row */
	CHECK_EQ(ui.pk.kind, PK_WITHDRAW);
	CHECK_EQ(picker_max(&ui), 700);
	picker_text(&ui, &t);
	CHECK_STR(t.title, "WITHDRAW");
	press_seq(&ui, "B U A");
	CHECK_EQ(ui.pk.kind, PK_DEPOSIT);
}

static void test_dial_digits(void)
{
	open_bank(BRANCH_HOOD, 3412, 0);
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(picker_max(&ui), 3412);
	press_seq(&ui, "L L L");
	CHECK_EQ(ui.pk.sel, 3);
	press_seq(&ui, "L");			/* the top digit is the highest */
	CHECK_EQ(ui.pk.sel, 3);
	press_seq(&ui, "U U U");
	CHECK_EQ(ui.pk.q, 3000);
	press_seq(&ui, "U");
	CHECK_EQ(ui.pk.q, 3412);
	press_seq(&ui, "R D");
	CHECK_EQ(ui.pk.sel, 2);
	CHECK_EQ(ui.pk.q, 3312);
	press_seq(&ui, "X");
	CHECK_EQ(ui.pk.q, 0);
	press_seq(&ui, "D");			/* never below zero */
	CHECK_EQ(ui.pk.q, 0);
	press_seq(&ui, "Y");
	CHECK_EQ(ui.pk.q, 3412);
	press_seq(&ui, "R R R R");
	CHECK_EQ(ui.pk.sel, 0);
	press_seq(&ui, "D");
	CHECK_EQ(ui.pk.q, 3411);
}

static void test_dial_digit_count(void)
{
	open_bank(BRANCH_HOOD, 0, 0);
	press_seq(&ui, "A");			/* max 0: one digit */
	press_seq(&ui, "L U");
	CHECK_EQ(ui.pk.sel, 0);
	CHECK_EQ(ui.pk.q, 0);
	press_seq(&ui, "A");			/* A at zero just closes */
	CHECK_EQ(ui.scr, SCR_BANK);
	CHECK_EQ(app.g.cash, 0);

	open_bank(BRANCH_HOOD, 7, 0);
	press_seq(&ui, "A L L");
	CHECK_EQ(ui.pk.sel, 0);
	press_seq(&ui, "U U");
	CHECK_EQ(ui.pk.q, 2);
}

static void test_dial_draws(void)
{
	open_bank(BRANCH_HOOD, 3412, 0);
	press_seq(&ui, "A L L L U U U");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	/* "$3,000" at 2x: 4 digits, 1 comma and the $ make 6 cells of 18, 108 wide, starting at
	 * x 36 + 70 = 106. The $ takes the first cell, the thousands digit the second (x 124),
	 * at y 48 + 32 = 80. */
	CHECK_EQ(px(123, 77), PAL_HOT);		/* the selected digit's box, rect(cx-2, ty-3) */
	CHECK(px(106 + 2, 80 + 2) != PAL_HOT);	/* the $ is not selected */
	CHECK_EQ(px(128, 73), PAL_GOLD);		/* the arrow above the digit */
	press_seq(&ui, "R");
	ui_draw(&ui, &cv);
	CHECK(px(123, 77) != PAL_HOT);
}

static void test_dial_clicks(void)
{
	open_bank(BRANCH_HOOD, 3412, 0);
	press_seq(&ui, "A");
	ui_draw(&ui, &cv);
	/* the thousands digit: upper half adds 1000, lower half takes it away */
	ui_click(&ui, 124 + 8, 80 - 8);
	CHECK_EQ(ui.pk.sel, 3);
	CHECK_EQ(ui.pk.q, 1000);
	ui_draw(&ui, &cv);
	ui_click(&ui, 124 + 8, 80 + 16 + 8);
	CHECK_EQ(ui.pk.q, 0);
	CHECK_EQ(ui.scr, SCR_PICKER);
}

static void test_deposit_at_branch_is_free(void)
{
	PickText t;

	open_bank(BRANCH_HOOD, 5000, 200);
	CHECK(game_bank_branch_here(&app.g));
	press_seq(&ui, "A L L L U");
	CHECK_EQ(ui.pk.q, 1000);
	picker_text(&ui, &t);
	CHECK_STR(t.unit, "BRANCH, NO FEE");
	CHECK_STR(t.info, "Branch here. No fee.");
	CHECK_STR(t.total, "BANK $200 > $1,200");
	press_seq(&ui, "A");
	CHECK_EQ(app.g.bank, 1200);
	CHECK_EQ(app.g.cash, 4000);
	CHECK_EQ(ui.scr, SCR_BANK);
	CHECK_STR(ui.toast, "Deposited $1,000.");
	CHECK(heard(SFX_CASH));
}

static void test_deposit_at_atm_pays_fee(void)
{
	PickText t;

	open_bank(ATM_HOOD, 5000, 200);
	CHECK(!game_bank_branch_here(&app.g));
	press_seq(&ui, "A L L L U");
	picker_text(&ui, &t);
	CHECK_STR(t.unit, "ATM FEE 3%");
	CHECK(strstr(t.info, "$30") != NULL);
	CHECK_STR(t.info, "Fee $30. 3%, at least $5.");
	CHECK_STR(t.total, "BANK $200 > $1,170");
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	press_seq(&ui, "A");
	CHECK_EQ(app.g.bank, 1170);
	CHECK_EQ(app.g.cash, 4000);
	CHECK_STR(ui.toast, "Deposited $1,000, minus a $30 ATM fee.");
}

static void test_withdraw(void)
{
	PickText t;

	open_bank(ATM_HOOD, 100, 9000);
	press_seq(&ui, "R A");
	CHECK_EQ(ui.pk.kind, PK_WITHDRAW);
	press_seq(&ui, "Y");
	CHECK_EQ(ui.pk.q, 9000);
	picker_text(&ui, &t);
	CHECK_STR(t.total, "CASH $100 > $8,830");
	CHECK_STR(t.info, "Fee $270. 3%, at least $5.");
	press_seq(&ui, "A");
	CHECK_EQ(app.g.bank, 0);
	CHECK_EQ(app.g.cash, 8830);
	CHECK_STR(ui.toast, "Took out $9,000, minus a $270 ATM fee.");

	open_bank(BRANCH_HOOD, 100, 900);
	press_seq(&ui, "R A Y A");
	CHECK_EQ(app.g.cash, 1000);
	CHECK_STR(ui.toast, "Took out $900.");
}

static void test_fee_text_follows_params(void)
{
	PickText t;

	open_bank(ATM_HOOD, 5000, 0);
	app.g.p.bank_fee_pct = 10;
	app.g.p.bank_fee_min = 40;
	press_seq(&ui, "A L L L U");
	picker_text(&ui, &t);
	CHECK_STR(t.unit, "ATM FEE 10%");
	CHECK_STR(t.info, "Fee $100. 10%, at least $40.");
	press_seq(&ui, "X R R U");		/* $10 costs the $40 minimum, so it can't be deposited */
	CHECK_EQ(ui.pk.q, 10);
	picker_text(&ui, &t);
	CHECK_STR(t.info, "Fee $40. 10%, at least $40.");
	CHECK_STR(t.total, "BANK $0 > $0");
}

static void test_fee_not_covered(void)
{
	open_bank(ATM_HOOD, 4, 0);
	press_seq(&ui, "A U U U U");
	CHECK_EQ(ui.pk.q, 4);
	snap();
	drain();
	press_seq(&ui, "A");
	CHECK(same_game(&before, &app.g));
	CHECK_STR(ui.toast, "That won't even cover the ATM fee.");
	CHECK(heard(SFX_DENY));
	CHECK_EQ(ui.scr, SCR_BANK);
}

static void test_picker_b_changes_nothing(void)
{
	open_bank(ATM_HOOD, 5000, 100);
	press_seq(&ui, "A L L L U");
	snap();
	press_seq(&ui, "B");
	CHECK_EQ(ui.scr, SCR_BANK);
	CHECK(same_game(&before, &app.g));
}

static void test_borrow_and_repay_all(void)
{
	PickText t;

	open_lender(100, 0);
	CHECK_EQ(ui.scr, SCR_LENDER);
	press_seq(&ui, "A");
	CHECK_EQ(ui.pk.kind, PK_BORROW);
	CHECK_EQ(picker_max(&ui), 15000);
	press_seq(&ui, "L L U U U U U");
	CHECK_EQ(ui.pk.q, 500);
	picker_text(&ui, &t);
	CHECK_STR(t.title, "BORROW");
	CHECK_STR(t.unit, "9.5% A DAY");
	CHECK_STR(t.total, "OWE $0 > $500");
	CHECK_STR(t.info, "Limit $15,000. $15,000 more.");
	CHECK_EQ(t.total_col, PAL_CORAL);
	press_seq(&ui, "A");
	CHECK_EQ(app.g.cash, 600);
	CHECK_EQ(app.g.debt[0], 500);
	CHECK_EQ(ui.scr, SCR_LENDER);
	CHECK_STR(ui.toast, "Tornado Terry hands you $500, smiling.");

	press_seq(&ui, "R A");
	CHECK_EQ(ui.pk.kind, PK_REPAY);
	CHECK_EQ(picker_max(&ui), 500);
	picker_text(&ui, &t);
	CHECK_STR(t.title, "REPAY");
	CHECK_STR(t.unit, "");
	CHECK_STR(t.info, "Pay 10%+ to reset the clock.");
	press_seq(&ui, "Y");
	CHECK_STR((picker_text(&ui, &t), t.total), "OWE $500 > $0");
	press_seq(&ui, "A");
	CHECK_EQ(app.g.debt[0], 0);
	CHECK_EQ(app.g.cash, 100);
	CHECK_STR(ui.toast, "Paid in full. Tornado Terry looks almost sad.");
}

static void test_repay_partial_and_max(void)
{
	open_lender(300, 1000);
	press_seq(&ui, "R A");
	CHECK_EQ(picker_max(&ui), 300);		/* cash is the limit */
	press_seq(&ui, "L L U");
	press_seq(&ui, "A");
	CHECK_EQ(app.g.debt[0], 900);
	CHECK_STR(ui.toast, "Paid $100. $900 to go.");

	open_lender(100, 14000);		/* borrow is limited by what the shark has left */
	press_seq(&ui, "A");
	CHECK_EQ(picker_max(&ui), 1000);
	open_lender(100, 15000);
	press_seq(&ui, "A");
	CHECK_EQ(picker_max(&ui), 0);
}

static void test_repay_nothing_owed(void)
{
	open_lender(100, 0);
	snap();
	press_seq(&ui, "R A");
	CHECK_EQ(ui.scr, SCR_LENDER);
	CHECK_STR(ui.toast, "You owe nothing here.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));
}

static void test_lender_texts(void)
{
	LenderText t;

	open_lender(0, 0);
	app.g.p.enforce_days = 8;
	lender_text(&app.g, &t);
	CHECK_STR(t.name, "TORNADO TERRY");
	CHECK_STR(t.rate, "9.5% A DAY");
	CHECK_STR(t.limit, "LIMIT $15,000");
	CHECK_STR(t.owed, "$0");
	CHECK_STR(t.paid, "");
	CHECK_STR(t.warn, "No loan in Oklahoma City. Each city has its own lender.");
	CHECK(!t.overdue);

	app.g.debt[0] = 2500;
	app.g.debt_age[0] = 1;
	lender_text(&app.g, &t);
	CHECK_STR(t.owed, "$2,500");
	CHECK_STR(t.paid, "LAST PAID 1 DAY AGO");
	CHECK_STR(t.warn, "Pay 10% within 8 days or Tornado Terry's crew comes looking.");
	app.g.debt_age[0] = 3;
	lender_text(&app.g, &t);
	CHECK_STR(t.paid, "LAST PAID 3 DAYS AGO");
	CHECK_STR(t.warn, "Pay 10% within 6 days or Tornado Terry's crew comes looking.");
	app.g.debt_age[0] = 8;
	lender_text(&app.g, &t);
	CHECK_STR(t.warn, "Pay 10% within 1 day or Tornado Terry's crew comes looking.");
	CHECK(!t.overdue);
	app.g.debt_age[0] = 9;
	lender_text(&app.g, &t);
	CHECK_STR(t.warn, "Tornado Terry's crew is out looking for you.");
	CHECK(t.overdue);

	app.g.p.min_payment_pct = 25;		/* tuned numbers show up */
	app.g.p.enforce_days = 4;
	app.g.debt_age[0] = 2;
	lender_text(&app.g, &t);
	CHECK_STR(t.warn, "Pay 25% within 3 days or Tornado Terry's crew comes looking.");
}

static void test_lender_draws_colors(void)
{
	open_lender(0, 0);
	app.g.p.enforce_days = 8;
	ui_draw(&ui, &cv);
	CHECK_EQ(cv.overflow, 0);
	CHECK_EQ(px(8, 76), PAL_LINE);		/* the portrait frame */
	CHECK_EQ(px(9, 192 + 1), PAL_HOT);	/* BORROW is the highlighted button at first */
	CHECK(px(163, 193) != PAL_HOT);
	press_seq(&ui, "L");
	ui_draw(&ui, &cv);
	CHECK_EQ(px(163, 193), PAL_HOT);
}

static void test_lender_screens_draw_clean(void)
{
	int city, age, d;

	for (city = 0; city < NUM_CITIES; city++)
		for (d = 0; d < 3; d++)
			for (age = 0; age <= 12; age += 4) {
				open_lender(100, 0);
				app.g.loc.city = (uint8_t)city;
				app.g.debt[city] = d == 0 ? 0 : d == 1 ? 1 : CITIES[city].lender_max;
				app.g.debt_age[city] = (int16_t)age;
				ui_draw(&ui, &cv);
				CHECK_EQ(cv.overflow, 0);
				CHECK(canvas_colors(&cv, 12) >= 12);
				press_seq(&ui, "A");	/* the borrow picker over it */
				ui_draw(&ui, &cv);
				CHECK_EQ(cv.overflow, 0);
			}
}

static void test_bank_screens_draw_clean(void)
{
	int city, hood;

	for (city = 0; city < NUM_CITIES; city++)
		for (hood = 0; hood < HOODS_PER_CITY; hood++) {
			open_bank(hood, MONEY_MAX, MONEY_MAX);
			app.g.loc.city = (uint8_t)city;
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
			press_seq(&ui, "A Y");		/* the biggest dial there is */
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
			press_seq(&ui, "L L L L L L L L L L L L L L L");
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
			press_seq(&ui, "B R A Y");
			ui_draw(&ui, &cv);
			CHECK_EQ(cv.overflow, 0);
		}
}

static void test_lender_and_bank_clicks(void)
{
	open_bank(BRANCH_HOOD, 500, 700);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_MONEY_BTN, 1));
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_WITHDRAW);

	open_lender(500, 0);
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_MONEY_BTN, 1));
	CHECK_EQ(ui.scr, SCR_LENDER);
	CHECK_STR(ui.toast, "You owe nothing here.");
	ui_draw(&ui, &cv);
	click_hit(&ui, hit_find(&ui, HIT_MONEY_BTN, 0));
	CHECK_EQ(ui.scr, SCR_PICKER);
	CHECK_EQ(ui.pk.kind, PK_BORROW);
}

static void test_core_refusals_toast(void)
{
	open_lender(100, 0);
	press_seq(&ui, "A Y");
	app.g.phase = PH_OVER;			/* the core says no */
	snap();
	drain();
	press_seq(&ui, "A");
	CHECK_EQ(ui.scr, SCR_LENDER);
	CHECK_STR(ui.toast, "You can't do that right now.");
	CHECK(heard(SFX_DENY));
	CHECK(same_game(&before, &app.g));

	open_lender(100, 0);			/* a limit that shrank under the dial clamps it */
	press_seq(&ui, "A Y");
	app.g.debt[0] = 15000;
	ui_draw(&ui, &cv);
	CHECK_EQ(ui.pk.q, 0);
}

void tests_ui_money(void)
{
	test_bank_opens();
	test_bank_texts();
	test_bank_draws_vault_and_balances();
	test_bank_row_and_pickers();
	test_dial_digits();
	test_dial_digit_count();
	test_dial_draws();
	test_dial_clicks();
	test_deposit_at_branch_is_free();
	test_deposit_at_atm_pays_fee();
	test_withdraw();
	test_fee_text_follows_params();
	test_fee_not_covered();
	test_picker_b_changes_nothing();
	test_borrow_and_repay_all();
	test_repay_partial_and_max();
	test_repay_nothing_owed();
	test_lender_texts();
	test_lender_draws_colors();
	test_lender_screens_draw_clean();
	test_bank_screens_draw_clean();
	test_lender_and_bank_clicks();
	test_core_refusals_toast();
}
