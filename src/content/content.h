/* Game content: goods, cities, gear and characters. Everything here is player-visible
 * and original; tests/t_content.c lints it. */
#ifndef PH_CONTENT_H
#define PH_CONTENT_H

#include <stdint.h>

enum { NUM_GOODS = 10, NUM_CITIES = 18, HOODS_PER_CITY = 6,
       NUM_LOCS = NUM_CITIES * HOODS_PER_CITY, MAX_GEAR = 16 };

enum { COUNTRY_US = 0, COUNTRY_MX = 1 };

enum { NUM_CHEMS = 10 };

/* A research chemical a stake can ride on: the odds it catches on and what it pays then. */
typedef struct {
	const char *name, *blurb;
	int16_t odds_permille;			/* chance it catches on */
	int16_t mult_lo_pct, mult_hi_pct;	/* payout multiplier range, percent of the stake */
} ChemDef;

typedef struct { const char *name, *blurb; int32_t lo, hi; } GoodDef;	/* base price range */

typedef struct {
	const char *name, *nickname;		/* nickname: the local name, shown on the skyline */
	const char *short_name;			/* at most 9 characters, for the flight grid */
	const char *transit, *airport, *code;	/* code: three-letter airport code */
	const char *hoods[HOODS_PER_CITY];
	int16_t bias_pct[NUM_GOODS];		/* 50..200, 100 = no tendency */
	const char *lender;			/* this city's loan shark */
	int32_t lender_rate_bp;			/* daily interest, basis points */
	int64_t lender_max;			/* most you can owe this lender */
	uint8_t bank_hoods;			/* bit h set: a bank branch in hoods[h], no fee */
	uint8_t country;			/* COUNTRY_US or COUNTRY_MX; flights between differ cost more */
} CityDef;

typedef enum { GEAR_WEAPON, GEAR_SPACE } GearKind;

typedef struct {
	const char *name, *blurb;
	GearKind kind;
	int32_t price;
	int16_t power;		/* weapon: hit chance permille; space: extra capacity */
	int8_t city;		/* -1 = sold everywhere, else only in that city */
} GearDef;

extern const GoodDef GOODS[NUM_GOODS];
extern const CityDef CITIES[NUM_CITIES];
extern const GearDef GEAR[];
extern const ChemDef CHEMS[NUM_CHEMS];
extern const int NUM_GEAR;
extern const char *const COP_NAME;	/* the cop who chases you */
extern const char *const RUMOR_NAME;	/* the rumor-monger */

/* Every player-visible content string. Writes up to max pointers, returns the total count. */
int content_strings(const char **out, int max);

#endif
