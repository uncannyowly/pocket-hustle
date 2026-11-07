#include <ctype.h>

#include "check.h"
#include "tests.h"
#include "content/content.h"

#define MAX_STRINGS 2048

static const char *const DENYLIST[] = {
	"dope wars", "beermat", "hardass", "netmasters", "peashoot", "pea shoot",
	"catapult", "chainsaw", "handbag", "monkey", "chicken", "old lady",
};

static void test_cities_in_seth_order(void)
{
	static const char *const names[NUM_CITIES] = {
		"Oklahoma City", "Houston", "San Jose", "New York City", "Miami", "New Orleans", "Portland",
	};
	static const char *const transit[NUM_CITIES] = {
		"OKC Streetcar", "METRORail", "VTA Light Rail", "the Subway", "Metrorail",
		"the St. Charles streetcar", "MAX",
	};
	int c;
	CHECK_STR(CITIES[0].nickname, "Loud City");
	for (c = 0; c < NUM_CITIES; c++) {
		CHECK_STR(CITIES[c].name, names[c]);
		CHECK_STR(CITIES[c].transit, transit[c]);
	}
}

static void test_names_fit_and_unique(void)
{
	int i, j, c;
	for (i = 0; i < NUM_GOODS; i++) {
		CHECK(strlen(GOODS[i].name) <= 10);
		for (j = i + 1; j < NUM_GOODS; j++)
			CHECK(strcmp(GOODS[i].name, GOODS[j].name) != 0);
	}
	for (c = 0; c < NUM_CITIES; c++) {
		CHECK(strlen(CITIES[c].lender) <= 20);
		for (i = 0; i < HOODS_PER_CITY; i++) {
			CHECK(strlen(CITIES[c].hoods[i]) <= 16);
			for (j = i + 1; j < HOODS_PER_CITY; j++)
				CHECK(strcmp(CITIES[c].hoods[i], CITIES[c].hoods[j]) != 0);
		}
	}
	for (i = 0; i < NUM_GEAR; i++) {
		CHECK(strlen(GEAR[i].name) <= 16);
		for (j = i + 1; j < NUM_GEAR; j++)
			CHECK(strcmp(GEAR[i].name, GEAR[j].name) != 0);
	}
}

static void test_nyc_not_the_classic_list(void)
{
	static const char *const classic[] = {
		"Bronx", "Queens", "Central Park", "Manhattan", "Coney Island", "Brooklyn",
	};
	int i, k, overlap = 0;
	for (i = 0; i < HOODS_PER_CITY; i++)
		for (k = 0; k < 6; k++)
			if (strcmp(CITIES[3].hoods[i], classic[k]) == 0)
				overlap++;
	CHECK(overlap <= 2);
}

static void test_price_ranges_sane(void)
{
	int i;
	int32_t min_lo = GOODS[0].lo, max_hi = GOODS[0].hi;
	for (i = 0; i < NUM_GOODS; i++) {
		CHECK(GOODS[i].lo > 0);
		CHECK(GOODS[i].lo < GOODS[i].hi);
		if (GOODS[i].lo < min_lo) min_lo = GOODS[i].lo;
		if (GOODS[i].hi > max_hi) max_hi = GOODS[i].hi;
	}
	CHECK(min_lo < 100);
	CHECK(max_hi >= 10000);
}

static void test_city_tendencies(void)
{
	int c, i;
	for (c = 0; c < NUM_CITIES; c++) {
		int cheap = 0, dear = 0;
		for (i = 0; i < NUM_GOODS; i++) {
			int b = CITIES[c].bias_pct[i];
			CHECK(b >= 50 && b <= 200);
			if (b <= 80) cheap++;
			if (b >= 125) dear++;
		}
		CHECK(cheap >= 2);
		CHECK(dear >= 2);
	}
}

static void test_lenders_differ(void)
{
	int c, d;
	for (c = 0; c < NUM_CITIES; c++) {
		CHECK(CITIES[c].lender_rate_bp >= 1 && CITIES[c].lender_rate_bp <= 3000);
		CHECK(CITIES[c].lender_max > 0);
		for (d = c + 1; d < NUM_CITIES; d++)
			CHECK(CITIES[c].lender_rate_bp != CITIES[d].lender_rate_bp ||
			      CITIES[c].lender_max != CITIES[d].lender_max);
	}
}

static void test_gear_mix(void)
{
	int i, weapons = 0, space = 0, local = 0;
	CHECK(NUM_GEAR <= MAX_GEAR);
	for (i = 0; i < NUM_GEAR; i++) {
		CHECK(GEAR[i].price > 0);
		CHECK(GEAR[i].city >= -1 && GEAR[i].city < NUM_CITIES);
		if (GEAR[i].kind == GEAR_WEAPON) {
			weapons++;
			CHECK(GEAR[i].power >= 100 && GEAR[i].power <= 900);
		} else {
			space++;
			CHECK(GEAR[i].power > 0);
		}
		if (GEAR[i].city >= 0) local++;
	}
	CHECK(weapons >= 6);
	CHECK(space >= 3);
	CHECK(local >= 2);
}

static void test_strings_ascii(void)
{
	static const char *s[MAX_STRINGS];
	int n = content_strings(s, MAX_STRINGS), i, ok = 1;
	CHECK(n > 0 && n <= MAX_STRINGS);
	for (i = 0; i < n && i < MAX_STRINGS; i++) {
		const unsigned char *p = (const unsigned char *)s[i];
		CHECK(p != NULL);
		for (; p && *p; p++)
			if (*p < 0x20 || *p > 0x7e) {
				ok = 0;
				fprintf(stderr, "non-ASCII byte in: %s\n", s[i]);
				break;
			}
	}
	CHECK(ok);
}

static int contains_ci(const char *hay, const char *needle)
{
	size_t n = strlen(needle);
	for (; *hay; hay++) {
		size_t k = 0;
		while (k < n && hay[k] && tolower((unsigned char)hay[k]) == needle[k])
			k++;
		if (k == n)
			return 1;
	}
	return 0;
}

static void test_strings_not_beermat(void)
{
	static const char *s[MAX_STRINGS];
	int n = content_strings(s, MAX_STRINGS), i;
	size_t k;
	for (i = 0; i < n && i < MAX_STRINGS; i++)
		for (k = 0; k < sizeof DENYLIST / sizeof DENYLIST[0]; k++)
			if (contains_ci(s[i], DENYLIST[k])) {
				fprintf(stderr, "denylisted \"%s\" in: %s\n", DENYLIST[k], s[i]);
				CHECK(0);
			}
	CHECK(contains_ci("Rubber Chicken", "chicken"));	/* the matcher itself works */
}

void tests_content(void)
{
	test_cities_in_seth_order();
	test_names_fit_and_unique();
	test_nyc_not_the_classic_list();
	test_price_ranges_sane();
	test_city_tendencies();
	test_lenders_differ();
	test_gear_mix();
	test_strings_ascii();
	test_strings_not_beermat();
}
