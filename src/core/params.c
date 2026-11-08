/* Starting values: our own picks in the genre's general shape. Task 14 tunes them
 * with ph-sim. Change them here, never in the rules code. */
#include <string.h>

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
	p.intl_flight_lo = 350;
	p.intl_flight_hi = 850;

	p.rc_offer_permille = 80;
	p.rc_max_stakes = 3;
	p.rc_days_lo = 2;
	p.rc_days_hi = 4;
	p.rc_find_permille = 25;
	p.rc_find_lo = 1000;
	p.rc_find_hi = 5000;

	p.cohort_max = 2;
	p.cohort_offer_permille = 40;
	p.cohort_min_stash = 5000;
	p.cohort_od_permille = 550;
	p.cohort_mult_pct = 300;
	p.cohort_min_payout = 10000;
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
	       PAIR(p->flight_lo, p->flight_hi, 0, 1000000000) &&
	       PAIR(p->intl_flight_lo, p->intl_flight_hi, 0, 1000000000) &&
	       IN(p->rc_offer_permille, 0, 1000) && IN(p->rc_max_stakes, 0, MAX_STAKES) &&
	       PAIR(p->rc_days_lo, p->rc_days_hi, 1, 30) && IN(p->rc_find_permille, 0, 1000) &&
	       PAIR(p->rc_find_lo, p->rc_find_hi, 0, 1000000000) &&
	       IN(p->cohort_max, 0, 10) && IN(p->cohort_offer_permille, 0, 1000) &&
	       IN(p->cohort_min_stash, 0, MONEY_MAX) && IN(p->cohort_od_permille, 0, 1000) &&
	       IN(p->cohort_mult_pct, 0, 10000) && IN(p->cohort_min_payout, 0, 1000000000000LL);
}

int params_set(Params *p, const char *name, long long value)
{
#define I32(f) if (!strcmp(name, #f)) { if (value < INT32_MIN || value > INT32_MAX) return -1; p->f = (int32_t)value; return 0; }
#define I64(f) if (!strcmp(name, #f)) { p->f = value; return 0; }
	I32(rc_offer_permille) I32(rc_max_stakes) I32(rc_days_lo) I32(rc_days_hi)
	I32(rc_find_permille) I64(rc_find_lo) I64(rc_find_hi)
	I32(cohort_max) I32(cohort_offer_permille) I64(cohort_min_stash)
	I32(cohort_od_permille) I32(cohort_mult_pct) I64(cohort_min_payout)
#undef I32
#undef I64
	return -1;
}
