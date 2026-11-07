/* ph-sim report helpers: comparing our numbers against observed targets. */
#ifndef PH_REPORT_H
#define PH_REPORT_H

typedef enum { V_OK, V_LOW, V_HIGH, V_TOO_CLOSE } Verdict;

/* ours/theirs outside 0.6..1.67 is LOW or HIGH; with must_differ, within 5% of 1 is
 * TOO_CLOSE (our number is a copy, not a tuning); anything else is OK. */
Verdict     compare_value(double ours, double theirs, int must_differ);
const char *verdict_name(Verdict v);
/* The target name our good is compared with (exact name, or a synonym). */
int         target_matches_good(const char *target_name, const char *good_name);

#endif
