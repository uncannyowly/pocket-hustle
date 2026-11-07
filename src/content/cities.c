/* The seven cities, in Seth's order. bias_pct follows GOODS order:
 * Carts, Weed, Shrooms, Molly, Acid, Ketamine, DMT, Meth, Heroin, Cocaine. */
#include "content/content.h"

const CityDef CITIES[NUM_CITIES] = {
	{"Oklahoma City", "Loud City", "OKC Streetcar", "Will Rogers",
	 {"Bricktown", "Midtown", "Deep Deuce", "Plaza District", "Paseo Arts", "Stockyards City"},
	 { 70,  60, 100, 130, 100, 110, 100, 100, 100, 135},
	 "Tornado Terry", 900, 15000},
	{"Houston", "", "METRORail", "Bush Intercontinental",
	 {"Montrose", "The Heights", "EaDo", "Galleria", "Rice Village", "Downtown"},
	 {100, 115, 130, 100, 130, 100, 100,  80, 100,  75},
	 "Lone Star Lou", 700, 25000},
	{"San Jose", "", "VTA Light Rail", "Mineta San Jose",
	 {"Downtown", "Japantown", "Willow Glen", "Santana Row", "Rose Garden", "Little Portugal"},
	 {100,  80,  75, 110,  70, 100, 110, 130, 125, 140},
	 "Angel Investor Al", 500, 60000},
	{"New York City", "", "the Subway", "JFK",
	 {"Times Square", "SoHo", "Williamsburg", "Astoria", "Lower East Side", "Chelsea"},
	 { 80, 130, 140,  80, 100, 100, 125, 100, 100, 100},
	 "Rent-Stabilized Ray", 1100, 40000},
	{"Miami", "", "Metrorail", "Miami International",
	 {"South Beach", "Wynwood", "Little Havana", "Brickell", "Coconut Grove", "Design District"},
	 {100, 100, 125,  75, 100, 110, 100, 135, 100,  60},
	 "Flamingo Flo", 800, 30000},
	{"New Orleans", "", "the St. Charles streetcar", "Louis Armstrong",
	 {"French Quarter", "Marigny", "Bywater", "Garden District", "Uptown", "Mid-City"},
	 {100,  75,  80, 100, 110, 130, 100, 100, 100, 125},
	 "Mardi Gras Marge", 1000, 20000},
	{"Portland", "", "MAX", "PDX",
	 {"Pearl District", "Alberta Arts", "Hawthorne", "Sellwood", "St. Johns", "Mississippi Ave"},
	 { 90,  65,  55, 100, 100, 100, 110, 125, 100, 130},
	 "Kombucha Kyle", 600, 18000},
};
