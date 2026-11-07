/* The goods, cheapest first. Starting price ranges; Task 14 tunes them against the sim. */
#include "content/content.h"

const GoodDef GOODS[NUM_GOODS] = {
	{"Carts",    "Tastes like blue raspberry and poor decisions.",          20,    90},
	{"Weed",     "Legal in half the map. Still sold out of a backpack.",    60,   240},
	{"Shrooms",  "Grown in a closet by a guy who calls himself Moss.",     150,   500},
	{"Molly",    "Allegedly pure. Allegedly.",                              300,  1100},
	{"Acid",     "Ten tiny squares of somebody else's art.",                600,  2200},
	{"Ketamine", "For horses, ravers and the horses at raves.",             900,  3000},
	{"DMT",      "Fifteen minutes that feel like a weekend in space.",     1500,  4800},
	{"Meth",     "Cooked in an RV that has never once moved.",             2500,  7500},
	{"Heroin",   "Moves slow, sells fast.",                                5000, 14000},
	{"Cocaine",  "Imported, overpriced and somehow always in a bathroom.", 9000, 26000},
};
