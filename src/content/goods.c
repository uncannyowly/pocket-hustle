/* The goods, cheapest first. Starting price ranges; Task 14 tunes them against the sim. */
#include "content/content.h"

const GoodDef GOODS[NUM_GOODS] = {
	{"Carts",    "Tastes like blue raspberry and poor decisions.",    30,   220},
	{"Weed",     "Legal in half the map. Still sold out of a backpack.",   300,  1150},
	{"Shrooms",  "Grown in a closet by a guy who calls himself Moss.",   450,  1500},
	{"Ketamine", "For horses, ravers and the horses at raves.",   600,  2000},
	{"Molly",    "Allegedly pure. Allegedly.",  1000,  3600},
	{"Acid",     "Ten tiny squares of somebody else's art.",  1800,  5000},
	{"DMT",      "Fifteen minutes that feel like a weekend in space.",  2600,  7500},
	{"Meth",     "Cooked in an RV that has never once moved.",  3500, 11000},
	{"Heroin",   "Moves slow, sells fast.",  6500, 20000},
	{"Cocaine",  "Imported, overpriced and somehow always in a bathroom.", 12500, 33000},
};
