# Pocket Hustle, Plan 4: More of North America Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Grow the map from 7 cities to 18 and add three gambles (research chemical wild cards, finds, and a twice-per-run shady crew) (Seth, 2026-11-08): add Dallas, San Francisco, Los Angeles, Seattle, Cincinnati, Dayton and Destin, and four places across the border (Tijuana, Cozumel, Los Cabos, Sinaloa), all in North America. Every location and neighborhood uses its real local nickname where one exists. Then play-test on the PC build, on the real Miyoo binary under an ARM emulator, and with the balance bots, and install the result on Seth's SD card.

**Architecture:** The cities are content (`src/content/cities.c`), so most of the change is data. The core grows `NUM_CITIES` to 18, learns which country a city is in (flights that cross the border cost more and say so), and bumps the save version. The art layer gets 11 new skylines and lender portraits. The flight screen gets two tabs (USA and MEXICO) and a two-column grid so 17 destinations fit without scrolling. Nothing else changes shape.

**Tech Stack:** as Plans 2 and 3a (C99, SDL2 on the PC, the `gta1-miyoo-toolchain` Docker image for the Mini). For the ARM emulator test, a throwaway Docker container with `qemu-user` and the armhf C library installed inside it (nothing installed on the host, no binfmt or kernel changes).

**Spec:** `docs/superpowers/specs/2026-11-06-pocket-hustle-design.md`. Seth's request supersedes its "seven cities" goal.

## Global Constraints

- Everything in Plan 2's and Plan 3a's Global Constraints still holds: no em or en dashes anywhere, printable ASCII player text (write Spanish names without accents: Malecon, Rio), the Beermat denylist, integer-only core and content, the UI never writes Game fields, `make test` pristine, `scripts/arm-check.sh` passing, commit identity from the repo config and a `Co-Authored-By:` line naming the model that wrote the commit.
- Anti-goals from the spec apply with extra care across the border: no neighborhood presented as the poor or dangerous one, no real people named or mocked, no references to cartels, real crimes or real criminals; local flavor (food, music, beaches, landmarks, transit) instead.
- City order (indices are content; jokes and tests depend on them): the original seven unchanged and first, then the new US cities, then Mexico: 0 Oklahoma City, 1 Houston, 2 San Jose, 3 New York City, 4 Miami, 5 New Orleans, 6 Portland, 7 Dallas, 8 San Francisco, 9 Los Angeles, 10 Seattle, 11 Cincinnati, 12 Dayton, 13 Destin, 14 Tijuana, 15 Cozumel, 16 Los Cabos, 17 Sinaloa. Oklahoma City stays the start city.
- Name lengths: city names at most 13 characters, nicknames at most 13, short names at most 9, hood names at most 15, lender names at most 19 (the existing content lint limits, extended to the new fields).
- Bank branches: every US city has exactly two branch hoods; Mexican cities have none (your bank is out of network across the border, so every ATM there charges the fee).
- Skylines and portraits: drawn in code in the style of the existing seven (`src/gfx/skyline.c`, `src/gfx/sprites.c`), deterministic, integer only, no text, no logos or trademark marks (no sign lettering, no team marks; generic silhouettes of famous shapes are fine, as with the existing seven).

## The content table (exact values; Task 1 transcribes it)

Fields: name | nickname (the skyline label) | short (flight grid) | code | airport | transit (as used in "You ride {transit} to {hood}.") | hoods 0..5 (hood 0 is where flights land) | branch hoods | lender, daily rate in basis points, limit | country.

Existing cities keep their biases, lenders, rates, limits and branch masks; their nickname, short name and some hood names change as shown.

| # | name | nickname | short | code | airport | transit | hoods | branches | lender bp limit | country |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 | Oklahoma City | Loud City | OKC | OKC | Will Rogers | OKC Streetcar | Bricktown, Midtown, Deep Deuce, Plaza District, Paseo Arts, Stockyards City | 0, 1 | Tornado Terry 950 15000 | US |
| 1 | Houston | H-Town | Houston | IAH | Bush Intercontinental | METRORail | Montrose, The Heights, EaDo, Galleria, Rice Village, Downtown | 3, 5 | Lone Star Lou 700 25000 | US |
| 2 | San Jose | The 408 | San Jose | SJC | Mineta San Jose | VTA Light Rail | Downtown, Japantown, Willow Glen, Santana Row, Rose Garden, Little Portugal | 0, 3 | Angel Investor Al 500 60000 | US |
| 3 | New York City | The Big Apple | NYC | JFK | JFK | the Subway | Times Square, SoHo, Williamsburg, Astoria, LES, Chelsea | 1, 5 | Rent-Stabilized Ray 1200 40000 | US |
| 4 | Miami | The 305 | Miami | MIA | Miami International | Metrorail | SoBe, Wynwood, Little Havana, Brickell, The Grove, Design District | 3, 4 | Flamingo Flo 800 30000 | US |
| 5 | New Orleans | The Big Easy | NOLA | MSY | Louis Armstrong | the St. Charles streetcar | The Quarter, Marigny, Bywater, Garden District, Uptown, Mid-City | 3, 5 | Mardi Gras Marge 1100 20000 | US |
| 6 | Portland | Rip City | Portland | PDX | PDX | MAX | The Pearl, Alberta Arts, Hawthorne, Sellwood, St. Johns, Mississippi Ave | 0, 2 | Kombucha Kyle 600 18000 | US |
| 7 | Dallas | Big D | Dallas | DFW | DFW International | DART | Deep Ellum, Uptown, Bishop Arts, Greenville Ave, Oak Cliff, Design District | 1, 5 | Brisket Brenda 850 25000 | US |
| 8 | San Francisco | The City | SF | SFO | SFO | Muni | The Mission, SoMa, The Haight, The Castro, North Beach, Dogpatch | 1, 4 | Sourdough Sam 650 50000 | US |
| 9 | Los Angeles | La La Land | LA | LAX | LAX | the Metro | DTLA, WeHo, K-Town, Silver Lake, Venice, Echo Park | 0, 1 | Influencer Ivy 750 45000 | US |
| 10 | Seattle | Emerald City | Seattle | SEA | Sea-Tac | Link light rail | Cap Hill, Ballard, Fremont, SoDo, The Ave, Pioneer Square | 0, 3 | Espresso Ezra 600 30000 | US |
| 11 | Cincinnati | The Nati | Cincy | CVG | CVG | the Connector | OTR, Clifton, Mt. Adams, Northside, The Banks, Oakley | 0, 4 | Three-Way Wes 900 20000 | US |
| 12 | Dayton | Gem City | Dayton | DAY | Dayton International | the trolley bus | Oregon District, Fire Blocks, Webster Station, South Park, Grafton Hill, Wright-Dunbar | 1, 2 | Propeller Pat 1000 15000 | US |
| 13 | Destin | Emerald Coast | Destin | VPS | Destin-Fort Walton | the water taxi | The Harbor, Crab Island, Miramar Beach, Okaloosa Island, Henderson Beach, Holiday Isle | 0, 2 | Snapper Sally 800 25000 | US |
| 14 | Tijuana | TJ | TJ | TIJ | Tijuana International | a calafia | La Revu, Zona Rio, La Cacho, Playas, Hipodromo, Otay | none | Zonkey Zoe 1100 15000 | MX |
| 15 | Cozumel | Coz | Cozumel | CZM | Cozumel International | a moped | San Miguel, The Malecon, Chankanaab, Punta Sur, El Cedral, San Gervasio | none | Reef Rita 875 22000 | MX |
| 16 | Los Cabos | Cabo | Cabo | SJD | Los Cabos International | a panga | The Marina, Medano Beach, Land's End, Art District, The Corridor, Pedregal | none | Timeshare Tim 1000 25000 | MX |
| 17 | Sinaloa | La Perla | Sinaloa | MZT | Mazatlan International | a pulmonia | El Malecon, Olas Altas, Zona Dorada, El Centro, Stone Island, Playa Norte | none | Banda Bea 1200 15000 | MX |

Price tendencies (bias percent, in GOODS order Carts, Weed, Shrooms, Ketamine, Molly, Acid, DMT, Meth, Heroin, Cocaine) for the new cities; the original seven keep theirs:

| # | biases |
|---|---|
| 7 Dallas | 75, 110, 100, 100, 125, 100, 100, 80, 100, 125 |
| 8 San Francisco | 110, 60, 70, 105, 110, 75, 90, 120, 125, 130 |
| 9 Los Angeles | 80, 65, 90, 95, 125, 100, 100, 110, 100, 125 |
| 10 Seattle | 100, 65, 60, 100, 100, 90, 95, 120, 125, 125 |
| 11 Cincinnati | 90, 105, 115, 105, 100, 110, 125, 80, 80, 125 |
| 12 Dayton | 80, 100, 110, 110, 105, 110, 125, 80, 85, 125 |
| 13 Destin | 110, 120, 125, 115, 130, 120, 120, 80, 80, 135 |
| 14 Tijuana | 100, 70, 100, 90, 125, 125, 100, 70, 80, 65 |
| 15 Cozumel | 115, 125, 120, 110, 125, 115, 110, 80, 105, 80 |
| 16 Los Cabos | 115, 115, 120, 110, 135, 125, 110, 80, 110, 80 |
| 17 Sinaloa | 125, 60, 95, 95, 125, 100, 100, 60, 75, 55 |

Podcast Pete's local jokes for the new cities (C(city, ...) lines in `src/content/text.c`, exact text):

- 7: "{rumor} says everything is bigger in {city}. He means his ego." / "{rumor} waited two hours for brisket in Deep Ellum and calls it a personality."
- 8: "{rumor} called it Frisco. Six people corrected him at once." / "{rumor} says the fog in {city} has its own podcast now. It's better than his."
- 9: "{rumor} is pitching his podcast as a streaming series. Nobody called back." / "{rumor} says the 405 is not a freeway, it's a lifestyle."
- 10: "{rumor} says it only rained once in {city}. It started in October." / "{rumor} is on his fourth espresso and his second podcast."
- 11: "{rumor} put chili on spaghetti and called it a three-way. He's not wrong." / "{rumor} says the Nati has more hills than Rome. He has never been to Rome."
- 12: "{rumor} says {city} invented flight, so he invented the podcast." / "{rumor} calls {city} the Land of Funk and will not stop doing the dance."
- 13: "{rumor} says he caught a 46-pound snapper. It was a flip-flop." / "{rumor} anchored at Crab Island in June. Nobody has seen him leave."
- 14: "{rumor} took a picture with a zonkey on La Revu. It's his new profile pic." / "{rumor} says the Caesar salad was invented in {city}, and so was his podcast."
- 15: "{rumor} rented a moped on {city} three days ago. Still looking for the rental shop." / "{rumor} says the reef off Punta Sur is quieter than his podcast. Much quieter."
- 16: "{rumor} sat through a timeshare pitch for free tacos. He bought two timeshares." / "{rumor} says swim at Lover's Beach, never at Divorce Beach. He learned the hard way."
- 17: "{rumor} rode a pulmonia the length of the Malecon twice. The driver wants a cut." / "{rumor} says the aguachile in La Perla made him cry, in a good way."

Border flights: a flight whose origin and destination are in different countries costs `intl_flight_lo..intl_flight_hi` (new params, 350 and 850 dollars) instead of the domestic range, and its news line says the border was crossed, from one of these (new M_FLEW variants for a crossing): "You cross the border to {city}. The ticket cost {amount}." / "Passport out, {city} here you come. That flight cost {amount}."

## Wild cards, finds and the shady crew (Seth, 2026-11-08; Task 2 transcribes this)

Seth asked for three gambles on top of trading. The core owns all of them (deterministic, integer only, saved); the UI only offers the choices.

**Research chemical wild cards.** On arriving somewhere, with chance `rc_offer_permille` (80) when no offer is open and fewer than `rc_max_stakes` (3) stakes are running, a chemist offers a stake in a research chemical. The player may back it with any amount of cash (the money dial) or pass; the offer lasts until the next move. A stake resolves after `rc_days_lo..rc_days_hi` (2..4) days, on that day's arrival: with the chemical's odds it "catches on" and pays the stake times a multiplier drawn from its range; otherwise it busts and the stake is gone. Big or bust, nothing in between.

**Finds.** On arrival, with chance `rc_find_permille` (25) and a free stake slot, the player finds a batch of a research chemical: a free stake with a notional value of `rc_find_lo..rc_find_hi` (1,000..5,000 dollars) that resolves the same way (a hit pays notional times the multiplier; a bust costs nothing).

The chemicals (real research chemicals; name, blurb, odds of catching on in permille, payout multiplier range in percent):

| name | blurb | odds | multiplier |
|---|---|---|---|
| 2C-B | Older than most of the scene and still invited to every party. | 600 | 200..250 |
| 1V-LSD | The newest LSD cousin. Legal until somebody notices. | 550 | 200..300 |
| 4-AcO-DMT | Mushrooms, but from a lab coat instead of a closet. | 550 | 200..300 |
| 3-MMC | Smells like a chemistry set, sells like a rumor. | 450 | 220..320 |
| 2-FDCK | Ketamine's cousin from out of town. | 450 | 220..320 |
| 5-MeO-MiPT | The scene calls it Moxy. Nobody remembers why. | 400 | 250..350 |
| 4-HO-MET | Sounds like a stock ticker. Trades like one too. | 420 | 230..330 |
| Hexen | A bath salt with a marketing budget. | 250 | 300..450 |
| 3-HO-PCP | A deep cut. Either everybody wants it or nobody does. | 300 | 300..450 |
| 4-FA | Speedy, cheerful and banned in three countries. | 400 | 230..330 |

Messages (exact text; `{rc}` is the chemical, `{amount}` money):
- Offer: "A chemist in {hood} has a new research chemical: {rc}. Back it before the scene decides?" / "Word in {hood} is that {rc} might be the next big thing. Want a piece?"
- Find: "You found a forgotten batch of {rc}. If it catches on, it could be worth {amount}."
- Hit: "{rc} caught on in the scene. Your stake pays {amount}!" / "Everybody wants {rc} now. You cash out {amount}."
- Bust (staked): "{rc} flopped. Nobody wants it, and your {amount} is gone." / "The scene passed on {rc}. You lose {amount}."
- Bust (found): "Your batch of {rc} never caught on. Easy come, easy go."

**The shady crew.** At most `cohort_max` (2) times per run, on arrival, with chance `cohort_offer_permille` (40) and only while the player carries goods worth at least `cohort_min_stash` (5,000 dollars at mid prices), a crew offers to cut the stash with fentanyl. The offer lasts until the next move; every offer made counts toward the two, taken or not. Taking it hands over the whole stash and resolves at once:
- With chance 1000 - `cohort_od_permille` (450 in 1000): a huge payout, `cohort_mult_pct` (300) percent of the stash's value at today's prices, at least `cohort_min_payout` (10,000 dollars).
- Otherwise (550 in 1000): people overdose. The stash is gone, the city blacklists the player for the rest of the run (no flights there; a blacklisted city is shown as BANNED), and the crew drives the player out of town to the first hood of a random city that is not blacklisted (a day passes, like a flight with no ticket; on the last day the game ends instead).

These lines are serious; no jokes anywhere near this mechanic, and Podcast Pete never mentions it:
- Offer: "A shady crew in {hood} wants to cut your stash with fentanyl. They promise a huge payout." / "Some people you would rather not know offer to stretch your stash with fentanyl. Big money, they say."
- Paid: "The cut batch moved fast. The crew pays you {amount}. You try not to think about it."
- Overdoses: "The cut batch hit the streets and people overdosed. {city} wants you gone, for good. Your stash is gone."
- Debt sold (follows an overdose, once the crew has moved the player): "Your old lender sold your debt to {lender}. You owe {amount} here now." The rule: any debt owed in the banned city moves to the new city's lender and is added to what is owed there (the old city's debt and its age go to zero; if nothing was owed in the new city, the age starts at zero).
- Prompt: A on DO IT does not accept at once. It opens the game's YES/NO prompt with "Hand your whole stash to the crew?", NO highlighted. YES then A (or a click on YES) accepts; NO, B or a click on NO returns to the same card, still unanswered. WALK AWAY stays B on the card.

Mexico ATMs: with no bank branches in Mexico, the bank screen's blurb in any Mexican hood reads "No branches across the border. Every ATM here charges the fee." (US cities keep "No branch here. The bank symbol on the map marks free banking.").

Rulings of 2026-11-08 (final review): the overdose line now ends "Your stash is gone."; the debt-sold line and its rule are as above; the crew's DO IT asks first; the Mexican ATM line is as above; Sinaloa's sixth hood is Playa Norte (was Culiacan).

New errors: flying to a blacklisted city returns `E_BANNED`; acting on an offer that is not open returns `E_PHASE`. Bots in ph-sim never take offers (balance is unchanged), and one test bot takes every offer to prove nothing breaks over thousands of games.

## Review Focus

1. **Every city works end to end:** each of the 18 can be flown to, ridden around, banked in (fee rules by country) and borrowed in, and every screen draws for each with no overflow.
2. **The border:** crossing in either direction costs the international range and says so; flights within one country do not.
3. **Saves:** a version 3 save is refused cleanly (new game) and version 4 round-trips with the larger arrays.
4. **The flight screen:** all 17 destinations reachable by buttons and by mouse, the tabs switch with L1/R1, nothing scrolls, and the highlighted destination's skyline and boarding pass match.
5. **Determinism across architectures:** the same seed and button script give byte-identical saves on the PC build and on the ARM build under the emulator.
6. **The gambles:** a stake can never be paid twice or lost twice, a find never costs money, the crew can never be offered a third time in a run, and a banned city can never be reached again (by flight, by the overdose relocation, or by a resumed save).
7. **Tone on the crew path:** every line on the fentanyl path is sober; no joke, no celebratory sound or color.

---

### Task 1: Eighteen cities in the core and content

**Files:**
- Modify: `src/content/content.h` (`NUM_CITIES` 18; `CityDef` gains `const char *short_name;` and `uint8_t country;` with `COUNTRY_US 0`, `COUNTRY_MX 1`), `src/content/cities.c` (the table above), `src/content/text.c` (the new jokes and border flight lines), `src/core/params.h`/`params.c` (`intl_flight_lo`, `intl_flight_hi`), the flight cost and M_FLEW code in `src/core` (a crossing flag in the message), `src/core/save.c` (SAVE_VERSION 4; params and any per-city arrays round-trip), `src/text/commands.c` only if ph-text needs it for 18 cities, tests (`t_content`, `t_move` or the flight tests, `t_save`, `t_text`), `tests/golden/basic.out` (deliberately regenerated, diff read and summarized), `docs/balance.md` (new sim table).

**Interfaces:**
- Produces: `CITIES[18]` with `short_name` and `country`; `int game_flight_crosses(const Game *g, int city);` (1 if the flight to `city` crosses the border); M_FLEW's `c` is 1 for a crossing; `game_flight_cost` uses the international range for a crossing.

- [ ] **Step 1: Tests first.** Content: 18 cities in the order above with every field from the table (names, nicknames, short names, codes, airports, transit, hoods, lenders, rates, limits, branches, countries, biases for the new cities); length limits; codes and short names unique; every US city exactly two branches and every Mexican city none; nicknames non-empty. Core: a flight between countries costs within 350..850 and sets the crossing flag, within one country within the domestic range and no flag; ph-text and save round-trips with 18 cities; a version 3 save is refused. Text: each new joke formats for its city; the crossing lines are used only for crossings. Watch them fail.
- [ ] **Step 2: Implement** the table, the fields, the params, the crossing flag and fares, the text, the save version.
- [ ] **Step 3: Golden and balance.** Regenerate the golden transcript (new hood names, 17 flights, a changed random stream if fares draw per city), read the diff, and summarize what changed in the report. Run `ph-sim` (the same runs as `docs/balance.md`) and record the new table; if any Plan 1 balance target in `docs/balance.md` falls outside its tolerance, adjust only the new cities' biases or the international fare range, and say what you changed.
- [ ] **Step 4:** `make test`, `scripts/arm-check.sh`. Commit `feat: eighteen cities across North America`.

### Task 2: Wild cards, finds and the shady crew in the core

**Files:**
- Create: `src/content/chems.c` (the research chemical table), tests in a new module `tests/t_wild.c`
- Modify: `src/content/content.h` (chemical type and table), `src/core/params.h`/`params.c` (every number in the Wild cards section), `src/core/game.h` (stakes, today's offer, the crew counter, the blacklist bitmask, `E_BANNED`, the API below), a new `src/core/wild.c`, the arrival code (offers, finds and resolutions on arrival), `src/core/move.c` (refuse banned cities), `src/core/save.c` (the new state round-trips; still version 4), `src/core/msg.h` and `src/content/text.c` (the new message kinds and exact lines), `tools/sim` (bots never take offers; one test bot takes every offer), `tests/t_content.c` (chemical lint: names ASCII and at most 15 characters, odds and ranges sane).

**Interfaces:**
- Produces: `enum { OFFER_NONE, OFFER_RC, OFFER_CREW };` `g->offer` (kind, chemical, day); `int game_rc_back(Game *g, int64_t stake);` (backs today's chemical offer; E_PHASE without one, E_NO_CASH, E_BAD_ARG for 0); `int game_offer_pass(Game *g);` (declines today's offer); `int game_crew_accept(Game *g);` (takes today's crew offer and resolves it); `int game_city_banned(const Game *g, int city);`; up to 3 running stakes visible to the UI (`g->stakes[]` with chemical, stake, resolve day, found flag); message kinds for offer, find, hit, bust, crew offer, crew payout, overdose.

- [ ] **Step 1: Tests first** (with forced odds through test params): an offer appears on arrival and expires on the next move; backing takes the cash and creates a stake; a stake resolves on its day to exactly stake times the drawn multiplier or to nothing; a find creates a free stake whose bust costs nothing; no more than 3 stakes; the crew offer appears only with a big enough stash and at most twice per run even when declined; accepting pays at least the minimum and takes the stash; the overdose outcome bans the city, clears the stash, moves the player to another city's first hood and passes a day (or ends the game on the last day); flights to a banned city return E_BANNED; everything survives a save and load; the all-offers sim bot plays thousands of games without a crash or a failed sanity check.
- [ ] **Step 2: Implement.** Keep the random stream for games that never see an offer as close as practical to Task 1's; regenerate the golden transcript only if it changes, and summarize the diff.
- [ ] **Step 3:** `make test`, `make sim` runs, `scripts/arm-check.sh`. Commit `feat: research chemical wild cards, finds and the shady crew`.

### Task 3: Eleven new skylines

**Files:**
- Modify: `src/gfx/skyline.c` (11 new backdrop functions, indexed by city), `tests/t_art.c` (the existing per-city tests now cover 18).

- [ ] **Step 1:** Extend the per-city art tests to 18 cities (band containment at (12, 56) and (0, 240), fills its band, all 18 distinct, deterministic, and t = 0 vs a later tick differs for every city that animates). Watch them fail for the new indices.
- [ ] **Step 2:** Draw, in the existing style and palette approach (sky gradient with Bayer dither, far and near layers, one or two landmarks, a ground strip, gentle animation where it fits):
  - Dallas: a ball-topped observation tower and a skyline with green-outlined towers at dusk.
  - San Francisco: rolling fog over hills, an orange-red suspension bridge, a pointed pyramid tower, a cable car on a hill street (animated).
  - Los Angeles: hazy pink-orange sunset, palm rows, hills with a domed observatory, a freeway with moving lights.
  - Seattle: a snowy volcano in the distance, towers, a ferry crossing the bay (animated), drizzle, evergreens, port cranes.
  - Cincinnati: a blue suspension bridge over the river, a riverboat with a turning paddle wheel, hillside houses, the skyline.
  - Dayton: a modest skyline by the river with a vintage biplane crossing the sky (animated) and a trolley bus with overhead wires.
  - Destin: emerald-green water, white sand dunes, charter fishing boats in the harbor, pelicans, a bright sky.
  - Tijuana: hills packed with colorful houses, a tall monumental arch with a clock, a busy avenue, a sunset.
  - Cozumel: turquoise sea and reef, a cruise ship at the pier, palms, a striped lighthouse at the point.
  - Los Cabos: a rock arch standing in the sea at land's end, a marina with boats, desert mountains, a deep blue sky.
  - Sinaloa: a long seaside promenade at sunset with breaking waves, cathedral towers, open-air taxis rolling along it (animated).
- [ ] **Step 3:** Render all 18 bands stacked into one PNG and look at it; fix anything broken; describe the result in the report.
- [ ] **Step 4:** `make test`, `scripts/arm-check.sh`. Commit `feat: skylines for the new cities`.

### Task 4: New lenders' portraits and transit colors

**Files:**
- Modify: `src/gfx/sprites.c` (`art_portrait` for cities 7..17), `src/ui/scr_travel.c` (transit line colors for 18 cities), `tests/t_art.c`.

- [ ] **Step 1:** Tests first: 18 distinct portraits, each inside its 56 x 56 box; 18 transit colors defined and distinct from each neighbor in the list.
- [ ] **Step 2:** Draw the 11 new lenders in the existing portrait style (a face, a hairstyle or hat, one prop that tells the joke): Brisket Brenda (big hair, apron), Sourdough Sam (beanie, a loaf), Influencer Ivy (ring light glow, sunglasses), Espresso Ezra (flannel, coffee cup), Three-Way Wes (chili-red apron, bowl), Propeller Pat (aviator cap and goggles), Snapper Sally (fishing hat, sunburn), Zonkey Zoe (striped zonkey hat), Reef Rita (dive mask on forehead), Timeshare Tim (polo shirt, clipboard), Banda Bea (banda hat and a sash). Transit colors: one per new city, not reused by a neighbor.
- [ ] **Step 3:** Render the 18 portraits into one PNG and look at it. `make test`, `scripts/arm-check.sh`. Commit `feat: portraits and transit colors for the new lenders`.

### Task 5: Flights, offers and every screen for 18 cities

**Files:**
- Modify: `src/ui/scr_travel.c` (fly screen), `src/ui/scr_menus.c` (the title cycles 18 skylines), any screen that assumed 7 cities, `src/text/commands.c` if needed, `tests/t_ui_travel.c`, `tests/t_ui_edges.c` (the sweep covers all 18 cities), other UI tests that assumed 7.

**Interfaces:**
- Consumes: `CITIES[].short_name`, `.country`, `game_flight_crosses`, `game_flight_cost` (Task 1).

- [ ] **Step 0 (offers and bans):** The news shows an offer as a card with two choices: for a chemical, A = BACK IT (opens the money dial for the stake, max = cash) and B = PASS; for the crew, A = DO IT and B = WALK AWAY; the card stays until answered, by buttons or by clicking its two choices. Outcomes arrive as ordinary news cards (the crew's in the TROUBLE style, with no joke anywhere on that path). The market shows running wild cards on the bag panel's bottom line ("WILD: 3-MMC, 2C-B", shortened to fit, in place of the weapon line when any are running). The fly screen shows a banned city with BANNED instead of a fare, and A on it toasts "You're not welcome in {city} anymore." Game over adds wild card wins and losses to the stats grid if it fits, otherwise leaves the grid as is. Tests first for each of these.
- [ ] **Step 1:** Tests first: the fly screen opens on the tab of the current city's country; L1/R1 (and clicking a tab) switch between USA and MEXICO; the grid lists every city of that tab except the current one, in table order, two columns of up to seven; D-pad moves within and across columns; A flies to the highlighted city; a crossing shows on the boarding pass; every destination is reachable by buttons and by mouse; nothing draws off the canvas for any origin city; the title cycle visits all 18 skylines.
- [ ] **Step 2:** Implement: the top band shows the highlighted destination's skyline and nickname with "Lands at <hood 0>"; under it two tab chips (USA, MEXICO; the MEXICO tab is headed "CROSS THE BORDER"); a two-column grid of `CODE short $fare` rows (fares in coral when unaffordable); a compact boarding pass at the bottom (passenger, from and to codes, lands at, fare, and a "BORDER CROSSING" line for a crossing). Keep the screen within 320 x 240 with no scrolling and the mockup's look.
- [ ] **Step 3:** Render the fly screen from OKC (USA tab), from OKC (MEXICO tab), from Tijuana, and the market, move, bank and lender screens for three of the new cities into PNGs and look at them. `make test`, `make game`, `make smoke`, `scripts/arm-check.sh`. Commit `feat: flights to seventeen destinations`.

### Task 6: Play-test on the PC, the emulator and the bots; build the Mini package

**Files:**
- Create: `scripts/watch-pc.sh`, `scripts/watch-emu.sh`, `tools/fbview/` (host SDL viewer for the emulator's framebuffer), `scripts/emu-play.sh` (runs the Miyoo binary under `qemu-arm` in a throwaway Docker container with the armhf C library installed inside it; uses the platform's fake framebuffer hook and the binary's `--frames/--keys` scripted mode; writes frames and the save), `tests/golden/rounds/*.keys` (button scripts for the rounds), `scripts/cross-arch.sh` (same seed and script on the PC build and the ARM build; compares the saves byte for byte).
- Modify: `Makefile` (targets `emu` and `cross-arch`), `docs/HANDOFF.md`, `README.md` (the new cities), VERSION 0.2.0.

- [ ] **Step 1:** Write at least four rounds as button scripts (15-day games): one that trades and rides around Oklahoma City then flies to Dallas and Seattle; one that crosses the border to Tijuana and on to Cozumel and Los Cabos, banking at an ATM; one that borrows in Sinaloa and repays; one that plays to the last day and checks the high score; and one with test params that force a chemical offer (backed), a find, and a crew offer (taken, once paying and once overdosing, then trying to fly back to the banned city). Run each on the PC build (SDL dummy drivers) and on the ARM build under the emulator, capture frames at key moments, and look at them.
- [ ] **Step 1b (Seth wants to watch, 2026-11-08):** add a watch pace to the scripted mode (`--pace MS`: real-time frames, one scripted press every MS milliseconds, default unchanged for the smoke test), `scripts/watch-pc.sh ROUND` (runs a round in a visible SDL window on the desktop at watch pace), a small host viewer `tools/fbview` (SDL, reads the emulator's fake framebuffer file and shows it live, undoing the 180 degree rotation) with `scripts/watch-emu.sh ROUND` (starts the ARM binary under the emulator writing that file, and the viewer showing it), and recordings: each round saved as a video under `build/rounds/` (ffmpeg is installed on the host) plus a contact sheet of key frames. The controller runs the rounds on Seth's desktop so he can watch, and publishes the recordings.
- [ ] **Step 2:** `scripts/cross-arch.sh`: for each round the PC save and the ARM save must be byte-identical.
- [ ] **Step 3:** Run ph-sim over all game lengths and confirm no crashes and the balance table in docs/balance.md.
- [ ] **Step 4:** `scripts/build-miyoo.sh && scripts/package.sh`. Update HANDOFF (how to run the emulator rounds), README and VERSION. Commit `test: emulator and cross-architecture play-tests; 0.2.0`.

### Task 7: Install on Seth's card (controller)

- [ ] `scripts/deploy-sd.sh /run/media/shoots/E010-96EB`, compare the binary on the card with the build, `sync`, and report.
