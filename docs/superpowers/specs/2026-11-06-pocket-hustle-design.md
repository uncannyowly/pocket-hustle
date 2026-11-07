# Pocket Hustle (working title): Design

**Date:** 2026-11-06
**Status:** Approved by Seth 2026-11-06
**Repo (planned):** `/mnt/data/git/MiYoo-Mini/pocket-hustle`
**Replaces:** `2026-11-06-dopewars-emiyoolator-design.md` (shelved)

---

## Intent

Seth wants an original trading game for the Miyoo Mini and other ARM handhelds that plays
like Beermat Software's **Dope Wars 2.1 for Windows** (2001), but is entirely ours: "own
art, names, jokes and the likes. The same thing with cities."

**Why fresh instead of a port:** porting Beermat's exe ran into two walls. Its license
forbids translating, decompiling and reverse engineering it, and it is wrapped in PC Guard
copy protection. Its desktop windows are also taller than the Mini's 480-pixel screen.
A new game designed for a handheld fixes both, and because everything in it is ours, it can
be public.

**Success:** a quick, tense, funny game that someone who loved Beermat's version picks up and
says "yes, this is that game" about, yet nothing in it (name, characters, text, art,
sounds) is Beermat's.

## What "plays like Beermat's" means

The genre loop: you start with a little cash and a big debt to a loan shark, and a fixed
number of days. Every day you can buy and sell goods at the local market, then move. Prices
differ by place and swing wildly, so the money is in buying low and moving somewhere that
pays high. When the days run out, your score is your net worth.

The pieces Beermat's version has (from its readme and the windows it shows in play), which
ours has too, in our own form:

- **Market:** buy and sell goods at local prices, with a limited amount you can carry.
- **Market events:** crashes (a flood of cheap supply) and spikes (a bust drives the price
  up), announced with a joke.
- **Travel:** neighborhoods within a city by local transit, and other cities by plane. Each
  move costs a day.
- **Loan shark:** borrow and repay, with debt that grows daily. Lenders get violent if you
  don't pay.
- **Bank:** savings that are safe from muggers.
- **Cops:** a chase where you run or fight back, with a cop who brings more help each time.
- **Store:** weapons to fight with and more carrying space.
- **Health:** you get hurt in fights. The hospital heals you for a price.
- **Jail:** a night in jail if you're caught.
- **Random trouble:** muggings and dropped stash.
- **A rumor character:** someone who tells you tips (some true) and jokes.
- **Price history chart** for each good.
- **Game length choice**, stats, high scores, and an end-of-game summary.

## Where the inspiration comes from (Seth chose both clean sources)

1. **Public reference.** Ben Webb's open-source dopewars (GPL) and its documentation, plus the
   well-known rules of the 1984 DOS game Drug Wars. Study them for which systems exist and the
   rough shape of the numbers. Then write our own code and numbers. Never copy code, text or
   names from it.
2. **Watching Beermat's game play.** Take the game files out of the installer without running
   it (`dw21.exe` also installs Gator's `Trickler.exe` adware). Run `DopeWars.exe` in a
   throwaway Wine prefix (never `/mnt/scratch/Games_Wine`), play several games, and log only
   what's on screen: prices per good, place and day, how often events, cops and muggings
   happen, and how fast debt and savings grow. **Observation through the screen only:** no
   debugger, memory dumps, API tracing, decrypting or disassembling. If PC Guard refuses to
   run under Wine, fall back to sessions Seth plays himself.

Our numbers are then tuned so a game *feels* the same: the size of the swings, how often you
get busted, how fast the debt bites, the typical final score. They aren't copied. The logs stay
local research notes, outside the repo.

## Goals

1. The full loop above, finished and balanced. A game takes about 15 to 30 minutes, and you
   want another one right after.
2. Seth's seven cities: **Oklahoma City ("Loud City")**, Houston, San Jose, New York City, Miami,
   New Orleans and Portland.
3. Designed for handheld buttons and a 640 x 480 screen first. Every screen fits with no
   scrolling or pointer needed.
4. Runs on the Miyoo Mini Plus under OnionOS, and on other ARM Linux handhelds. A PC build is
   used for development.
5. Everything original: title, characters, text and jokes, art, sounds. Any outside asset
   (a font, say) must be under a license that allows redistribution (CC0, OFL) and is credited.
6. Safe to put to sleep or power off: it autosaves and resumes where you left off.
7. Public on GitHub, and in the MiyooMini-Ports catalog, when Seth gives the go-ahead.

## Non-goals (for now)

- Online score tables and multiplayer (Ben Webb's dopewars has network play; this doesn't).
- A city editor (Beermat had one).
- Music. Sound effects only in v1.
- A mouse pointer on the Mini. See "My calls".

## Anti-goals (failure even if it works)

- Anything recognizably Beermat's: the "Dope Wars" name, its cop, its old lady, its jokes, its
  weapon list, its skins and layout, its sounds, its district lists. Real places are fine,
  but pick our own set (Beermat's NYC list is the classic Bronx, Queens, Central Park,
  Manhattan, Coney Island, Brooklyn, so ours should differ).
- Numbers copied rather than tuned.
- Code copied from GPL dopewars.
- A desktop app shrunk onto a handheld.
- Punching down: no neighborhood presented as the poor or dangerous one, and no real people
  named or mocked.
- Sluggish on the Mini: input lag, long loads or stutters.

## Constraints

- **Device:** Miyoo Mini Plus, OnionOS 4.4.0 beta, 640 x 480, Cortex-A7, about 128 MB RAM. SD
  card at `/run/media/shoots/E010-96EB`. From Scorched: a short `usleep` really takes about
  9 ms on the Mini, so frame pacing must allow for that. Screen is `/dev/fb0`, buttons are
  evdev, sound is MI_AO. All three already work in `scorched-emiyoolator/src/platform_miyoo.c`.
- **Toolchain:** the `gta1-miyoo-toolchain` image (glibc 2.28) the other ports use.
- **No Beermat files** (installer, exe, sounds, text, screenshots, observation logs) in the repo.
- **No em dashes** (or en dashes as punctuation) anywhere: game text, README, docs, commits.

## Decided by Seth

- An original game from scratch that plays like Beermat's, with its own art, names, jokes
  and cities.
- The seven cities above.
- Inspiration from both clean sources, changed enough to be ours.
- Handheld-first, written in C reusing Scorched's Mini plumbing, public on GitHub and the
  catalog (approved as played back: "looks good to go").
- From the shelved port discussion: a Tab-style button to jump between fields quickly, and he
  wanted the mouse kept as well. How that carries over is under "My calls".

## My calls (Claude's; Seth can overrule any of them)

- **Title:** "Pocket Hustle" as a working title, and the repo slug follows it. Seth can rename
  it any time before release.
- **The goods:** about ten real street drugs with a 2026 mix, following the genre's convention,
  with our own descriptions and jokes. (The other option would be parody goods.)
- **Map:** each city has about six neighborhoods, picked from real, well-known areas. Getting
  around a city uses its own transit (OKC Streetcar, METRORail, VTA Light Rail, the Subway,
  Metrorail, the St. Charles streetcar, MAX). Flying between cities uses its real airport and
  costs a ticket. Each city also has its own price tendencies, so cities feel different.
- **Game length:** 30 days by default, with 15, 60 and 90 day options.
- **Characters:** all new, named by the builder and reviewed by Seth. That means a cop with a
  growing posse, a lender in each city with its own terms and threats, and a rumor-monger
  with tips and jokes.
- **Store:** our own comedic arsenal, with some items local to each city.
- **Look:** pixel art at 320 x 240, doubled to 640 x 480 (Scorched's approach, readable on the
  3.5" screen). A bold pixel font. Each city gets its own drawn skyline or landmark backdrop.
  Art is drawn by Claude in code. No logos or trademarks: "Loud City" appears as text only,
  with no team marks.
- **Sound:** original chiptune-style effects from an sfxr-style synthesizer.
- **Controls:**

  | Button | Does |
  |---|---|
  | D-pad | Move the highlight |
  | A | Select or confirm |
  | B | Back |
  | L1 / R1 | Jump between panels (market, your stash, actions): the Tab idea |
  | Amount picker | Up/down by 1, left/right by 10, Y = max, X = zero |
  | SELECT | Price chart for the highlighted good |
  | START | Game menu (save and quit, options, high scores) |

  Every screen is built for buttons, so the Mini gets **no mouse pointer**. On the PC build,
  the mouse works (click anything) along with the keyboard. This is the one place I'm
  reading Seth's "keep the mouse" differently for a game designed from scratch.
- **Saves:** autosave at the start of each day, resume on launch. High scores and options
  persist too, all synced to the card on write.
- **Code shape:** C99. The game rules are a deterministic, seedable core with no input or
  output, covered by unit tests. Separate layers handle the UI and the platform: Miyoo, SDL2
  (PC and other handhelds), and headless (tests and balance runs).
- **Balance method:** headless bots play thousands of games, and their price ranges, event
  rates and final scores are compared against the Beermat observation logs.
- **Other ARM devices:** a generic armhf/aarch64 Linux SDL2 build. Only the Mini is tested
  on hardware.
- **Text and jokes:** written by Claude, reviewed by Seth, with his own additions welcome.
  Local flavor for each city.
- **License:** MIT, like Scorched.
- **Repo:** private until the first playable build, then Seth flips it public.
- **Before building the UI:** mockups of the main screens for Seth to react to.

## Left to the builder

- Exact numbers, formulas and the event list (inside the "feels the same" target).
- Character names, neighborhood picks and joke text, all reviewed with Seth.
- How to observe Beermat's game: driving it with xdotool and screenshots, or logging by hand.
- Code layout, asset pipeline, test structure, OnionOS packaging (`launch.sh`, likely
  `Roms/PORTS/Games/PocketHustle` with a `Shortcuts/Strategy` entry).
