/* Every tunable number in the rules. params_default() holds the shipped values;
 * tests build their own. Units: permille = chance out of 1000, pct = percent,
 * bp = basis points per day, money in dollars. */
#ifndef PH_PARAMS_H
#define PH_PARAMS_H

#include <stdint.h>

enum { MAX_STAKES = 3 };

typedef struct {
	int64_t start_cash, start_debt;			/* debt is owed to the start city's lender */
	int32_t start_city, start_capacity;
	int32_t min_goods, absent_permille;		/* goods always on sale; chance one is missing */
	int32_t spike_permille, crash_permille;		/* per good per stop */
	int32_t spike_lo_pct, spike_hi_pct, crash_lo_pct, crash_hi_pct;	/* price multipliers */
	int32_t bank_rate_bp, min_payment_pct;		/* bank interest; payment that resets a lender's clock */
	int32_t bank_fee_pct;				/* away from a branch: this pct of the amount, */
	int64_t bank_fee_min;				/* but at least this much */
	int32_t enforce_days, enforce_dmg_lo, enforce_dmg_hi;	/* days overdue before goons; their damage */
	int32_t cop_base_permille, cop_per_10k_permille, cop_max_permille;
	int32_t posse_start, posse_growth, max_shooters;
	int32_t run_permille, cop_hit_permille, cop_dmg_lo, cop_dmg_hi;
	int64_t loot_lo, loot_hi;			/* cash found after beating the cops */
	int32_t drop_permille, drop_lo_pct, drop_hi_pct, jail_fine_pct;
	int32_t mug_permille, mug_lo_pct, mug_hi_pct;
	int32_t rumor_permille, rumor_true_permille, rumor_lead_lo, rumor_lead_hi;
	int32_t hospital_per_hp;
	int64_t flight_lo, flight_hi;			/* a flight within one country */
	int64_t intl_flight_lo, intl_flight_hi;		/* a flight across the border */
	/* Wild cards and finds: a chemist offers a stake on arrival; a stake resolves days later. */
	int32_t rc_offer_permille, rc_max_stakes;	/* chance per arrival; stakes running at once (at most MAX_STAKES) */
	int32_t rc_days_lo, rc_days_hi;			/* days until a stake resolves */
	int32_t rc_find_permille;			/* chance per arrival of finding a free batch */
	int64_t rc_find_lo, rc_find_hi;			/* a found batch's notional value */
	/* The shady crew: at most cohort_max offers per run, only to a big stash. */
	int32_t cohort_max, cohort_offer_permille;
	int64_t cohort_min_stash;			/* stash value at mid prices */
	int32_t cohort_od_permille, cohort_mult_pct;	/* chance of the overdose; payout as pct of the stash's value */
	int64_t cohort_min_payout;
} Params;

Params params_default(void);
/* 1 if every value is in a range the rules can work with (used on loaded saves). */
int    params_sane(const Params *p);

#endif
