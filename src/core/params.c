/* Starting values: our own picks in the genre's general shape. Task 14 tunes them
 * with ph-sim. Change them here, never in the rules code. */
#include "core/game.h"
#include "core/params.h"

Params params_default(void)
{
	Params p;

	p.start_cash = 2600;
	p.start_debt = 4200;
	p.start_city = 0;		/* Oklahoma City */
	p.start_capacity = 150;

	p.min_goods = 5;
	p.absent_permille = 200;
	p.spike_permille = 30;
	p.crash_permille = 6;
	p.spike_lo_pct = 250;
	p.spike_hi_pct = 450;
	p.crash_lo_pct = 20;
	p.crash_hi_pct = 45;

	p.bank_rate_bp = 400;
	p.bank_fee_pct = 3;
	p.bank_fee_min = 5;
	p.min_payment_pct = 10;
	p.enforce_days = 8;
	p.enforce_dmg_lo = 12;
	p.enforce_dmg_hi = 26;

	p.cop_base_permille = 100;
	p.cop_per_10k_permille = 12;
	p.cop_max_permille = 400;
	p.posse_start = 1;
	p.posse_growth = 1;
	p.max_shooters = 3;
	p.run_permille = 550;
	p.cop_hit_permille = 300;
	p.cop_dmg_lo = 4;
	p.cop_dmg_hi = 12;
	p.loot_lo = 200;
	p.loot_hi = 2500;

	p.drop_permille = 300;
	p.drop_lo_pct = 20;
	p.drop_hi_pct = 60;
	p.jail_fine_pct = 25;

	p.mug_permille = 50;
	p.mug_lo_pct = 10;
	p.mug_hi_pct = 35;

	p.rumor_permille = 120;
	p.rumor_true_permille = 600;
	p.rumor_lead_lo = 1;
	p.rumor_lead_hi = 3;

	p.hospital_per_hp = 60;
	p.flight_lo = 150;
	p.flight_hi = 450;
	return p;
}

#define IN(v, lo, hi) ((v) >= (lo) && (v) <= (hi))
#define PAIR(a, b, lo, hi) (IN(a, lo, hi) && IN(b, lo, hi) && (a) <= (b))

int params_sane(const Params *p)
{
	return IN(p->start_cash, 0, MONEY_MAX) && IN(p->start_debt, 0, MONEY_MAX) &&
	       IN(p->start_city, 0, NUM_CITIES - 1) && IN(p->start_capacity, 0, 1000000) &&
	       IN(p->min_goods, 0, NUM_GOODS) && IN(p->absent_permille, 0, 1000) &&
	       IN(p->spike_permille, 0, 1000) && IN(p->crash_permille, 0, 1000) &&
	       PAIR(p->spike_lo_pct, p->spike_hi_pct, 1, 10000) &&
	       PAIR(p->crash_lo_pct, p->crash_hi_pct, 1, 10000) &&
	       IN(p->bank_rate_bp, 0, 3000) && IN(p->min_payment_pct, 0, 100) &&
	       IN(p->bank_fee_pct, 0, 100) && IN(p->bank_fee_min, 0, MONEY_MAX) &&
	       IN(p->enforce_days, 0, 30000) && PAIR(p->enforce_dmg_lo, p->enforce_dmg_hi, 0, 1000) &&
	       IN(p->cop_base_permille, 0, 1000) && IN(p->cop_per_10k_permille, 0, 1000) &&
	       IN(p->cop_max_permille, 0, 1000) &&
	       IN(p->posse_start, 0, 1000) && IN(p->posse_growth, 0, 100) && IN(p->max_shooters, 0, 1000) &&
	       IN(p->run_permille, 0, 1000) && IN(p->cop_hit_permille, 0, 1000) &&
	       PAIR(p->cop_dmg_lo, p->cop_dmg_hi, 0, 1000) &&
	       PAIR(p->loot_lo, p->loot_hi, 0, 1000000000) &&
	       IN(p->drop_permille, 0, 1000) && PAIR(p->drop_lo_pct, p->drop_hi_pct, 0, 100) &&
	       IN(p->jail_fine_pct, 0, 100) &&
	       IN(p->mug_permille, 0, 1000) && PAIR(p->mug_lo_pct, p->mug_hi_pct, 0, 100) &&
	       IN(p->rumor_permille, 0, 1000) && IN(p->rumor_true_permille, 0, 1000) &&
	       PAIR(p->rumor_lead_lo, p->rumor_lead_hi, 0, MAX_DAYS) &&
	       IN(p->hospital_per_hp, 0, 1000000000) &&
	       PAIR(p->flight_lo, p->flight_hi, 0, 1000000000);
}
