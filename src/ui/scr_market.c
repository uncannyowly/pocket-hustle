/* The market: the mockup's drawMarket, inMarket, buyGood, sellGood and doAction. The top
 * band is the day and health, the city skyline, then cash, bank and debt; below it the
 * MARKET and BAG panels and the six action buttons. L1/R1 cycle the three. LEFT and RIGHT
 * never cross between MARKET and BAG: A buys in one and sells in the other, so a sideways
 * press must not quietly turn the next A into the opposite trade. */
#include <stdio.h>

#include "gfx/art.h"
#include "gfx/palette.h"
#include "ui/input.h"
#include "ui/ui_internal.h"

static const char *const ACT_LABEL[NUM_MKT_ACTS] = {"MOVE", "FLY", "BANK", "LOAN", "SHOP", "DOC"};
static const Hint BUY_HINTS[] = {{"A", "Buy"}, {"SEL", "Chart"}, {"L1R1", "Panel"}, {"START", "Menu"}};
static const Hint SELL_HINTS[] = {{"A", "Sell"}, {"SEL", "Chart"}, {"L1R1", "Panel"}, {"START", "Menu"}};
static const Hint ACT_HINTS[] = {{"A", "Go"}, {"L1R1", "Panel"}, {"START", "Menu"}};

int bag_list(const Game *g, int out[NUM_GOODS])
{
	int i, n = 0;

	for (i = 0; i < NUM_GOODS; i++)
		if (g->stash[i] > 0)
			out[n++] = i;
	return n;
}

static int64_t floor_div(int64_t a, int64_t b)	/* b > 0 */
{
	return a >= 0 ? a / b : -((-a + b - 1) / b);
}

int64_t stash_pct(const Game *g, int good, int *known)
{
	int64_t value, paid = g->stash_paid[good];

	*known = g->price[good] > 0 && g->stash[good] > 0 && paid > 0;
	if (!*known)
		return 0;
	value = (int64_t)g->price[good] * g->stash[good];	/* under 2^62 */
	if (value / paid >= 11)
		return 1000;					/* past the shown cap */
	while (paid > ((int64_t)1 << 50)) {			/* keeps 200 * value in range */
		value >>= 1;
		paid >>= 1;
	}
	/* Math.round((value - paid) * 100 / paid), the mockup's rounding */
	return floor_div(200 * (value - paid) + paid, 2 * paid);
}

int bag_pct(const Game *g, int good, char *buf, size_t n)
{
	int known;
	int64_t pct = stash_pct(g, good, &known);

	if (!known) {
		snprintf(buf, n, "--");
		return 0;
	}
	snprintf(buf, n, "%s%d%%", pct >= 0 ? "+" : "", (int)(pct > 999 ? 999 : pct < -99 ? -99 : pct));
	return pct >= 0 ? 1 : -1;
}

int price_flag(const Game *g, int good)
{
	int ev;

	if (good < 0 || good >= NUM_GOODS || g->price[good] <= 0 || g->day < 0 || g->day > MAX_DAYS)
		return PF_NORMAL;
	ev = g->hist_ev[good][g->day];		/* what the market roll applied today; saved with the game */
	return ev == EV_SPIKE ? PF_SPIKE : ev == EV_CRASH ? PF_CRASH : PF_NORMAL;
}

/* Keeps the highlights on something that exists, whatever changed underneath. */
static void clamp_rows(Ui *u, int held)
{
	if (u->panel < 0 || u->panel > 2)
		u->panel = 0;
	if (u->mrow < 0 || u->mrow >= NUM_GOODS)
		u->mrow = 0;
	if (u->act < 0 || u->act >= NUM_MKT_ACTS)
		u->act = 0;
	if (u->brow >= held)
		u->brow = held - 1;
	if (u->brow < 0)
		u->brow = 0;
}

static void draw_goods(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	char s[32];
	int i;

	panel(c, 0, 82, 168, 116, "MARKET", u->panel == 0, NULL);
	for (i = 0; i < NUM_GOODS; i++) {
		int y = 96 + i * 10, sel = u->mrow == i, on = sel && u->panel == 0, flag = price_flag(g, i);
		int32_t p = g->price[i];
		uint32_t pc = !p ? PAL_FAINT : on ? PAL_WHITE : flag == PF_SPIKE ? PAL_HOT :
			      flag == PF_CRASH ? PAL_MINT : PAL_TEXT;

		if (sel)
			cv_rect(c, 2, y - 1, 164, 10, on ? PAL_HOT : PAL_PANEL2);
		tx(c, GOODS[i].name, 6, y, on ? PAL_WHITE : p ? PAL_TEXT : PAL_FAINT, 1, TX_LEFT);
		if (p)
			fmt_money_short(p, s, sizeof s);
		else
			snprintf(s, sizeof s, "--");
		tx(c, s, 154, y, pc, 1, TX_RIGHT);
		if (p && flag != PF_NORMAL)
			art_icon(c, flag == PF_SPIKE ? ICON_UP : ICON_DOWN, 158, y + 1, 1,
				 on ? PAL_WHITE : flag == PF_SPIKE ? PAL_HOT : PAL_MINT);
		hit(u, 2, y - 1, 164, 10, HIT_MARKET_ROW, i);
	}
}

enum { WILD_MAX = 17 };		/* characters that fit along the bag panel's bottom line */

void bag_wild_line(const Game *g, char *buf, size_t n)
{
	int count = g->nstakes < MAX_STAKES ? g->nstakes : MAX_STAKES, k, i;

	if (!n)
		return;
	buf[0] = '\0';
	if (count <= 0)
		return;
	for (k = count; k >= 1; k--) {		/* as many names as fit, then how many more */
		char line[48];
		size_t len = (size_t)snprintf(line, sizeof line, "WILD: ");

		for (i = 0; i < k && len < sizeof line; i++) {
			int chem = g->stakes[i].chem;

			if (chem < 0 || chem >= NUM_CHEMS)
				chem = 0;
			len += (size_t)snprintf(line + len, sizeof line - len, "%s%s", i ? ", " : "", CHEMS[chem].name);
		}
		if (k < count && len < sizeof line)
			len += (size_t)snprintf(line + len, sizeof line - len, " +%d", count - k);
		if (len <= WILD_MAX) {
			snprintf(buf, n, "%s", line);
			return;
		}
	}
	snprintf(buf, n, "WILD: %d RUNNING", count);
}

static void draw_bag(Ui *u, Canvas *c, const int *held, int nh)
{
	const Game *g = &u->app->g;
	char s[32];
	int i;

	snprintf(s, sizeof s, "%d/%d", (int)game_carried(g), (int)g->capacity);
	panel(c, 168, 82, 152, 116, "BAG", u->panel == 1, s);
	if (!nh) {
		tx(c, "Empty.", 174, 96, PAL_FAINT, 1, TX_LEFT);
		tx(c, "Buy low.", 174, 106, PAL_FAINT, 1, TX_LEFT);
	}
	for (i = 0; i < nh; i++) {
		int good = held[i], y = 96 + i * 10, on = u->brow == i && u->panel == 1, dir;

		if (u->brow == i)
			cv_rect(c, 170, y - 1, 148, 10, on ? PAL_HOT : PAL_PANEL2);
		tx(c, GOODS[good].name, 172, y, on ? PAL_WHITE : PAL_TEXT, 1, TX_LEFT);
		snprintf(s, sizeof s, "%d", (int)g->stash[good]);
		tx(c, s, 268, y, on ? PAL_WHITE : PAL_GOLD, 1, TX_RIGHT);
		dir = bag_pct(g, good, s, sizeof s);
		tx(c, s, 316, y, on ? PAL_WHITE : dir > 0 ? PAL_UP : dir < 0 ? PAL_DOWN : PAL_FAINT, 1, TX_RIGHT);
		hit(u, 170, y - 1, 148, 10, HIT_BAG_ROW, i);
	}
	bag_wild_line(g, s, sizeof s);
	if (nh <= 8 && s[0]) {		/* running wild cards take the weapon's line */
		tx(c, s, 173, 186, PAL_HOT, 1, TX_LEFT);
	} else if (nh <= 8) {		/* a ninth line would sit on the weapon line */
		art_icon(c, ICON_BOLT, 173, 186, 1, 0);
		tx(c, g->weapon >= 0 && g->weapon < NUM_GEAR ? GEAR[g->weapon].name : "No weapon",
		   182, 186, PAL_DIM, 1, TX_LEFT);
	}
}

void market_draw(Ui *u, Canvas *c)
{
	const Game *g = &u->app->g;
	int held[NUM_GOODS], nh = bag_list(g, held), i;
	int city = g->loc.city < NUM_CITIES ? g->loc.city : 0;
	int hood = g->loc.hood < HOODS_PER_CITY ? g->loc.hood : 0;

	clamp_rows(u, nh);
	top_bar(u, c, NULL, NULL);
	art_backdrop(c, city, 12, 56, u->frame);
	place_label(c, city, CITIES[city].hoods[hood], 68);
	money_strip(u, c, 68);
	draw_goods(u, c);
	draw_bag(u, c, held, nh);
	cv_rect(c, 0, 198, CV_W, 26, PAL_NIGHT);
	for (i = 0; i < NUM_MKT_ACTS; i++)
		button(u, c, 2 + i * 53, 201, 51, 18, ACT_LABEL[i], u->panel == 2 && u->act == i,
		       HIT_MARKET_ACT, i);
	if (u->panel == 2)
		hints(c, ACT_HINTS, 3, HINTS_Y);
	else
		hints(c, u->panel == 0 ? BUY_HINTS : SELL_HINTS, 4, HINTS_Y);
}

static void buy_good(Ui *u, int good)
{
	char msg[64];

	if (u->app->g.price[good] <= 0) {
		snprintf(msg, sizeof msg, "Nobody's selling %s here today.", GOODS[good].name);
		toast(u, msg);
		play(u, SFX_DENY);
		return;
	}
	picker_open(u, PK_BUY, good, SCR_MARKET);
}

static void sell_good(Ui *u, int good)
{
	char msg[64];

	if (u->app->g.price[good] <= 0) {
		snprintf(msg, sizeof msg, "Nobody's buying %s here today.", GOODS[good].name);
		toast(u, msg);
		play(u, SFX_DENY);
		return;
	}
	picker_open(u, PK_SELL, good, SCR_MARKET);
}

static void do_action(Ui *u, int act)
{
	if (act == MKT_MOVE) {
		move_open(u);
		return;
	}
	if (act == MKT_FLY) {
		fly_open(u);
		return;
	}
	if (act == MKT_BANK) {
		bank_open(u);
		return;
	}
	if (act == MKT_LOAN) {
		lender_open(u);
		return;
	}
	if (act == MKT_SHOP) {
		store_open(u);
		return;
	}
	if (u->app->g.health >= 100) {
		toast(u, "You're in perfect health.");
		play(u, SFX_DENY);
		return;
	}
	picker_open(u, PK_HEAL, -1, SCR_MARKET);
}

void market_press(Ui *u, int pad)
{
	int held[NUM_GOODS], nh = bag_list(&u->app->g, held);

	clamp_rows(u, nh);
	if (pad == PAD_L1 || pad == PAD_R1) {
		u->panel = (u->panel + (pad == PAD_L1 ? 2 : 1)) % 3;
		play(u, SFX_MOVE);
		return;
	}
	if (pad == PAD_SELECT) {	/* the price chart of the highlighted good */
		chart_open(u, u->panel == 1 && nh ? held[u->brow] : u->mrow);
		return;
	}
	if (u->panel == 0) {
		if (pad == PAD_UP)
			u->mrow = (u->mrow + NUM_GOODS - 1) % NUM_GOODS;
		else if (pad == PAD_DOWN && u->mrow == NUM_GOODS - 1)
			u->panel = 2;
		else if (pad == PAD_DOWN)
			u->mrow++;
		else if (pad == PAD_A) {
			buy_good(u, u->mrow);
			return;
		} else
			return;
	} else if (u->panel == 1) {
		if (!nh && pad == PAD_DOWN)
			u->panel = 2;
		else if (!nh)
			return;
		else if (pad == PAD_UP)
			u->brow = (u->brow + nh - 1) % nh;
		else if (pad == PAD_DOWN && u->brow == nh - 1)
			u->panel = 2;
		else if (pad == PAD_DOWN)
			u->brow++;
		else if (pad == PAD_A) {
			sell_good(u, held[u->brow]);
			return;
		} else
			return;
	} else {
		if (pad == PAD_LEFT)
			u->act = (u->act + NUM_MKT_ACTS - 1) % NUM_MKT_ACTS;
		else if (pad == PAD_RIGHT)
			u->act = (u->act + 1) % NUM_MKT_ACTS;
		else if (pad == PAD_UP)
			u->panel = 0;
		else if (pad == PAD_A) {
			do_action(u, u->act);
			return;
		} else
			return;
	}
	play(u, SFX_MOVE);
}

void market_click(Ui *u, int action, int arg)
{
	if (action == HIT_MARKET_ROW) {
		u->panel = 0;
		u->mrow = arg;
	} else if (action == HIT_BAG_ROW) {
		u->panel = 1;
		u->brow = arg;
	} else if (action == HIT_MARKET_ACT) {
		u->panel = 2;
		u->act = arg;
	} else
		return;
	ui_press(u, PAD_A);
}
