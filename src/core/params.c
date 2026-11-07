/* Starting values: our own picks in the genre's general shape. Task 14 tunes them
 * with ph-sim. Change them here, never in the rules code. */
#include "core/params.h"

Params params_default(void)
{
	Params p;

	p.start_cash = 1800;
	p.start_debt = 5000;
	p.start_city = 0;		/* Oklahoma City */
	p.start_capacity = 100;

	p.min_goods = 5;
	p.absent_permille = 200;
	p.spike_permille = 15;
	p.crash_permille = 15;
	p.spike_lo_pct = 250;
	p.spike_hi_pct = 450;
	p.crash_lo_pct = 20;
	p.crash_hi_pct = 45;

	p.bank_rate_bp = 150;
	p.min_payment_pct = 10;
	p.enforce_days = 6;
	p.enforce_dmg_lo = 8;
	p.enforce_dmg_hi = 20;

	p.cop_base_permille = 60;
	p.cop_per_10k_permille = 8;
	p.cop_max_permille = 400;
	p.posse_start = 1;
	p.posse_growth = 1;
	p.max_shooters = 3;
	p.run_permille = 550;
	p.cop_hit_permille = 300;
	p.cop_dmg_lo = 3;
	p.cop_dmg_hi = 9;
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
