/* The store. Weapons: power = hit chance in permille. Space items: power = extra room. */
#include "content/content.h"

const GearDef GEAR[] = {
	{"Pool Noodle",      "Foam justice.",                                  GEAR_WEAPON,    80, 150, -1},
	{"Selfie Stick",     "Extends your reach and your follower count.",    GEAR_WEAPON,   300, 250, -1},
	{"Leaf Blower",      "Loud enough to scare anyone over forty.",        GEAR_WEAPON,   900, 350, -1},
	{"Rodeo Lasso",      "Yee, and indeed, haw.",                          GEAR_WEAPON,  1800, 450,  1},
	{"Paintball Gun",    "Leaves a mark and a dry-cleaning bill.",         GEAR_WEAPON,  2500, 500, -1},
	{"Brass Section",    "Four trombones who owe you a favor.",            GEAR_WEAPON,  4000, 550,  5},
	{"Taser",            "Ride the lightning, pass it along.",             GEAR_WEAPON,  6000, 650, -1},
	{"T-Shirt Cannon",   "Free merch at ninety miles an hour.",            GEAR_WEAPON, 15000, 800, -1},
	{"Fanny Pack",       "Back in style, holds more than you'd think.",    GEAR_SPACE,    500,  20, -1},
	{"Cargo Shorts",     "Eight pockets. No shame.",                       GEAR_SPACE,   1500,  30, -1},
	{"Bike Trailer",     "Built for kids, used for commerce.",             GEAR_SPACE,   2500,  40,  6},
	{"Rolling Suitcase", "Looks like a tourist. Moves like a warehouse.",  GEAR_SPACE,   5000,  50, -1},
};

const int NUM_GEAR = (int)(sizeof GEAR / sizeof GEAR[0]);
