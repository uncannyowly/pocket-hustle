/* ph-text commands: parse a line, call the rules, print what happened. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "content/text.h"
#include "text/commands.h"

static const char *const help_text =
	"Commands:\n"
	"  status | market | flights | store | scores | help | quit\n"
	"  buy <good#> <qty|max>     sell <good#> <qty|all>\n"
	"  go <hood#>                fly <city#>\n"
	"  borrow <amt>              repay <amt|all>\n"
	"  deposit <amt|all>         withdraw <amt|all>\n"
	"  gear <item#>              heal <hp|all>\n"
	"  run | fight | surrender   (when the cops show up)\n"
	"  chart <good#>             (prices you've seen)\n";

static void money(int64_t v, char *buf, size_t n)
{
	fmt_money(v, buf, n);
}

static int64_t total_debt(const Game *g)
{
	int64_t d = 0;
	int c;
	for (c = 0; c < NUM_CITIES; c++)
		d += g->debt[c];
	return d;
}

static void print_market(const Game *g)
{
	char p[32];
	int i;

	printf(" # %-10s %9s %6s\n", "Good", "Price", "Have");
	for (i = 0; i < NUM_GOODS; i++) {
		if (g->price[i] > 0)
			money(g->price[i], p, sizeof p);
		else
			strcpy(p, "-");
		printf("%2d %-10s %9s %6d\n", i, GOODS[i].name, p, (int)g->stash[i]);
	}
}

void ph_print_status(const Ctx *c)
{
	const Game *g = &c->g;
	char cash[32], bank[32], owed[32];
	int h;

	money(g->cash, cash, sizeof cash);
	money(g->bank, bank, sizeof bank);
	money(total_debt(g), owed, sizeof owed);
	printf("Day %d/%d  %s: %s  Health %d\n", g->day, g->last_day,
	       CITIES[g->loc.city].name, CITIES[g->loc.city].hoods[g->loc.hood], g->health);
	printf("Cash %s  Bank %s  Owed %s\n", cash, bank, owed);
	if (game_bank_branch_here(g)) {
		printf("Your bank has a branch here: no fees.\n");
	} else {
		char fmin[32];
		money(g->p.bank_fee_min, fmin, sizeof fmin);
		printf("No branch here: the ATM charges %d%% (at least %s).\n", (int)g->p.bank_fee_pct, fmin);
	}
	printf("Carrying %d/%d", (int)game_carried(g), (int)g->capacity);
	if (g->weapon >= 0)
		printf("  Packing: %s", GEAR[g->weapon].name);
	printf("\n");
	print_market(g);
	printf("Neighborhoods ($ = bank branch):");
	for (h = 0; h < HOODS_PER_CITY; h++)
		printf(" %d %s%s%s", h, CITIES[g->loc.city].hoods[h],
		       (CITIES[g->loc.city].bank_hoods >> h) & 1 ? " $" : "", h + 1 < HOODS_PER_CITY ? "," : "");
	printf("\n");
}

void ph_print_msgs(const Ctx *c)
{
	char buf[256];
	int i;

	for (i = 0; i < c->g.nmsgs; i++) {
		text_msg(&c->g, &c->g.msgs[i], buf, sizeof buf);
		printf("%s\n", buf);
	}
	if (c->g.phase == PH_CHASE)
		printf("The cops are on you: run, fight or surrender?\n");
}

static void print_err(int rc)
{
	switch (rc) {
	case E_NOT_SOLD: printf("Not sold here.\n"); break;
	case E_NO_CASH:  printf("Not enough cash.\n"); break;
	case E_NO_SPACE: printf("No room.\n"); break;
	case E_NO_STOCK: printf("You don't have that many.\n"); break;
	case E_PHASE:    printf("Can't do that now.\n"); break;
	case E_LIMIT:    printf("That's over the limit.\n"); break;
	case E_FEE:      printf("That won't even cover the ATM fee.\n"); break;
	default:         printf("Huh?\n"); break;
	}
}

/* Parses a whole non-negative number. Returns 1 on success. */
static int parse_num(const char *s, int64_t *out)
{
	char *end;
	long long v;

	if (!s || !*s || !isdigit((unsigned char)*s))
		return 0;
	v = strtoll(s, &end, 10);
	if (*end || v < 0)
		return 0;
	*out = (int64_t)v;
	return 1;
}

static void print_flights(const Game *g)
{
	char p[32];
	int c;

	for (c = 0; c < NUM_CITIES; c++) {
		if (c == g->loc.city)
			continue;
		money(game_flight_cost(g, c), p, sizeof p);
		printf("%d %-14s %s (lands at %s)\n", c, CITIES[c].name, p, CITIES[c].hoods[0]);
	}
}

static void print_store(const Game *g)
{
	char p[32];
	int i;

	for (i = 0; i < NUM_GEAR; i++) {
		const char *note = "";
		if (!game_gear_available(g, i))
			continue;
		if (GEAR[i].kind == GEAR_SPACE ? g->gear_owned[i] != 0 : g->weapon == i)
			note = "  (yours)";
		money(GEAR[i].price, p, sizeof p);
		printf("%2d %-16s %9s  %s +%d%s\n", i, GEAR[i].name, p,
		       GEAR[i].kind == GEAR_WEAPON ? "aim" : "room", (int)GEAR[i].power, note);
	}
}

static void print_chart(const Game *g, int good)
{
	char p[32];
	int d;

	printf("%s, prices you've seen:\n", GOODS[good].name);
	for (d = 1; d <= g->day; d++) {
		if (g->hist[good][d] > 0)
			money(g->hist[good][d], p, sizeof p);
		else
			strcpy(p, "-");
		printf("  day %2d  %s\n", d, p);
	}
}

static void print_scores(const Ctx *c)
{
	char p[32];
	int k = days_index(c->g.last_day), i;

	printf("High scores, %d days:\n", c->g.last_day);
	if (k < 0 || c->scores.n[k] == 0) {
		printf("  (none yet)\n");
		return;
	}
	for (i = 0; i < c->scores.n[k]; i++) {
		money(c->scores.t[k][i].score, p, sizeof p);
		printf("  %2d. %-15s %s%s\n", i + 1, c->scores.t[k][i].name, p,
		       c->scores.t[k][i].end == END_DEAD ? "  (dead)" : "");
	}
}

static void report_game_over(Ctx *c)
{
	const Game *g = &c->g;
	const Stats *s = &g->stats;
	char nw[32];
	int rank;

	if (c->over_reported)
		return;
	c->over_reported = 1;
	money(game_net_worth(g), nw, sizeof nw);
	printf("Game over: %s.\n", g->end == END_DEAD ? "you died" :
	       g->end == END_QUIT ? "you retired early" : "time's up");
	printf("Final net worth: %s\n", nw);
	printf("Moves %d, flights %d, cop chases %d (beaten %d, escaped %d), jailed %d, mugged %d, goons %d\n",
	       s->moves, s->flights, s->cop_chases, s->cops_beaten, s->escapes, s->times_jailed,
	       s->times_mugged, s->goon_visits);
	rank = hiscore_insert(&c->scores, g->last_day, g->name, game_net_worth(g), g->end);
	if (rank >= 0)
		printf("High score! You're #%d.\n", rank + 1);
	else
		printf("Not a high score this time.\n");
	if (c->scores_path && hiscores_write(&c->scores, c->scores_path) != 0)
		printf("(Couldn't save high scores.)\n");
	if (c->save_path)
		unlink(c->save_path);
}

int ph_command(Ctx *c, const char *line)
{
	Game *g = &c->g;
	char verb[32] = "", a1[64] = "", a2[64] = "";
	int16_t day = g->day;
	Phase phase = g->phase;
	int64_t n = 0, m = 0, fee = 0;
	int rc = OK, handled = 1;

	if (sscanf(line, "%31s %63s %63s", verb, a1, a2) < 1)
		return 0;

	if (!strcmp(verb, "quit")) {
		if (g->phase != PH_OVER)
			game_end(g);
		report_game_over(c);
		return 1;
	} else if (!strcmp(verb, "help")) {
		printf("%s", help_text);
		return 0;
	} else if (!strcmp(verb, "scores")) {
		print_scores(c);
		return 0;
	} else if (g->phase == PH_OVER) {
		printf("The game is over. Type quit.\n");
		return 0;
	}

	if (!strcmp(verb, "status")) {
		ph_print_status(c);
		return 0;
	} else if (!strcmp(verb, "market")) {
		print_market(g);
		return 0;
	} else if (!strcmp(verb, "flights")) {
		print_flights(g);
		return 0;
	} else if (!strcmp(verb, "store")) {
		print_store(g);
		return 0;
	} else if (!strcmp(verb, "chart")) {
		if (parse_num(a1, &n) && n < NUM_GOODS)
			print_chart(g, (int)n);
		else
			print_err(E_BAD_ARG);
		return 0;
	} else if (!strcmp(verb, "buy") || !strcmp(verb, "sell")) {
		int buy = verb[0] == 'b';
		if (!parse_num(a1, &n) || n >= NUM_GOODS) {
			rc = E_BAD_ARG;
		} else {
			int good = (int)n;
			if (!strcmp(a2, buy ? "max" : "all"))
				m = buy ? game_max_buy(g, good) : g->stash[good];
			else if (!parse_num(a2, &m) || m > INT32_MAX)
				m = -1;
			if (m == 0 && strcmp(a2, "0") != 0)
				m = 1;	/* "max"/"all" of nothing: let the rules say why */
			rc = m < 0 ? E_BAD_ARG : buy ? game_buy(g, good, (int32_t)m) : game_sell(g, good, (int32_t)m);
			if (rc == OK) {
				char p[32];
				money((int64_t)m * g->price[good], p, sizeof p);
				printf("%s %d %s for %s.\n", buy ? "Bought" : "Sold", (int)m, GOODS[good].name, p);
			}
		}
	} else if (!strcmp(verb, "go") || !strcmp(verb, "fly")) {
		Loc dest = g->loc;
		if (!parse_num(a1, &n) || n > 255) {
			rc = E_BAD_ARG;
		} else {
			if (verb[0] == 'g') {
				dest.hood = (uint8_t)n;
			} else {
				dest.city = (uint8_t)n;
				dest.hood = 0;
			}
			rc = game_move(g, dest);
		}
	} else if (!strcmp(verb, "borrow")) {
		const char *lender = CITIES[g->loc.city].lender;
		rc = parse_num(a1, &n) ? game_borrow(g, n) : E_BAD_ARG;
		if (rc == OK) {
			char p[32];
			money(n, p, sizeof p);
			printf("Borrowed %s from %s.\n", p, lender);
		}
	} else if (!strcmp(verb, "repay")) {
		const char *lender = CITIES[g->loc.city].lender;
		int64_t owed = g->debt[g->loc.city];
		if (owed == 0) {
			printf("You don't owe anyone here.\n");
			return 0;
		}
		if (!strcmp(a1, "all")) {
			n = owed < g->cash ? owed : g->cash;
			rc = game_repay(g, n > 0 ? n : owed);
		} else {
			rc = parse_num(a1, &n) ? game_repay(g, n) : E_BAD_ARG;
		}
		if (rc == OK) {
			char p[32], left[32];
			money(n, p, sizeof p);
			money(g->debt[g->loc.city], left, sizeof left);
			if (g->debt[g->loc.city] == 0)
				printf("Paid %s %s. You're square with them.\n", lender, p);
			else
				printf("Paid %s %s. You still owe %s.\n", lender, p, left);
		}
	} else if (!strcmp(verb, "deposit") || !strcmp(verb, "withdraw")) {
		int dep = verb[0] == 'd';
		if (!strcmp(a1, "all")) {
			n = dep ? g->cash : g->bank;
			if (n == 0) {
				printf(dep ? "You have no cash to deposit.\n" : "Your bank account is empty.\n");
				return 0;
			}
		} else if (!parse_num(a1, &n)) {
			n = -1;
		}
		fee = n > 0 ? game_bank_fee(g, n) : 0;
		rc = n < 0 ? E_BAD_ARG : dep ? game_deposit(g, n) : game_withdraw(g, n);
		if (rc == E_NO_CASH && !dep) {
			printf("Not enough in the bank.\n");
			return 0;
		}
		if (rc == OK) {
			char p[32], bank[32];
			money(n, p, sizeof p);
			money(g->bank, bank, sizeof bank);
			if (fee > 0) {
				char f[32];
				money(fee, f, sizeof f);
				printf("%s %s, minus a %s out-of-network fee. Bank balance %s.\n",
				       dep ? "Deposited" : "Withdrew", p, f, bank);
			} else {
				printf("%s %s. Bank balance %s.\n", dep ? "Deposited" : "Withdrew", p, bank);
			}
		}
	} else if (!strcmp(verb, "gear")) {
		rc = parse_num(a1, &n) && n < NUM_GEAR ? game_buy_gear(g, (int)n) : E_BAD_ARG;
	} else if (!strcmp(verb, "heal")) {
		if (!strcmp(a1, "all")) {
			n = 100 - g->health;
			if (g->p.hospital_per_hp > 0 && n * g->p.hospital_per_hp > g->cash)
				n = g->cash / g->p.hospital_per_hp;
			if (n <= 0)
				n = g->health >= 100 ? 1 : 100 - g->health;	/* let the rules say why */
		} else if (!parse_num(a1, &n) || n > 100) {
			n = -1;
		}
		rc = n < 0 ? E_BAD_ARG : game_heal(g, (int32_t)n);
	} else if (!strcmp(verb, "run")) {
		rc = game_chase(g, CH_RUN);
	} else if (!strcmp(verb, "fight")) {
		rc = game_chase(g, CH_FIGHT);
	} else if (!strcmp(verb, "surrender")) {
		rc = game_chase(g, CH_SURRENDER);
	} else {
		handled = 0;
	}

	if (!handled || rc != OK) {
		print_err(handled ? rc : E_BAD_ARG);
		return 0;
	}
	ph_print_msgs(c);
	if (g->phase == PH_OVER) {
		report_game_over(c);
		return 0;
	}
	if (g->day != day || g->phase != phase)
		ph_print_status(c);
	if (g->day != day && c->save_path && save_write(g, c->save_path) != 0)
		printf("(Couldn't save the game.)\n");
	return 0;
}
