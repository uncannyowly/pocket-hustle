/* Loan sharks (one per city) and the bank (works anywhere, safe from muggers, free at a
 * branch and an out-of-network fee everywhere else). */
#include "core/game_internal.h"

int game_borrow(Game *g, int64_t amt)
{
	int c = g->loc.city;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (amt <= 0)
		return E_BAD_ARG;
	if (amt > CITIES[c].lender_max - g->debt[c])
		return E_LIMIT;

	game_clear_msgs(g);
	if (g->debt[c] == 0)
		g->debt_age[c] = 0;
	g->debt[c] += amt;
	g->cash += amt;
	if (g->cash > MONEY_MAX)
		g->cash = MONEY_MAX;
	return OK;
}

int game_repay(Game *g, int64_t amt)
{
	int c = g->loc.city;
	int64_t needed;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (amt <= 0 || amt > g->debt[c])
		return E_BAD_ARG;
	if (amt > g->cash)
		return E_NO_CASH;

	game_clear_msgs(g);
	needed = g->debt[c] * g->p.min_payment_pct / 100;
	if (needed < 1)
		needed = 1;
	if (amt == g->debt[c] || amt >= needed)
		g->debt_age[c] = 0;
	g->debt[c] -= amt;
	g->cash -= amt;
	return OK;
}

int game_bank_branch_here(const Game *g)
{
	return (CITIES[g->loc.city].bank_hoods >> g->loc.hood) & 1;
}

int64_t game_bank_fee(const Game *g, int64_t amt)
{
	int64_t fee;

	if (amt <= 0 || game_bank_branch_here(g))
		return 0;
	fee = amt * g->p.bank_fee_pct / 100;
	return fee < g->p.bank_fee_min ? g->p.bank_fee_min : fee;
}

static void pay_fee(Game *g, int64_t fee)
{
	g->stats.bank_fees_paid += fee;
	if (g->stats.bank_fees_paid > MONEY_MAX)
		g->stats.bank_fees_paid = MONEY_MAX;
}

int game_deposit(Game *g, int64_t amt)
{
	int64_t fee;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (amt <= 0)
		return E_BAD_ARG;
	if (amt > g->cash)
		return E_NO_CASH;
	fee = game_bank_fee(g, amt);
	if (amt <= fee)
		return E_FEE;
	game_clear_msgs(g);
	g->cash -= amt;
	g->bank += amt - fee;
	if (g->bank > MONEY_MAX)
		g->bank = MONEY_MAX;
	pay_fee(g, fee);
	return OK;
}

int game_withdraw(Game *g, int64_t amt)
{
	int64_t fee;

	if (g->phase != PH_TRADING)
		return E_PHASE;
	if (amt <= 0)
		return E_BAD_ARG;
	if (amt > g->bank)
		return E_NO_CASH;
	fee = game_bank_fee(g, amt);
	if (amt <= fee)
		return E_FEE;
	game_clear_msgs(g);
	g->bank -= amt;
	g->cash += amt - fee;
	if (g->cash > MONEY_MAX)
		g->cash = MONEY_MAX;
	pay_fee(g, fee);
	return OK;
}
