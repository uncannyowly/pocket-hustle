/* The seven cities, in Seth's order. bias_pct follows GOODS order:
 * Carts, Weed, Shrooms, Ketamine, Molly, Acid, DMT, Meth, Heroin, Cocaine.
 * The last number marks the hoods with a bank branch (bit 0 = first hood). Branches:
 * Bricktown, Midtown; Galleria, Downtown; Downtown, Santana Row; SoHo, Chelsea;
 * Brickell, Coconut Grove; Garden District, Mid-City; Pearl District, Hawthorne. */
#include "content/content.h"

const CityDef CITIES[NUM_CITIES] = {
	{"Oklahoma City", "Loud City", "OKC Streetcar", "Will Rogers",
	 {"Bricktown", "Midtown", "Deep Deuce", "Plaza District", "Paseo Arts", "Stockyards City"},
	 { 70,  60, 100, 110, 130, 100, 100, 100, 100, 135},
	 "Tornado Terry", 950, 15000, 0x03},
	{"Houston", "", "METRORail", "Bush Intercontinental",
	 {"Montrose", "The Heights", "EaDo", "Galleria", "Rice Village", "Downtown"},
	 {100, 115, 130, 100, 100, 130, 100,  80, 100,  75},
	 "Lone Star Lou", 700, 25000, 0x28},
	{"San Jose", "", "VTA Light Rail", "Mineta San Jose",
	 {"Downtown", "Japantown", "Willow Glen", "Santana Row", "Rose Garden", "Little Portugal"},
	 {100,  80,  75, 100, 110,  70, 110, 130, 125, 140},
	 "Angel Investor Al", 500, 60000, 0x09},
	{"New York City", "", "the Subway", "JFK",
	 {"Times Square", "SoHo", "Williamsburg", "Astoria", "Lower East Side", "Chelsea"},
	 { 80, 130, 140, 100,  80, 100, 125, 100, 100, 100},
	 "Rent-Stabilized Ray", 1200, 40000, 0x22},
	{"Miami", "", "Metrorail", "Miami International",
	 {"South Beach", "Wynwood", "Little Havana", "Brickell", "Coconut Grove", "Design District"},
	 {100, 100, 125, 110,  75, 100, 100, 135, 100,  60},
	 "Flamingo Flo", 800, 30000, 0x18},
	{"New Orleans", "", "the St. Charles streetcar", "Louis Armstrong",
	 {"French Quarter", "Marigny", "Bywater", "Garden District", "Uptown", "Mid-City"},
	 {100,  75,  80, 130, 100, 110, 100, 100, 100, 125},
	 "Mardi Gras Marge", 1100, 20000, 0x28},
	{"Portland", "", "MAX", "PDX",
	 {"Pearl District", "Alberta Arts", "Hawthorne", "Sellwood", "St. Johns", "Mississippi Ave"},
	 { 90,  65,  55, 100, 100, 100, 110, 125, 100, 130},
	 "Kombucha Kyle", 600, 18000, 0x05},
};
