/* ph-sim report helpers. */
#include <string.h>

#include "report.h"

Verdict compare_value(double ours, double theirs, int must_differ)
{
	double r, d;

	if (theirs == 0)
		return ours == 0 ? (must_differ ? V_TOO_CLOSE : V_OK) : V_HIGH;
	r = ours / theirs;
	if (r < 0.6)
		return V_LOW;
	if (r > 1.67)
		return V_HIGH;
	d = r > 1 ? r - 1 : 1 - r;
	if (must_differ && d <= 0.05)
		return V_TOO_CLOSE;
	return V_OK;
}

const char *verdict_name(Verdict v)
{
	switch (v) {
	case V_OK: return "OK";
	case V_LOW: return "LOW";
	case V_HIGH: return "HIGH";
	default: return "TOO_CLOSE";
	}
}

int target_matches_good(const char *target_name, const char *good_name)
{
	static const char *const alias[][2] = {
		{"Mushrooms", "Shrooms"}, {"Ecstasy", "Molly"}, {"Special K", "Ketamine"},
	};
	size_t i;

	if (!strcmp(target_name, good_name))
		return 1;
	for (i = 0; i < sizeof alias / sizeof alias[0]; i++)
		if (!strcmp(target_name, alias[i][0]) && !strcmp(good_name, alias[i][1]))
			return 1;
	return 0;
}
