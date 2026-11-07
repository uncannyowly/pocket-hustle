# Pocket Hustle, Plan 1 of 3: Game Rules Core Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The complete, deterministic, tested rules of Pocket Hustle, playable in a terminal (`ph-text`) and balanced with a headless simulator (`ph-sim`) against screen observations of Beermat's game.

**Architecture:** A C99 core (`src/core`) and content tables (`src/content`). They do no I/O except save files and use integer math only, so the PC and 32-bit ARM give identical results. A terminal frontend and a simulator link the core. Screens, art and sound come in Plan 2, and the Miyoo build and release in Plan 3 (outlined at the end).

**Tech Stack:** C99, GNU make, gcc and clang; Docker image `gta1-miyoo-toolchain` (`arm-linux-gnueabihf-gcc`, glibc 2.28) for an ARM compile check; Wine 11 and Python 3 only in the local research folder.

**Spec:** `docs/superpowers/specs/2026-11-06-pocket-hustle-design.md` (approved 2026-11-06)

## Global Constraints

- Repo: `/mnt/data/git/MiYoo-Mini/pocket-hustle`, local git only in this plan (the GitHub repo comes in Plan 3, private until the first playable build).
- Commit identity (repo-local config): `Seth Hoots <53498983+uncannyowly@users.noreply.github.com>`. Every commit message ends with `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- License: MIT, "Copyright (c) 2026 Seth Hoots".
- No em dashes (U+2014) or en dashes (U+2013) in any repo file or commit message. Enforced by `scripts/check-copy.sh` in `make test`.
- Player-visible strings are printable ASCII (0x20 to 0x7E) only.
- Nothing recognizably Beermat's in game content. Denylist, case-insensitive: `dope wars`, `beermat`, `hardass`, `netmasters`, `peashoot`, `pea shoot`, `catapult`, `chainsaw`, `handbag`, `monkey`, `chicken`, `old lady`.
- No Beermat files, screenshots, observation logs or targets in the repo. They live in `/mnt/data/git/MiYoo-Mini/pocket-hustle-research/` (not a git repo).
- Beermat's game is observed through the screen only: no debugger, memory dumps, API tracing (`WINEDEBUG` relay), decrypting or disassembling. Never run `dw21.exe` (it carries Gator adware).
- Ben Webb's GPL dopewars is a reference for which systems exist and the rough shape of numbers. No code, text or names are copied from it.
- No `float`/`double` in `src/core` or `src/content`. Money is `int64_t` whole dollars, clamped to `MONEY_MAX` = 1,000,000,000,000,000.
- Cities, in this order: Oklahoma City (nickname "Loud City"), Houston, San Jose, New York City, Miami, New Orleans, Portland. Transit, same order: OKC Streetcar, METRORail, VTA Light Rail, the Subway, Metrorail, the St. Charles streetcar, MAX.
- 6 neighborhoods per city, 10 goods, game lengths 15, 30, 60 and 90 days (default 30).
- Saves and high scores are written atomically and fsynced.
- Builds warning-free with `-std=c99 -Wall -Wextra -Werror` on gcc and clang. `src/core` and `src/content` also compile with `arm-linux-gnueabihf-gcc` in `gta1-miyoo-toolchain`.

## Review Focus

1. **A save or high-score file cut short by a power-off, or garbage:** loading reports an error, and the game starts fresh (or the table starts empty). It never crashes or loads half a game. (Task 10: truncated and bit-flipped files.)
2. **Very large amounts** (a 90-day hoarder near `MONEY_MAX`, debts compounding for months): no overflow in price times quantity, interest, net worth or money formatting. (Tasks 4, 5 and 9.)
3. **The final day:** moving on the last day ends the game with no arrival events. Jail on the last day ends the game instead of passing it. A rumor tip dated after the last day is never scheduled. (Tasks 5, 7 and 8.)
4. **A negative final net worth** (died in debt) is recorded as a negative score and sorts below positive ones. (Task 10.)
5. **Actions in the wrong phase** (trading during a chase, chase actions while trading, anything after game over) return `E_PHASE` and change nothing. (Task 7.)

---

## File Structure

```
pocket-hustle/
  Makefile                 build/host/{run-tests,ph-text,ph-sim}; targets test, text, sim, arm-check
  LICENSE  README.md  VERSION (0.0.1)  .gitignore (build/)
  src/core/
    rng.h rng.c            PCG32 random numbers
    params.h params.c      every tunable number, params_default()
    msg.h                  message kinds the core reports to frontends
    game.h                 Game state, error codes, public API
    game_internal.h        helpers shared inside the core
    game.c                 game_new, net worth, messages, day advance, death
    market.c               daily prices, spikes and crashes, price history
    trade.c                buy, sell, carrying
    move.c                 transit, flights, final day
    finance.c              lenders, bank
    encounter.c            arrival events: goons, cops, muggings, rumors
    chase.c                cop chase actions, jail
    shop.c                 gear store, hospital
    rumor.c                tips and jokes
    save.h save.c          save files, high scores, crc32, atomic writes
  src/content/
    content.h              content types and tables
    goods.c cities.c gear.c characters.c
    text.h text.c          message templates, money formatting
  src/text/main.c commands.c   ph-text terminal frontend
  tools/sim/main.c bots.h bots.c report.h report.c   ph-sim
  tests/check.h tests.h helpers.h helpers.c run_tests.c t_*.c
  tests/golden/basic.cmds basic.out run.sh
  scripts/check-copy.sh arm-check.sh
  docs/HANDOFF.md docs/balance.md docs/superpowers/{specs,plans}/
```

The test runner is one binary. `make test` runs every module, the golden transcript and the copy check. `make test T=<module>` runs one module.

---

### Task 1: Repo, test harness and random numbers

**Files:**
- Create: `Makefile`, `LICENSE`, `README.md`, `VERSION`, `.gitignore`, `src/core/rng.h`, `src/core/rng.c`, `tests/check.h`, `tests/tests.h`, `tests/run_tests.c`, `tests/t_rng.c`, `scripts/check-copy.sh`, `scripts/arm-check.sh`, `docs/HANDOFF.md`
- Move: `/home/shoots/docs/superpowers/specs/2026-11-06-pocket-hustle-design.md` to `docs/superpowers/specs/`, and this plan to `docs/superpowers/plans/` (leave the shelved Dope Wars port doc where it is; it does not belong in this repo)

**Interfaces:**
- Produces:
  ```c
  typedef struct { uint64_t state, inc; } Rng;
  void     rng_seed(Rng *r, uint64_t seed, uint64_t seq);   /* pcg32_srandom_r */
  uint32_t rng_u32(Rng *r);                                 /* pcg32_random_r */
  uint32_t rng_below(Rng *r, uint32_t bound);               /* [0, bound), unbiased, bound > 0 */
  int32_t  rng_range(Rng *r, int32_t lo, int32_t hi);       /* inclusive, lo <= hi */
  int      rng_permille(Rng *r, int32_t permille);          /* 1 with probability permille/1000 */
  ```
  `tests/check.h`: `CHECK(cond)`, `CHECK_EQ(a, b)` (compares as `long long`, prints both), `CHECK_STR(a, b)`, global counters `t_checks`, `t_failed`. Each `tests/t_<module>.c` defines `void tests_<module>(void)` and adds one `{"<module>", tests_<module>}` row to `MODULES[]` in `run_tests.c`. `run-tests [module]` prints `N checks, M failed` and exits 1 if any failed.

- [ ] **Step 1: Scaffold.** `git init`, set the repo-local identity, write LICENSE, README (one paragraph: what it is, `make test`, `make text`), VERSION, `.gitignore`. The Makefile has `CFLAGS = -std=c99 -Wall -Wextra -Werror -O2 -g -Isrc`, `HOST_CC ?= cc`, `MIYOO_CC ?= arm-linux-gnueabihf-gcc`, and targets `test`, `text`, `sim`, `arm-check`, `arm-check-inner`, `clean`.
- [ ] **Step 2: Write the failing tests** in `tests/t_rng.c`:
  ```c
  /* pcg-c-basic pcg32-demo, seed 42, seq 54 */
  test_rng_matches_pcg32_reference: rng_seed(&r, 42, 54);
      CHECK_EQ(rng_u32(&r), 0xa15c02b7u); CHECK_EQ(rng_u32(&r), 0x7b47f409u);
      CHECK_EQ(rng_u32(&r), 0xba1d3330u); CHECK_EQ(rng_u32(&r), 0x83d2f293u);
  test_rng_below_bounds: 10000 draws of rng_below(&r, 7): all < 7 and each of 0..6 seen
  test_rng_range_inclusive: 10000 draws of rng_range(&r, -3, 3): all in -3..3, both -3 and 3 seen
  test_rng_permille_edges: rng_permille(&r, 0) never 1 and rng_permille(&r, 1000) always 1 over 1000 draws
  test_rng_same_seed_same_stream: two Rngs seeded (9, 1) give the same 100 values
  ```
- [ ] **Step 3: Run `make test T=rng`.** Expected: link error (undefined `rng_seed`).
- [ ] **Step 4: Implement `src/core/rng.c`** (standard PCG32; `rng_below` uses pcg32_boundedrand's rejection threshold `-bound % bound`).
- [ ] **Step 5: Run `make test T=rng`.** Expected: `N checks, 0 failed`.
- [ ] **Step 6: Write `scripts/check-copy.sh`.** It greps every file from `git ls-files` (plus the commit message file given as `$1`, if any) for U+2014 and U+2013 with `grep -nP`, prints the matches and exits 1. Then wire it into `make test`. To verify, create a scratch tracked file containing an em dash: `make test` must fail. Delete the file: `make test` must pass.
- [ ] **Step 7: Write `scripts/arm-check.sh`**, modeled on `../scorched-emiyoolator/scripts/build-miyoo.sh`: `docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src gta1-miyoo-toolchain make arm-check-inner`. `arm-check-inner` compiles and links `run-tests` with `$(MIYOO_CC)` into `build/arm/`. Run it. Expected: `build/arm/run-tests` exists, and `file` reports ARM EABI5.
- [ ] **Step 8: Write `docs/HANDOFF.md`.** Cover the goal (link the spec), where things are (repo, research folder, Beermat installer path `/mnt/scratch/Games_Wine/drive_c/Games/Dope Wars 2.1/`), the rules from Global Constraints, and how to build and test. Keep it current in every later task.
- [ ] **Step 9: Commit** `chore: scaffold repo, test harness, PCG32 rng`.

---

### Task 2: Content tables and content lint

**Files:**
- Create: `src/content/content.h`, `src/content/goods.c`, `src/content/cities.c`, `src/content/gear.c`, `src/content/characters.c`, `tests/t_content.c`

**Interfaces:**
- Produces:
  ```c
  enum { NUM_GOODS = 10, NUM_CITIES = 7, HOODS_PER_CITY = 6,
         NUM_LOCS = NUM_CITIES * HOODS_PER_CITY, MAX_GEAR = 16 };
  typedef struct { const char *name, *blurb; int32_t lo, hi; } GoodDef;  /* base price range */
  typedef struct {
      const char *name, *nickname;          /* nickname "" if none */
      const char *transit, *airport;
      const char *hoods[HOODS_PER_CITY];
      int16_t bias_pct[NUM_GOODS];          /* 50..200, 100 = no tendency */
      const char *lender;                   /* this city's loan shark */
      int32_t lender_rate_bp;               /* daily interest, basis points */
      int64_t lender_max;                   /* most you can owe this lender */
  } CityDef;
  typedef enum { GEAR_WEAPON, GEAR_SPACE } GearKind;
  typedef struct {
      const char *name, *blurb; GearKind kind; int32_t price;
      int16_t power;      /* weapon: hit chance permille; space: extra capacity */
      int8_t city;        /* -1 = sold everywhere, else only in that city */
  } GearDef;
  extern const GoodDef GOODS[NUM_GOODS];
  extern const CityDef CITIES[NUM_CITIES];
  extern const GearDef GEAR[]; extern const int NUM_GEAR;
  extern const char *const COP_NAME;     /* the cop who chases you */
  extern const char *const RUMOR_NAME;   /* the rumor-monger */
  int content_strings(const char **out, int max);  /* every player-visible content string */
  ```

Drafting rules (the implementer writes the content and Seth reviews it):
- **Goods:** 10 real street drugs in a 2026 mix, with no brand or trademark names. Names up to 10 characters. Each blurb is a one-line joke of up to 60 characters. Prices span about three orders of magnitude (the cheapest `lo` under 100, the priciest `hi` at least 10,000). Starting numbers follow the shape of the public reference, not its values (Task 14 tunes them).
- **Cities:** exact names, order and transit from Global Constraints, and real airport names. Six real, well-known neighborhoods each (up to 16 characters), with no neighborhood described as poor or dangerous anywhere. New York shares at most 2 neighborhoods with {Bronx, Queens, Central Park, Manhattan, Coney Island, Brooklyn}. Each city has at least 2 goods with `bias_pct <= 80` and at least 2 with `>= 125`. Lenders are fictional (names up to 20 characters), and no two cities have the same terms.
- **Gear:** at least 6 comedic weapons and at least 3 space items (names up to 16 characters). At least 2 items are sold in only one city. Weapon power is 100 to 900.
- **Characters:** a fictional cop and a fictional rumor-monger. No real people, and no team names or marks ("Loud City" only as OKC's nickname).

- [ ] **Step 1: Write the failing tests** in `tests/t_content.c`:
  ```c
  test_cities_in_seth_order: CHECK_STR(CITIES[0].name, "Oklahoma City"); CHECK_STR(CITIES[0].nickname, "Loud City");
      names [1..6] "Houston","San Jose","New York City","Miami","New Orleans","Portland";
      transit [0..6] "OKC Streetcar","METRORail","VTA Light Rail","the Subway","Metrorail","the St. Charles streetcar","MAX"
  test_names_fit_and_unique: good names <= 10, hoods <= 16 and unique per city, gear <= 16, lenders <= 20; good and gear names unique
  test_nyc_not_the_classic_list: overlap of CITIES[3].hoods with the classic six <= 2
  test_price_ranges_sane: 0 < lo < hi for all goods; min lo < 100; max hi >= 10000
  test_city_tendencies: every bias in 50..200; per city >= 2 at <= 80 and >= 2 at >= 125
  test_lenders_differ: rate in 1..3000; no two cities with equal (rate, max)
  test_gear_mix: NUM_GEAR <= MAX_GEAR; >= 6 weapons with power 100..900; >= 3 space items with power > 0; >= 2 items with city >= 0; prices > 0
  test_strings_ascii: every content_strings() byte in 0x20..0x7E
  test_strings_not_beermat: no content_strings() entry contains a denylist word (case-insensitive)
  ```
- [ ] **Step 2: Run `make test T=content`.** Expected: link errors.
- [ ] **Step 3: Write the content files** following the drafting rules.
- [ ] **Step 4: Run `make test T=content`.** Expected: 0 failed.
- [ ] **Step 5: Commit** `feat: first draft of cities, goods, gear and characters`.
- [ ] **Step 6: Seth review checkpoint.** Send Seth one short message listing the goods, neighborhoods, lenders, cop, rumor-monger and gear names. Carry on with Task 3. Apply his edits when they arrive (rerun `make test T=content`, commit `content: apply Seth's review`).

---

### Task 3: Rules numbers, game state, messages and the market

**Files:**
- Create: `src/core/params.h`, `src/core/params.c`, `src/core/msg.h`, `src/core/game.h`, `src/core/game_internal.h`, `src/core/game.c`, `src/core/market.c`, `tests/helpers.h`, `tests/helpers.c`, `tests/t_game.c`, `tests/t_market.c`

**Interfaces:**
- Consumes: `Rng` API (Task 1), content tables (Task 2).
- Produces (later tasks use these exact names):
  ```c
  /* params.h: every tunable number; Task 14 tunes params_default() */
  typedef struct {
      int64_t start_cash, start_debt;  int32_t start_city, start_capacity;
      int32_t min_goods, absent_permille;
      int32_t spike_permille, crash_permille, spike_lo_pct, spike_hi_pct, crash_lo_pct, crash_hi_pct;
      int32_t bank_rate_bp, min_payment_pct, enforce_days, enforce_dmg_lo, enforce_dmg_hi;
      int32_t cop_base_permille, cop_per_10k_permille, cop_max_permille;
      int32_t posse_start, posse_growth, max_shooters;
      int32_t run_permille, cop_hit_permille, cop_dmg_lo, cop_dmg_hi;  int64_t loot_lo, loot_hi;
      int32_t drop_permille, drop_lo_pct, drop_hi_pct, jail_fine_pct;
      int32_t mug_permille, mug_lo_pct, mug_hi_pct;
      int32_t rumor_permille, rumor_true_permille, rumor_lead_lo, rumor_lead_hi;
      int32_t hospital_per_hp;  int64_t flight_lo, flight_hi;
  } Params;
  Params params_default(void);

  /* msg.h: what the core reports; a, b, c, amount meanings per kind */
  typedef enum {
      M_SPIKE /*a good, amount price*/, M_CRASH /*a good, amount price*/,
      M_RODE /*a hood*/, M_FLEW /*a city, amount cost*/, M_LAST_DAY, M_GAME_OVER,
      M_GOONS /*a city, amount damage*/, M_DEAD, M_COPS /*a posse*/, M_ESCAPED,
      M_DROPPED /*a good, amount qty*/, M_YOU_HIT /*a cops left*/, M_YOU_MISSED,
      M_HIT_YOU /*amount damage*/, M_COPS_MISSED, M_COPS_BEATEN /*amount loot*/,
      M_JAILED /*amount fine*/, M_MUGGED /*amount*/,
      M_RUMOR_TIP /*a good, b loc index city*6+hood, c 1 spike 0 crash, amount day*/,
      M_RUMOR_JOKE /*a city*/, M_HEALED /*amount hp*/, M_BOUGHT_GEAR /*a gear*/,
      NUM_MSG_KINDS
  } MsgKind;
  typedef struct { uint8_t kind; uint16_t variant; int16_t a, b, c; int64_t amount; } Msg;

  /* game.h */
  #define MONEY_MAX 1000000000000000LL
  enum { MAX_DAYS = 90, MAX_MSGS = 32 };
  typedef enum { OK = 0, E_PHASE = -1, E_BAD_ARG = -2, E_NOT_SOLD = -3, E_NO_CASH = -4,
                 E_NO_SPACE = -5, E_NO_STOCK = -6, E_LIMIT = -7 } Err;
  typedef enum { PH_TRADING, PH_CHASE, PH_OVER } Phase;
  typedef enum { END_NONE, END_DAYS_UP, END_DEAD, END_QUIT } EndReason;
  typedef enum { CH_RUN, CH_FIGHT, CH_SURRENDER } ChaseAction;
  typedef struct { uint8_t city, hood; } Loc;
  typedef struct { int16_t day; Loc loc; int8_t good, spike; } Tip;      /* day 0 = none */
  typedef struct { int16_t posse, cops_left; } Chase;
  typedef struct {
      int32_t moves, flights, cop_chases, cops_beaten, escapes, times_jailed, times_mugged,
              times_dropped, goon_visits, spikes_seen, crashes_seen, rumors_true, rumors_joke;
      int64_t best_net_worth, hospital_spent;
  } Stats;
  typedef struct {
      Params p;  Rng rng, flavor;  uint64_t seed;  char name[16];
      int16_t day, last_day;  Loc loc;
      int64_t cash, bank, debt[NUM_CITIES];  int16_t debt_age[NUM_CITIES];
      int32_t stash[NUM_GOODS];  int64_t stash_paid[NUM_GOODS];
      int32_t capacity;  int16_t health;  int8_t weapon;  uint8_t gear_owned[MAX_GEAR];
      int32_t price[NUM_GOODS];                 /* today's market here, 0 = not sold */
      int32_t hist[NUM_GOODS][MAX_DAYS + 1];    /* price seen here each day, 0 = none */
      Tip tip;  Chase chase;  Phase phase;  EndReason end;  Stats stats;
      Msg msgs[MAX_MSGS];  int16_t nmsgs;
  } Game;
  int     game_new(Game *g, const Params *p, uint64_t seed, int days, const char *name);
  int64_t game_net_worth(const Game *g);        /* cash + bank - all debts */

  /* game_internal.h */
  void     game_msg(Game *g, MsgKind k, int16_t a, int16_t b, int16_t c, int64_t amount);
  void     game_clear_msgs(Game *g);            /* every public action starts with this */
  void     market_roll(Game *g);
  uint64_t mix64(uint64_t x);                   /* splitmix64 finalizer */

  /* tests/helpers.h */
  Params test_params(void);   /* params_default() with every random event off: absent, spike, crash,
                                 cop, mug, rumor, drop = 0; enforce_days = 1000; min_goods = 0 */
  void   new_test_game(Game *g, const Params *p, int days);   /* seed 1, name "Test" */
  int    same_game(const Game *a, const Game *b);              /* memcmp */
  ```

Rules this task pins:
- `game_new` zeroes the whole struct first (so `memcmp` and saving are stable). It accepts only 15, 30, 60 or 90 days (else `E_BAD_ARG`). It seeds `rng` with `(seed, 1)` and `flavor` with `(seed, 2)`. It sets cash, `debt[start_city] = start_debt`, capacity, health 100, weapon -1, day 1, `loc = {start_city, 0}`, then calls `market_roll`.
- `game_msg` picks `variant = rng_u32(&g->flavor) & 0xffff`. Only the flavor stream picks joke variants, so adding jokes never changes gameplay. When `msgs` is full, new messages are dropped.
- `market_roll`, per good in index order:
  1. Absent with `absent_permille`, giving price 0.
  2. Otherwise `base = rng_range(lo, hi) * bias_pct / 100`, minimum 1.
  3. The spike roll comes first (`spike_permille`, multiplier `rng_range(spike_lo_pct, spike_hi_pct)`). If no spike, the crash roll (`crash_permille`). Each fires `M_SPIKE`/`M_CRASH` and counts in stats.
  4. Then, if fewer than `min_goods` are priced, absent goods are re-priced (no event) in index order starting from `rng_below(NUM_GOODS)` until there are `min_goods`.
  5. Finally `hist[good][day] = price[good]`.
  6. Task 8 adds tip handling.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_new_game_start_state: Params p = params_default(); CHECK_EQ(game_new(&g,&p,1,30,"Seth"), OK);
      day 1, last_day 30, loc.city == p.start_city, loc.hood 0, cash == p.start_cash,
      debt[p.start_city] == p.start_debt, health 100, weapon -1, phase PH_TRADING, end END_NONE
  test_new_game_lengths: 15/30/60/90 -> OK; 0, 20, 91 -> E_BAD_ARG
  test_params_default_sane: every *_permille in 0..1000; every lo <= hi; start_city in 0..6;
      bank_rate_bp in 0..3000 (keeps balance * rate inside int64 at MONEY_MAX)
  test_net_worth: cash 1000, bank 500, debt[0] 300, debt[2] 200 -> 1000
  test_market_within_biased_range: test_params(); seeds 1..200: each price in
      [max(1, lo*bias/100), hi*bias/100] for the city
  test_market_min_goods: absent 1000, min_goods 4 -> exactly 4 nonzero prices
  test_market_spike_doubles: spike 1000, spike_lo/hi 200/200 -> every price >= 2*max(1,lo*bias/100),
      and one M_SPIKE per good with amount == price
  test_market_history: after market_roll, hist[i][day] == price[i] for every good
  test_market_deterministic: two games with seed 5 have identical price arrays
  test_flavor_independent: games A and B with seed 5; draw 7 extra from B.flavor; market_roll both -> same prices
  ```
- [ ] **Step 2: Run `make test T=game` and `make test T=market`.** Expected: link errors.
- [ ] **Step 3: Implement** `params.c` (starting defaults in the shape of the public reference, not its values), `game.c` (`game_new`, `game_net_worth`, `game_msg`, `game_clear_msgs`, `mix64`), `market.c` and `tests/helpers.c`.
- [ ] **Step 4: Run both modules.** Expected: 0 failed.
- [ ] **Step 5: Commit** `feat: game state, params and daily market`.

---

### Task 4: Trading

**Files:**
- Create: `src/core/trade.c`, `tests/t_trade.c`
- Modify: `src/core/game.h` (prototypes)

**Interfaces:**
- Consumes: `Game`, `Err`, helpers (Task 3).
- Produces: `int game_buy(Game *g, int good, int32_t qty)`, `int game_sell(Game *g, int good, int32_t qty)`, `int32_t game_max_buy(const Game *g, int good)`, `int32_t game_carried(const Game *g)`.

Rules: both actions need `PH_TRADING` (`E_PHASE`). Check in this order: bad good index or `qty <= 0` gives `E_BAD_ARG`, `price == 0` gives `E_NOT_SOLD`, then (buy) `E_NO_CASH`, then `E_NO_SPACE`, or (sell) `E_NO_STOCK`. All multiplication is in `int64_t`. A sale reduces `stash_paid` proportionally (`stash_paid -= stash_paid * qty / stash`, computed before `stash` changes). `game_max_buy` = min(cash / price, capacity - carried), or 0 if not sold. Selling only works where the good is on the market today.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_buy_basic: price[0] = 100, cash 1000 -> game_buy(&g,0,3) OK; cash 700, stash[0] 3, stash_paid[0] 300
  test_buy_errors_change_nothing: snapshot; not sold -> E_NOT_SOLD; qty 0 and -1 -> E_BAD_ARG;
      too dear -> E_NO_CASH; over capacity -> E_NO_SPACE; same_game(snapshot) after each
  test_sell_basic: stash[1] 4, stash_paid[1] 400, price[1] 250 -> sell 1: cash +250, stash 3, stash_paid 300
  test_sell_more_than_owned: E_NO_STOCK, unchanged
  test_max_buy: cash 950, price 100, capacity 20, carried 15 -> 5; cash 350 -> 3; not sold -> 0
  test_huge_cash_no_overflow: cash = MONEY_MAX, price[0] = 2000000000, capacity 100, carried 0 ->
      max_buy 100; buy 100 OK; cash == MONEY_MAX - 200000000000
  ```
- [ ] **Step 2: Run `make test T=trade`.** Expected: link errors.
- [ ] **Step 3: Implement `trade.c`.**
- [ ] **Step 4: Run `make test T=trade`.** Expected: 0 failed.
- [ ] **Step 5: Commit** `feat: buying and selling`.

---

### Task 5: Moving, days, interest and the final day

**Files:**
- Create: `src/core/move.c`, `src/core/encounter.c` (only `encounters_on_arrival`, empty for now), `tests/t_move.c`
- Modify: `src/core/game.h`, `src/core/game_internal.h`, `src/core/game.c`

**Interfaces:**
- Consumes: Tasks 3 and 4.
- Produces: `int game_move(Game *g, Loc dest)`, `int64_t game_flight_cost(const Game *g, int city)`, `int game_end(Game *g)`. Internal: `void game_advance_day(Game *g)`, `void game_check_death(Game *g)`, `void encounters_on_arrival(Game *g)`.

Rules:
- `game_move` steps, in order:
  1. Needs `PH_TRADING`.
  2. `dest` must be valid and differ from `loc` (`E_BAD_ARG`).
  3. A different city costs `game_flight_cost` (`E_NO_CASH` if short). A move within the city is free.
  4. **If `day == last_day`, the game ends instead:** `PH_OVER`, `END_DAYS_UP`, `M_GAME_OVER`. Nothing else happens: no travel, no payment, no events.
  5. Otherwise: pay, `game_advance_day`, set `loc`, count `moves`/`flights`, then push `M_RODE`/`M_FLEW`.
  6. Then `market_roll`. If `day == last_day`, push `M_LAST_DAY`.
  7. Then `encounters_on_arrival`.
- `game_advance_day`: `day++`. For each `debt[c] > 0`: `debt[c] += (debt[c] * rate_bp + 9999) / 10000`, `debt_age[c]++`. Then `bank += bank * bank_rate_bp / 10000`. Clamp every balance to `MONEY_MAX`, and update `stats.best_net_worth`.
- `game_flight_cost`: 0 within the same city. Otherwise `flight_lo + mix64(seed ^ ((uint64_t)day << 16) ^ ((uint64_t)from << 8) ^ to) % (flight_hi - flight_lo + 1)`. That makes it stable until you move, so the UI can show it.
- `game_check_death`: if `health <= 0`, then `health = 0`, `PH_OVER`, `END_DEAD`, `M_DEAD`.
- `game_end`: from any phase except `PH_OVER` gives `END_QUIT` and `PH_OVER`. From `PH_OVER` it returns `E_PHASE`.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_transit_free_one_day: move to {city, 1} -> OK, day 2, cash unchanged, loc.hood 1, msgs has M_RODE a=1
  test_flight_pays_ticket: c = game_flight_cost(&g, 3); c in [flight_lo, flight_hi]; move {3,0} ->
      cash -= c, M_FLEW a=3 amount=c, stats.flights 1
  test_flight_needs_cash: cash 0 -> E_NO_CASH, unchanged
  test_flight_cost_stable: same value on two calls; game_flight_cost(&g, g.loc.city) == 0
  test_bad_destinations: current loc, city 7, hood 6 -> E_BAD_ARG
  test_debt_interest_rounds_up: r = CITIES[c].lender_rate_bp, debt[c] 5500 -> after a transit move
      debt[c] == 5500 + (5500 * r + 9999) / 10000, debt_age[c] 1
  test_bank_interest_rounds_down: bank_rate 150 bp, bank 1000 -> 1015; bank 99 -> 100
  test_final_day_ends_without_events: 15-day game, day = 15, cop/mug/rumor/spike 1000 -> move:
      OK, PH_OVER, END_DAYS_UP, day 15, loc unchanged, nmsgs 1 (M_GAME_OVER)
  test_last_day_warning: day 14 of 15 -> move -> M_LAST_DAY present, phase PH_TRADING
  test_compound_debt_clamped: debt[c] = MONEY_MAX - 5 (any content rate adds more than 5) -> move -> debt[c] == MONEY_MAX
  test_end_quits: game_end -> END_QUIT, PH_OVER; second game_end -> E_PHASE
  ```
- [ ] **Step 2: Run `make test T=move`.** Expected: link errors.
- [ ] **Step 3: Implement** `move.c`, the `game.c` additions, and the empty `encounters_on_arrival`.
- [ ] **Step 4: Run `make test`.** Expected: all modules pass.
- [ ] **Step 5: Commit** `feat: travel, days, interest, final day`.

---

### Task 6: Lenders, bank and goons

**Files:**
- Create: `src/core/finance.c`, `tests/t_finance.c`
- Modify: `src/core/encounter.c`, `src/core/game.h`

**Interfaces:**
- Consumes: Task 5 (`encounters_on_arrival`, `game_check_death`).
- Produces: `int game_borrow(Game *g, int64_t amt)`, `int game_repay(Game *g, int64_t amt)`, `int game_deposit(Game *g, int64_t amt)`, `int game_withdraw(Game *g, int64_t amt)`. Inside `encounter.c`: `static int goons_check(Game *g)` returns 1 if goons came.

Rules:
- **All four actions:** they need `PH_TRADING`, and `amt <= 0` gives `E_BAD_ARG`.
- **Borrow and repay** use the current city's lender. Borrowing past `lender_max` gives `E_LIMIT`. Repaying more than cash gives `E_NO_CASH`. Repaying more than the debt gives `E_BAD_ARG`.
- **The lender's clock:** a repayment resets `debt_age[city]` to 0 only if it clears the debt or is at least `max(1, debt * min_payment_pct / 100)`. Borrowing into a zero balance also resets it.
- **The bank** works in any city.
- **Goons:** on arrival (transit or flight), if `debt[city] > 0 && debt_age[city] > enforce_days`, they deal damage `rng_range(enforce_dmg_lo, enforce_dmg_hi)`. That pushes `M_GOONS` (a=city, amount=damage), counts `goon_visits`, sets `debt_age[city] = 0`, then runs `game_check_death`. When goons come, no other trouble rolls on that arrival (Task 7 relies on this).

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_borrow_to_limit: debt[c] = lender_max - 100 -> borrow 100 OK; borrow 1 -> E_LIMIT
  test_repay_errors: repay > cash -> E_NO_CASH; > debt -> E_BAD_ARG; 0 -> E_BAD_ARG
  test_small_payment_keeps_clock: debt 10000, age 5, min_payment_pct 10 -> repay 999: age 5; repay 1000: age 0
  test_bank_moves: deposit 300 of cash 1000 -> cash 700 bank 300; withdraw 400 -> E_NO_CASH; withdraw 300 OK
  test_goons_when_overdue: enforce_days 3, debt_age[city] 4, dmg 10..10 -> transit move:
      health 90, M_GOONS a=city amount=10, debt_age 0, goon_visits 1
  test_goons_only_in_their_city: overdue in city 0, player moving inside city 1 -> no M_GOONS
  test_goons_can_kill: health 5, dmg 10..10 -> PH_OVER, END_DEAD, M_DEAD
  ```
- [ ] **Step 2: Run `make test T=finance`.** Expected: link errors.
- [ ] **Step 3: Implement** `finance.c` and the goons check.
- [ ] **Step 4: Run `make test`.** Expected: all pass.
- [ ] **Step 5: Commit** `feat: lenders, bank, goons`.

---

### Task 7: Cops, muggings, jail, hospital and the store

**Files:**
- Create: `src/core/chase.c`, `src/core/shop.c`, `tests/t_chase.c`, `tests/t_shop.c`
- Modify: `src/core/encounter.c`, `src/core/game.h` (the earlier actions already check the phase; this task's last test proves it)

**Interfaces:**
- Consumes: Tasks 3 to 6.
- Produces: `int game_chase(Game *g, ChaseAction a)`, `int game_heal(Game *g, int32_t hp)`, `int game_buy_gear(Game *g, int gear)`, `int game_gear_available(const Game *g, int gear)`, `int64_t game_stash_value(const Game *g)` (= sum of `stash[i] * (lo + hi) / 2`).

Rules:
- **Arrival order in `encounters_on_arrival`:** goons, else cops, else mugging (at most one kind of trouble per arrival). The rumor check comes after (Task 8).
- **Cops** appear with `min(cop_max_permille, cop_base_permille + cop_per_10k_permille * (stash_value / 10000))`. Then `posse = posse_start + posse_growth * stats.cop_chases`, `cops_left = posse + 1`, `cop_chases++`, `PH_CHASE`, and `M_COPS` (a=posse).
- **`game_chase`** needs `PH_CHASE`.
  - **`CH_RUN`:** escape with `run_permille`, giving `M_ESCAPED`, `escapes++` and `PH_TRADING`. Then, if the player carries anything, a drop with `drop_permille`: the first good with stock, starting from `rng_below(NUM_GOODS)`, loses `stash * rng_range(drop_lo_pct, drop_hi_pct) / 100` (at least 1), with `stash_paid` reduced in proportion. That gives `M_DROPPED` and `times_dropped++`. If the run fails, the cops shoot.
  - **`CH_FIGHT`:** with no weapon, `E_BAD_ARG`. A hit with `GEAR[weapon].power` permille gives `cops_left--` and `M_YOU_HIT` (a=cops_left), otherwise `M_YOU_MISSED`. When `cops_left == 0`: loot `rng_range(loot_lo, loot_hi)` to cash, `M_COPS_BEATEN`, `cops_beaten++` and `PH_TRADING`. Otherwise the cops shoot.
  - **`CH_SURRENDER`:** stash and `stash_paid` are cleared, the fine is `cash * jail_fine_pct / 100`, and `times_jailed++`. If `day == last_day`, the game ends (`PH_OVER`, `END_DAYS_UP`). Otherwise `game_advance_day`, `M_JAILED` (amount=fine), `PH_TRADING`, `market_roll`, and `M_LAST_DAY` if that is now the case.
  - **The cops shoot:** each of `min(cops_left, max_shooters)` cops hits with `cop_hit_permille` (`M_HIT_YOU`, damage `rng_range(cop_dmg_lo, cop_dmg_hi)`) or misses (`M_COPS_MISSED`), then `game_check_death`.
- **Muggings** happen only if there were no goons or cops and `cash > 0`. With `mug_permille`, the player loses `cash * rng_range(mug_lo_pct, mug_hi_pct) / 100`, giving `M_MUGGED` and `times_mugged++`. The bank is untouched.
- **`game_heal`:** `hp <= 0` gives `E_BAD_ARG`, `health + hp > 100` gives `E_LIMIT` (so healing at full health is `E_LIMIT`), and the cost is `hp * hospital_per_hp` (`E_NO_CASH`). Success gives `M_HEALED` and adds to `hospital_spent`.
- **Gear:** available if `GEAR[i].city == -1` or equals the current city (else `E_NOT_SOLD`). A weapon replaces the current one with no refund. A space item adds `power` to capacity and can be bought once (`gear_owned`, a second buy gives `E_LIMIT`). Success gives `M_BOUGHT_GEAR`.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_cops_start_chase: cop_base 1000 -> after a move: PH_CHASE, M_COPS; game_buy -> E_PHASE
  test_posse_grows: posse_start 2, growth 1: first chase cops_left 3; with stats.cop_chases = 3 beforehand, posse 5
  test_run_escapes: run 1000, drop 0 -> PH_TRADING, M_ESCAPED, escapes 1
  test_run_fails_shot: run 0, cop_hit 1000, dmg 7..7, max_shooters 2, cops_left 5 ->
      health 86, two M_HIT_YOU, still PH_CHASE
  test_drop_on_escape: run 1000, drop 1000, drop 50..50, only stash[2] = 10 -> stash[2] 5, M_DROPPED a=2 amount=5
  test_fight_needs_weapon: weapon -1 -> E_BAD_ARG
  test_fight_until_won: weapon = a GEAR weapon, cop_hit 0, loot 500..500, cops_left 2: call CH_FIGHT up to 200 times
      until PH_TRADING; each call adds M_YOU_HIT or M_YOU_MISSED; ends with cash +500, cops_beaten 1
  test_surrender_jails: stash[1] 10, cash 1000, fine 20 -> stash 0, cash 800, day +1, M_JAILED amount 200, PH_TRADING
  test_jail_on_final_day_ends: day == last_day -> surrender -> PH_OVER, END_DAYS_UP, day unchanged
  test_mugging_pocket_only: mug 1000, 25..25, cash 1000, bank 500 -> cash 750, bank 500, M_MUGGED 250
  test_no_mugging_with_cops: cop 1000, mug 1000 -> M_COPS only, cash unchanged
  test_heal: health 60, 10/hp: heal 50 -> E_LIMIT; heal 40 -> health 100, cash -400; heal 1 -> E_LIMIT; cash 0 -> E_NO_CASH
  test_gear_rules: city-only item elsewhere -> E_NOT_SOLD; space item adds capacity, second buy -> E_LIMIT;
      buying a second weapon replaces g.weapon
  test_wrong_phase_changes_nothing: in PH_CHASE: buy, sell, move, borrow, repay, deposit, withdraw, heal,
      buy_gear -> E_PHASE; in PH_TRADING: game_chase -> E_PHASE; after game_end: every action incl. game_chase
      -> E_PHASE; same_game(snapshot) after each
  ```
- [ ] **Step 2: Run `make test T=chase` and `make test T=shop`.** Expected: link errors.
- [ ] **Step 3: Implement** `chase.c`, `shop.c` and the arrival order.
- [ ] **Step 4: Run `make test`.** Expected: all pass.
- [ ] **Step 5: Commit** `feat: cops, muggings, jail, hospital, store`.

---

### Task 8: Rumors

**Files:**
- Create: `src/core/rumor.c`, `tests/t_rumor.c`
- Modify: `src/core/encounter.c`, `src/core/market.c`, `src/core/game.c` (tip expiry in `game_advance_day`)

**Interfaces:**
- Consumes: Tasks 3 to 7.
- Produces: internal `void rumor_check(Game *g)`, called last in `encounters_on_arrival`.

Rules:
- **When a rumor comes:** if `PH_TRADING` after the trouble checks and `rng_permille(rumor_permille)`, then:
  - **A true tip** if `rng_permille(rumor_true_permille)`, `tip.day == 0`, and `day + lead <= last_day` (where `lead = rng_range(rumor_lead_lo, rumor_lead_hi)`). The location is drawn from the `NUM_LOCS - 1` places other than here, the good is `rng_below(NUM_GOODS)`, and spike is `rng_below(2)`. That gives `M_RUMOR_TIP` and `rumors_true++`.
  - **Otherwise a joke:** `M_RUMOR_JOKE` (a=current city) and `rumors_joke++`.
- **When the tip comes true:** `market_roll`, on the tip's day and place, prices that good (even if absent) with the tip's spike or crash multiplier and its message, then clears `tip`.
- **Expiry:** `game_advance_day` clears a tip whose day has passed.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_true_tip_comes_true: rumor 1000, true 1000, lead 1..1, cash 10^9 -> move: M_RUMOR_TIP (save good, loc, spike);
      set g.p.rumor_permille = 0; move to that loc -> price[good] > 0 and the matching M_SPIKE or M_CRASH
      for that good; tip.day 0
  test_no_tip_past_last_day: 15-day game, day 13 -> move (arrive day 14), lead 3..3 -> M_RUMOR_JOKE, tip.day 0
  test_missed_tip_expires: day 1, tip = {day 3, some other loc}, rumor 0; move three times, never to the tip's
      place -> day 4 and tip.day 0
  test_false_rumor_is_joke: true 0 -> M_RUMOR_JOKE a = loc.city
  ```
- [ ] **Step 2: Run `make test T=rumor`.** Expected: FAIL (no tip messages).
- [ ] **Step 3: Implement.**
- [ ] **Step 4: Run `make test`.** Expected: all pass.
- [ ] **Step 5: Commit** `feat: rumors and tips`.

---

### Task 9: Game text

**Files:**
- Create: `src/content/text.h`, `src/content/text.c`, `tests/t_text.c`
- Modify: `src/content/characters.c` (`content_strings` also returns every template)

**Interfaces:**
- Consumes: `Game`, `Msg`, `MsgKind` (Task 3), content tables (Task 2).
- Produces: `size_t text_msg(const Game *g, const Msg *m, char *buf, size_t n)` (returns the length; always NUL-terminated) and `void fmt_money(int64_t v, char *buf, size_t n)` (`$1,234`, `-$5,500`).

Rules:
- **Templates** are arrays of `{ int8_t city; const char *t; }` per kind (city -1 = any city). A message picks among the eligible lines with `variant % count`. City matters only for `M_RUMOR_JOKE` (the eligible lines are the general ones plus those for city `a`).
- **Placeholders:** `{good}` `{hood}` `{city}` `{lender}` `{cop}` `{rumor}` `{name}` `{amount}` (money) `{n}` (a as a number) `{loc}` ("Hood, City" from b) `{day}` (amount as "day N"). An allowed-placeholder table per kind is part of `text.c`.
- **How many lines:** at least 3 for each of M_SPIKE, M_CRASH, M_COPS, M_ESCAPED, M_MUGGED, M_GOONS, M_JAILED, M_DROPPED, M_HIT_YOU and M_YOU_MISSED; at least 1 for every other kind. `M_RUMOR_JOKE` needs at least 14 general lines and at least 3 for each city with local flavor.
- **Tone:** irreverent and silly, punching up. Write new jokes, nothing from Beermat or GPL dopewars.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_money_format: 0 "$0"; 1234 "$1,234"; -5500 "-$5,500"; MONEY_MAX "$1,000,000,000,000,000"
  test_every_kind_has_text: each kind with a=b=c=0, amount=1: length > 0, no '{' or '}' in the output
  test_placeholders_allowed: every template uses only its kind's allowed placeholders
  test_line_counts: the minimums above
  test_city_jokes: M_RUMOR_JOKE a=4, variants 0..999: every output is a general or Miami line; a Miami line appears
  test_lengths_worst_case: every kind and every line with the longest names and amount = MONEY_MAX: length <= 120
  ```
  The content lint tests from Task 2 (ASCII, denylist) now cover templates automatically.
- [ ] **Step 2: Run `make test T=text`.** Expected: link errors.
- [ ] **Step 3: Write `text.c`** and the jokes.
- [ ] **Step 4: Run `make test`.** Expected: all pass.
- [ ] **Step 5: Commit** `feat: game text and jokes`.
- [ ] **Step 6: Seth tone checkpoint.** Send Seth about 15 sample lines (spikes, cops, a few city jokes). Carry on, and apply his notes when they arrive.

---

### Task 10: High scores and saves

**Files:**
- Create: `src/core/save.h`, `src/core/save.c`, `tests/t_save.c`

**Interfaces:**
- Consumes: `Game` (Task 3).
- Produces:
  ```c
  typedef struct { char name[16]; int64_t score; uint8_t end; } HiScore;
  typedef struct { HiScore t[4][10]; uint8_t n[4]; } HiTable;
  int      days_index(int days);                     /* 15,30,60,90 -> 0..3, else -1 */
  int      hiscore_insert(HiTable *t, int days, const char *name, int64_t score, EndReason end); /* rank 0..9 or -1 */
  uint32_t crc32_buf(const void *p, size_t n);       /* IEEE, reflected, poly 0xEDB88320 */
  int      atomic_write_file(const char *path, const void *buf, size_t n);  /* 0 or -1 */
  int      save_write(const Game *g, const char *path);       /* 0 or -1 */
  int      save_read(Game *g, const char *path);              /* 0; -1 missing; -2 corrupt or other version */
  int      hiscores_write(const HiTable *t, const char *path);
  int      hiscores_read(HiTable *t, const char *path);       /* on error *t is zeroed; -1 or -2 */
  ```

Rules:
- **File layout.** A 16-byte header: magic (`PHSV` for saves, `PHHS` for scores), u16 version 1, u16 0, u32 payload length, u32 `crc32_buf(payload)`. Everything is little endian.
- **The save payload** is every `Game` field in declaration order as fixed-width little-endian integers (including `Params` and both `Rng`s). `name` is 16 raw bytes. `msgs` is not saved, and `nmsgs` loads as 0.
- **Loading** decodes into a zeroed temporary `Game` and copies it to `*g` only on success, so a failed load leaves `*g` untouched.
- **Atomic writes:** write `path.tmp`, `fsync`, `rename`, then `fsync` the directory.
- **High scores** are sorted by score, highest first. Ties go to the earlier entry. Each table keeps 10.

- [ ] **Step 1: Write the failing tests.**
  ```c
  test_crc32_check_value: crc32_buf("123456789", 9) == 0xCBF43926
  test_save_round_trip: seed 7, buy, three moves -> save_write; save_read into g2 -> OK;
      saving g2 produces a byte-identical file; one more identical move on both -> same prices and cash
  test_truncated_save: for every length 0..len-1 step 7: truncated copy -> save_read == -2, target unchanged
  test_bitflip_save: flip one payload byte -> -2
  test_other_version: version byte 2 -> -2
  test_missing_save: -1
  test_hiscores_order: insert 500, -2000, 9000 in the 30-day table -> order 9000, 500, -2000; ranks 0, 1, 0
  test_hiscores_top_ten: 11 inserts -> n == 10, lowest gone; a score below the 10th -> -1
  test_hiscores_corrupt_file: garbage file -> hiscores_read < 0 and table all zero
  test_atomic_write_no_temp_left: after a write the file exists and path.tmp does not
  ```
  Tests write into a fresh `mkdtemp` directory under `build/`.
- [ ] **Step 2: Run `make test T=save`.** Expected: link errors.
- [ ] **Step 3: Implement `save.c`.**
- [ ] **Step 4: Run `make test`.** Expected: all pass. Then run `scripts/arm-check.sh`. Expected: it still links for ARM.
- [ ] **Step 5: Commit** `feat: saves and high scores`.

---

### Task 11: `ph-text`, the terminal version

**Files:**
- Create: `src/text/main.c`, `src/text/commands.c`, `tests/golden/basic.cmds`, `tests/golden/basic.out`, `tests/golden/run.sh`
- Modify: `Makefile` (`text` target; `make test` runs `tests/golden/run.sh`)

**Interfaces:**
- Consumes: the whole core API, `text_msg`, `fmt_money`, saves and high scores.
- Produces: `build/host/ph-text [--seed N] [--days 15|30|60|90] [--name NAME] [--save PATH] [--scores PATH] [--script]`.
  - Seed default: time-based, and printed at start.
  - `--save`: resume if the file loads, autosave after every command that changes the day, delete at game over.
  - `--script`: no prompt; echo each input line as `> line`.
  - Commands: `status`, `market`, `buy <good> <qty|max>`, `sell <good> <qty|all>`, `go <hood>`, `fly <city>` (arrives at hood 0), `flights`, `borrow <amt>`, `repay <amt|all>`, `deposit <amt|all>`, `withdraw <amt|all>`, `store`, `gear <n>`, `heal <hp|all>`, `run`, `fight`, `surrender`, `chart <good>` (that good's `hist` so far), `scores`, `quit`.
  - Error codes print one short line each: "Not sold here.", "Not enough cash.", "No room.", "You don't have that many.", "Can't do that now.", "That's over the limit.", "Huh?"

Output rules:
- After each command, every message prints as `text_msg` output, one per line. Then the status block, when the day changed or on `status`:
  ```
  Day 3/30  Oklahoma City: <hood>  Health 100
  Cash $1,800  Bank $0  Owed $5,000
  Carrying 0/100
   # Good        Price   Have
   0 <name>     $1,234      0
  ```
- At game over it prints the final net worth, the end reason, the stats and the high-score rank.

- [ ] **Step 1: Write `tests/golden/basic.cmds`.** About 40 lines that touch every command at least once, for `--seed 1 --days 15 --script`. Commands that fail in that seed's situation are fine (their error lines are part of the transcript). Write `run.sh` to diff the output against `basic.out`.
- [ ] **Step 2: Run `make test`.** Expected: FAIL (no `ph-text`).
- [ ] **Step 3: Implement `main.c` and `commands.c`.**
- [ ] **Step 4: Generate `basic.out`** by running it once. Read it in full, checking that the numbers add up line by line, there are no leftover `{`, and no line is longer than 120 characters. Commit it.
- [ ] **Step 5: Run `make test`.** Expected: all pass, golden included.
- [ ] **Step 6: Commit** `feat: ph-text terminal version`.

---

### Task 12: Watch Beermat's game (research, outside the repo)

**Files (all in `/mnt/data/git/MiYoo-Mini/pocket-hustle-research/`, never in the repo):** `tools/wise_extract.py`, `tools/targets.py`, `extract/`, `wineprefix/`, `shots/`, `prices.csv`, `events.csv`, `balances.csv`, `notes.md`, `targets.txt`

**Interfaces:**
- Produces `targets.txt`, one `key value...` per line, read by Task 13:
  ```
  start_cash N
  start_debt N
  debt_daily_pct X
  bank_daily_pct X
  capacity N
  spikes_per_move X
  crashes_per_move X
  cops_per_move X
  mugs_per_move X
  price <Good Name> <lowest seen> <highest seen>
  ```

- [ ] **Step 1: Ask Seth** whether he's OK with a Wine window opening on his desktop for an hour or two, or whether he'd rather install Xvfb first (`sudo pacman -S xorg-server-xvfb`; he runs it). Wait for the answer.
- [ ] **Step 2: Extract without running the installer.** `tools/wise_extract.py` (run with `python3 -I`) walks the raw deflate streams of `dw21.exe` from offset 0x3aba. It identifies `DopeWars.exe` (PE whose sections are all `.PCGW32`), `OLChk32.dll` (version info "O'License"), `cities.txt` and `oldlady.txt` by content, plus the font and WAVs, and writes them under `extract/` with their WiseScript names. Expected: `extract/DopeWars.exe` is 1,487,360 bytes. `Trickler.exe` and `WiseUpdt.exe` are not written.
- [ ] **Step 3: Start the game** in a throwaway prefix: `WINEPREFIX=$PWD/wineprefix wine extract/DopeWars.exe`. Expected: the main window titled "Dope Wars" appears. If PC Guard refuses to start under Wine, write that in `notes.md`, don't try to get around it, and ask Seth for sessions he plays himself, taking a screenshot each day.
- [ ] **Step 4: Play three 30-day games**, driving the game with `xdotool`. Capture each day with `import -window <id> shots/g<game>_d<day>.png`, and read the screenshots. Log rows:
  - `prices.csv`: `game,day,city,location,good,price` (blank price = not sold)
  - `balances.csv`: `game,day,cash,bank,debt`
  - `events.csv`: `game,day,kind` (spike, crash, cops, mugging, goons, rumor, drop, jail)

  Also note the start capacity, store and hospital prices in `notes.md`. Events are logged by kind only: never copy the game's wording.
- [ ] **Step 5: Write `tools/targets.py`** (CSVs to `targets.txt`). Interest comes from day-over-day debt and bank changes on days with no payments. Rates per move are the event counts over the moves made. Run it. Expected: every key above is present.
- [ ] **Step 6: Write a short summary in `notes.md`** (what was observed, how many days, gaps). Nothing gets committed to the repo.

---

### Task 13: `ph-sim`, the balance simulator

**Files:**
- Create: `tools/sim/main.c`, `tools/sim/bots.h`, `tools/sim/bots.c`, `tools/sim/report.h`, `tools/sim/report.c`, `tests/t_report.c`
- Modify: `Makefile` (`sim` target; `report.c` is also linked into `run-tests`)

**Interfaces:**
- Consumes: the core API (Tasks 3 to 10). Floats are fine here (the simulator is not core).
- Produces:
  ```c
  typedef enum { BOT_GREEDY, BOT_CAUTIOUS } BotKind;
  void bot_play_day(Game *g, BotKind k);      /* trades, then moves (or resolves a chase) once */
  typedef enum { V_OK, V_LOW, V_HIGH, V_TOO_CLOSE } Verdict;
  Verdict compare_value(double ours, double theirs, int must_differ);
  ```
  `build/host/ph-sim --games N --seed S --days D --bot greedy|cautious [--targets FILE] [--strict]`

Rules:
- **The greedy bot** sells a good when its price is at least 1.3 times what it paid on average. It buys the priced good that is cheapest relative to its range midpoint, filling all its space. It repays debt when cash is over 3 times the debt, and deposits half of any cash over $20,000. It fights when armed with health at least 50, else runs. It heals below 40 health, and buys the best affordable weapon from day 3 and space items when cash is over 4 times their price. Each move, 70% of the time it takes transit to a random neighborhood, else it flies to a random city it can afford.
- **The cautious bot** fills only half its space, repays debt first, never fights, and surrenders below 30 health.
- **The report** prints these keys, one per line, in this order:
  - `games`, `days`, `bot`
  - `score_p10`, `score_p50`, `score_p90`, `score_mean`, `dead_pct`
  - per game: `cop_chases`, `spikes`, `crashes`, `mugged`, `goons`, `jailed`
  - the target-comparable keys: `start_cash`, `start_debt`, `debt_daily_pct` (start city's lender), `bank_daily_pct`, `capacity`, `spikes_per_move`, `crashes_per_move`, `cops_per_move`, `mugs_per_move`
  - a `price <name> <min seen> <p50> <max seen>` line per good
- **`--targets`** appends `cmp <key> ours=<v> theirs=<v> <OK|LOW|HIGH|TOO_CLOSE>` for each key in both. Prices match by good name, and both ends are compared.
  - `must_differ` is 1 for `start_cash`, `start_debt`, `debt_daily_pct`, `bank_daily_pct`, `capacity` and price ends. It is 0 for the rates, which should feel the same.
- **`--strict`** exits 1 when any verdict isn't OK.
- **`compare_value`:** a ratio `ours / theirs` outside 0.6 to 1.67 gives LOW or HIGH. A ratio within 5% of 1 with `must_differ` gives TOO_CLOSE. Anything else is OK.

- [ ] **Step 1: Write the failing tests** in `tests/t_report.c`:
  ```c
  test_compare_band: (60, 100, 0) OK; (59, 100, 0) LOW; (167, 100, 0) OK; (168, 100, 0) HIGH
  test_compare_too_close: (104, 100, 1) TOO_CLOSE; (106, 100, 1) OK; (104, 100, 0) OK
  ```
- [ ] **Step 2: Run `make test T=report`.** Expected: link errors.
- [ ] **Step 3: Implement** `report.c`, `bots.c` and `main.c`.
- [ ] **Step 4: Run `make test T=report`.** Expected: 0 failed. Then run `build/host/ph-sim --games 200 --seed 1 --days 30 --bot greedy` twice. Expected: every key is printed, and both outputs are identical.
- [ ] **Step 5: Commit** `feat: ph-sim balance simulator`.

---

### Task 14: Balance pass, Seth's playtest, handoff

**Files:**
- Modify: `src/core/params.c`, `src/content/goods.c`, `src/content/cities.c`, `src/content/gear.c`, `tests/golden/basic.out`, `docs/HANDOFF.md`
- Create: `docs/balance.md`

- [ ] **Step 1: Run the sim against the targets:** `build/host/ph-sim --games 2000 --seed 1 --days 30 --bot greedy --targets ../pocket-hustle-research/targets.txt --strict`. Expected at first: some LOW, HIGH or TOO_CLOSE lines.
- [ ] **Step 2: Tune `params_default()` and the content numbers** until that command exits 0 and, with the same flags:
  - the greedy bot has `score_p50 > 0` and `dead_pct` between 3 and 25
  - the cautious bot (`--bot cautious`) has `dead_pct < 10`
  - `cop_chases` per game is between 1 and 6 for the greedy bot

  Rerun `make test` after each change. Unit tests use `test_params()`, so they must not break.
- [ ] **Step 3: Regenerate `tests/golden/basic.out`.** Read the diff and commit it.
- [ ] **Step 4: Write `docs/balance.md`:** the tuned values and our own sim results only. No Beermat numbers, and no `cmp` lines.
- [ ] **Step 5: Seth playtest checkpoint.** Send Seth how to play (`make text && build/host/ph-text`) and ask how it feels compared with Beermat's. His notes become follow-up tasks, which aren't patched in silently.
- [ ] **Step 6: Update `docs/HANDOFF.md`** (state, how to tune, what's next: Plan 2). Commit `balance: tune against observations; handoff`.

---

## Next plans (written after this one lands)

- **Plan 2, screens and sound on the PC:**
  - mockups of the main screens for Seth first
  - 320 x 240 renderer doubled to 640 x 480, a pixel font (ours or OFL)
  - each city's backdrop drawn in code
  - the button flow (D-pad, A, B, L1/R1 panels, the amount picker, SELECT chart, START menu)
  - mouse on the PC, sfxr-style sound effects, an SDL2 build, autosave and resume through `save_write`
- **Plan 3, the Miyoo build and release:**
  - the Mini platform layer adapted from `scorched-emiyoolator/src/platform_miyoo.c` (fb0, evdev, MI_AO; mind the 9 ms sleeps)
  - OnionOS packaging (`Roms/PORTS/Games/PocketHustle`, `Shortcuts/Strategy`) and a test on the card
  - a generic ARM SDL2 build
  - a private GitHub repo, made public on Seth's word, then the MiyooMini-Ports catalog PR
