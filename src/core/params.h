/* Every tunable number in the rules. params_default() holds the shipped values;
 * tests build their own. Units: permille = chance out of 1000, pct = percent,
 * bp = basis points per day, money in dollars. */
#ifndef PH_PARAMS_H
#define PH_PARAMS_H

#include <stdint.h>

typedef struct {
	int64_t start_cash, start_debt;			/* debt is owed to the start city's lender */
	int32_t start_city, start_capacity;
	int32_t min_goods, absent_permille;		/* goods always on sale; chance one is missing */
	int32_t spike_permille, crash_permille;		/* per good per stop */
	int32_t spike_lo_pct, spike_hi_pct, crash_lo_pct, crash_hi_pct;	/* price multipliers */
	int32_t bank_rate_bp, min_payment_pct;		/* bank interest; payment that resets a lender's clock */
	int32_t enforce_days, enforce_dmg_lo, enforce_dmg_hi;	/* days overdue before goons; their damage */
	int32_t cop_base_permille, cop_per_10k_permille, cop_max_permille;
	int32_t posse_start, posse_growth, max_shooters;
	int32_t run_permille, cop_hit_permille, cop_dmg_lo, cop_dmg_hi;
	int64_t loot_lo, loot_hi;			/* cash found after beating the cops */
	int32_t drop_permille, drop_lo_pct, drop_hi_pct, jail_fine_pct;
	int32_t mug_permille, mug_lo_pct, mug_hi_pct;
	int32_t rumor_permille, rumor_true_permille, rumor_lead_lo, rumor_lead_hi;
	int32_t hospital_per_hp;
	int64_t flight_lo, flight_hi;
} Params;

Params params_default(void);
/* 1 if every value is in a range the rules can work with (used on loaded saves). */
int    params_sane(const Params *p);

#endif
