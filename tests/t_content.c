#include <ctype.h>

#include "check.h"
#include "core/params.h"
#include "tests.h"
#include "content/content.h"

#define MAX_STRINGS 2048

static const char *const DENYLIST[] = {
	"dope wars", "beermat", "hardass", "netmasters", "peashoot", "pea shoot",
	"catapult", "chainsaw", "handbag", "monkey", "chicken", "old lady",
};

/* The plan's content table, one row per city (Plan 4). */
typedef struct {
	const char *name, *nick, *shortn, *code, *airport, *transit, *hoods[6], *lender;
	int bp;
	long limit;
	int branch_mask, country;
} Row;

static const Row ROWS[18] = {
	{"Oklahoma City", "Loud City", "OKC", "OKC", "Will Rogers", "OKC Streetcar",
	 {"Bricktown", "Midtown", "Deep Deuce", "Plaza District", "Paseo Arts", "Stockyards City"},
	 "Tornado Terry", 950, 15000, 0x03, 0},
	{"Houston", "H-Town", "Houston", "IAH", "Bush Intercontinental", "METRORail",
	 {"Montrose", "The Heights", "EaDo", "Galleria", "Rice Village", "Downtown"},
	 "Lone Star Lou", 700, 25000, 0x28, 0},
	{"San Jose", "The 408", "San Jose", "SJC", "Mineta San Jose", "VTA Light Rail",
	 {"Downtown", "Japantown", "Willow Glen", "Santana Row", "Rose Garden", "Little Portugal"},
	 "Angel Investor Al", 500, 60000, 0x09, 0},
	{"New York City", "The Big Apple", "NYC", "JFK", "JFK", "the Subway",
	 {"Times Square", "SoHo", "Williamsburg", "Astoria", "LES", "Chelsea"},
	 "Rent-Stabilized Ray", 1200, 40000, 0x22, 0},
	{"Miami", "The 305", "Miami", "MIA", "Miami International", "Metrorail",
	 {"SoBe", "Wynwood", "Little Havana", "Brickell", "The Grove", "Design District"},
	 "Flamingo Flo", 800, 30000, 0x18, 0},
	{"New Orleans", "The Big Easy", "NOLA", "MSY", "Louis Armstrong", "the St. Charles streetcar",
	 {"The Quarter", "Marigny", "Bywater", "Garden District", "Uptown", "Mid-City"},
	 "Mardi Gras Marge", 1100, 20000, 0x28, 0},
	{"Portland", "Rip City", "Portland", "PDX", "PDX", "MAX",
	 {"The Pearl", "Alberta Arts", "Hawthorne", "Sellwood", "St. Johns", "Mississippi Ave"},
	 "Kombucha Kyle", 600, 18000, 0x05, 0},
	{"Dallas", "Big D", "Dallas", "DFW", "DFW International", "DART",
	 {"Deep Ellum", "Uptown", "Bishop Arts", "Lower Greenville", "Oak Cliff", "Design District"},
	 "Brisket Brenda", 850, 25000, 0x22, 0},
	{"San Francisco", "The City", "SF", "SFO", "SFO", "Muni",
	 {"The Mission", "SoMa", "The Haight", "The Castro", "North Beach", "Dogpatch"},
	 "Sourdough Sam", 650, 50000, 0x12, 0},
	{"Los Angeles", "La La Land", "LA", "LAX", "LAX", "the Metro",
	 {"DTLA", "WeHo", "K-Town", "Silver Lake", "Venice", "Echo Park"},
	 "Influencer Ivy", 750, 45000, 0x03, 0},
	{"Seattle", "Emerald City", "Seattle", "SEA", "Sea-Tac", "Link light rail",
	 {"Cap Hill", "Ballard", "Fremont", "SoDo", "The Ave", "Pioneer Square"},
	 "Espresso Ezra", 600, 30000, 0x09, 0},
	{"Cincinnati", "The Nati", "Cincy", "CVG", "CVG", "the Connector",
	 {"OTR", "Clifton", "Mt. Adams", "Northside", "The Banks", "Oakley"},
	 "Three-Way Wes", 900, 20000, 0x11, 0},
	{"Dayton", "Gem City", "Dayton", "DAY", "Dayton International", "the trolley bus",
	 {"Oregon District", "Fire Blocks", "Webster Station", "South Park", "Grafton Hill", "Wright-Dunbar"},
	 "Propeller Pat", 1000, 15000, 0x06, 0},
	{"Destin", "Emerald Coast", "Destin", "VPS", "Destin-Fort Walton", "the water taxi",
	 {"The Harbor", "Crab Island", "Miramar Beach", "Okaloosa Island", "Henderson Beach", "Holiday Isle"},
	 "Snapper Sally", 800, 25000, 0x05, 0},
	{"Tijuana", "TJ", "TJ", "TIJ", "Tijuana International", "a calafia",
	 {"La Revu", "Zona Rio", "La Cacho", "Playas", "Hipodromo", "Otay"},
	 "Zonkey Zoe", 1100, 15000, 0, 1},
	{"Cozumel", "Coz", "Cozumel", "CZM", "Cozumel International", "a moped",
	 {"San Miguel", "The Malecon", "Chankanaab", "Punta Sur", "El Cedral", "San Gervasio"},
	 "Reef Rita", 900, 20000, 0, 1},
	{"Los Cabos", "Cabo", "Cabo", "SJD", "Los Cabos International", "a panga",
	 {"The Marina", "Medano Beach", "Land's End", "Art District", "The Corridor", "Pedregal"},
	 "Timeshare Tim", 1000, 25000, 0, 1},
	{"Sinaloa", "La Perla", "Sinaloa", "MZT", "Mazatlan International", "a pulmonia",
	 {"El Malecon", "Olas Altas", "Zona Dorada", "El Centro", "Stone Island", "Culiacan"},
	 "Banda Bea", 1200, 15000, 0, 1},
};

static const int16_t NEW_BIAS[11][NUM_GOODS] = {
	{ 95, 110, 100, 100, 105, 100, 100,  85, 100,  95},
	{110,  60,  70, 105, 110,  75,  90, 120, 110, 130},
	{ 90,  65,  90,  95, 115, 100, 100, 110, 100, 120},
	{100,  65,  60, 100, 100,  90,  95, 120, 110, 125},
	{ 90, 105, 115, 105, 100, 110, 115,  85,  90, 110},
	{ 85, 100, 110, 110, 105, 110, 115,  80,  85, 115},
	{110, 120, 125, 115, 130, 120, 120, 105, 110, 135},
	{100,  70, 100,  90,  95, 105, 100,  70,  80,  65},
	{115, 110, 120, 110, 125, 115, 110, 100, 105,  85},
	{115, 115, 120, 110, 135, 120, 110, 100, 110,  80},
	{100,  60,  95,  95, 100, 100, 100,  60,  75,  55},
};

static void test_cities_in_plan_order(void)
{
	int c, h, g;
	CHECK_EQ(NUM_CITIES, 18);
	CHECK_EQ(NUM_LOCS, 18 * HOODS_PER_CITY);
	for (c = 0; c < NUM_CITIES; c++) {
		const Row *r = &ROWS[c];
		CHECK_STR(CITIES[c].name, r->name);
		CHECK_STR(CITIES[c].nickname, r->nick);
		CHECK_STR(CITIES[c].short_name, r->shortn);
		CHECK_STR(CITIES[c].code, r->code);
		CHECK_STR(CITIES[c].airport, r->airport);
		CHECK_STR(CITIES[c].transit, r->transit);
		CHECK_STR(CITIES[c].lender, r->lender);
		for (h = 0; h < HOODS_PER_CITY; h++)
			CHECK_STR(CITIES[c].hoods[h], r->hoods[h]);
		CHECK_EQ(CITIES[c].lender_rate_bp, r->bp);
		CHECK_EQ(CITIES[c].lender_max, r->limit);
		CHECK_EQ(CITIES[c].bank_hoods, r->branch_mask);
		CHECK_EQ(CITIES[c].country, r->country);
		CHECK(CITIES[c].nickname[0] != 0);
	}
	for (c = 7; c < NUM_CITIES; c++)
		for (g = 0; g < NUM_GOODS; g++)
			CHECK_EQ(CITIES[c].bias_pct[g], NEW_BIAS[c - 7][g]);
	CHECK_EQ(COUNTRY_US, 0);
	CHECK_EQ(COUNTRY_MX, 1);
}

static void test_original_seven_keep_biases(void)
{
	static const int16_t old[7][NUM_GOODS] = {
		{ 70,  60, 100, 110, 130, 100, 100, 100, 100, 135},
		{100, 115, 130, 100, 100, 130, 100,  80, 100,  75},
		{100,  80,  75, 100, 110,  70, 110, 130, 125, 140},
		{ 80, 130, 140, 100,  80, 100, 125, 100, 100, 100},
		{100, 100, 125, 110,  75, 100, 100, 135, 100,  60},
		{100,  75,  80, 130, 100, 110, 100, 100, 100, 125},
		{ 90,  65,  55, 100, 100, 100, 110, 125, 100, 130},
	};
	int c, g;
	for (c = 0; c < 7; c++)
		for (g = 0; g < NUM_GOODS; g++)
			CHECK_EQ(CITIES[c].bias_pct[g], old[c][g]);
}

static void test_short_names_and_codes_unique(void)
{
	int a, b;
	for (a = 0; a < NUM_CITIES; a++) {
		CHECK(strlen(CITIES[a].name) <= 13);
		CHECK(strlen(CITIES[a].nickname) <= 13);
		CHECK(strlen(CITIES[a].short_name) >= 1 && strlen(CITIES[a].short_name) <= 9);
		for (b = a + 1; b < NUM_CITIES; b++) {
			CHECK(strcmp(CITIES[a].short_name, CITIES[b].short_name) != 0);
			CHECK(strcmp(CITIES[a].nickname, CITIES[b].nickname) != 0);
			CHECK(strcmp(CITIES[a].lender, CITIES[b].lender) != 0);
		}
	}
}

static void test_branches_by_country(void)
{
	int c, h, n;
	for (c = 0; c < NUM_CITIES; c++) {
		n = 0;
		for (h = 0; h < HOODS_PER_CITY; h++)
			n += (CITIES[c].bank_hoods >> h) & 1;
		if (CITIES[c].country == COUNTRY_US)
			CHECK_EQ(n, 2);
		else
			CHECK_EQ(n, 0);
	}
}

static void test_airport_codes(void)
{
	int a, b, i;
	for (a = 0; a < NUM_CITIES; a++) {
		const char *c = CITIES[a].code;
		CHECK(c != NULL && strlen(c) == 3);
		for (i = 0; c && i < 3; i++)
			CHECK(c[i] >= 'A' && c[i] <= 'Z');
		for (b = a + 1; b < NUM_CITIES; b++)
			CHECK(strcmp(c, CITIES[b].code) != 0);
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
		CHECK(strlen(CITIES[c].lender) <= 19);
		for (i = 0; i < HOODS_PER_CITY; i++) {
			CHECK(strlen(CITIES[c].hoods[i]) <= 16);	/* Lower Greenville is 16 */
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
		int cheap = 0, dear = 0, off = 0;
		for (i = 0; i < NUM_GOODS; i++) {
			int b = CITIES[c].bias_pct[i];
			CHECK(b >= 50 && b <= 200);
			if (b <= 80) cheap++;
			if (b >= 125) dear++;
			if (b != 100) off++;
		}
		CHECK(off >= 3);	/* every city has a character */
		if (c < 7) {	/* the original seven are strongly shaped; the table's Plan 4 cities may be mild */
			CHECK(cheap >= 2);
			CHECK(dear >= 2);
		}
	}
}

static void test_lenders_differ(void)
{
	int c, d;
	for (c = 0; c < NUM_CITIES; c++) {
		CHECK(CITIES[c].lender_rate_bp >= 1 && CITIES[c].lender_rate_bp <= 3000);
		CHECK(CITIES[c].lender_max > 0);
		for (d = c + 1; d < NUM_CITIES; d++)	/* the table gives Cincinnati and Cozumel the same terms */
			CHECK(strcmp(CITIES[c].lender, CITIES[d].lender) != 0);
	}
}

static void test_bank_branches(void)
{
	Params p = params_default();
	int c, h, n;
	for (c = 0; c < NUM_CITIES; c++) {
		n = 0;
		for (h = 0; h < HOODS_PER_CITY; h++)
			n += (CITIES[c].bank_hoods >> h) & 1;
		CHECK(n < HOODS_PER_CITY);
		CHECK_EQ(CITIES[c].bank_hoods >> HOODS_PER_CITY, 0);
	}
	CHECK(CITIES[p.start_city].bank_hoods & 1u);	/* the first deposit is free */
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
	test_cities_in_plan_order();
	test_original_seven_keep_biases();
	test_short_names_and_codes_unique();
	test_branches_by_country();
	test_airport_codes();
	test_names_fit_and_unique();
	test_nyc_not_the_classic_list();
	test_price_ranges_sane();
	test_city_tendencies();
	test_lenders_differ();
	test_bank_branches();
	test_gear_mix();
	test_strings_ascii();
	test_strings_not_beermat();
}
