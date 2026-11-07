# Handoff: Pocket Hustle

Last updated: 2026-11-06. Keep this file current; it is the notes the next session continues from.

## Goal

An original trading game for the Miyoo Mini Plus and other ARM handhelds that plays like
Beermat Software's Dope Wars 2.1 (2001) but is entirely ours: name, characters, jokes, art,
sounds and cities. Seth chose this over porting Beermat's exe (its license forbids
translating or reverse engineering it, it is wrapped in PC Guard, and its windows do not fit
480 pixels). Design: `docs/superpowers/specs/2026-11-06-pocket-hustle-design.md` (approved).
Current plan: `docs/superpowers/plans/2026-11-06-pocket-hustle-core.md` (Plan 1 of 3: the
rules, a terminal version, a balance simulator).

## Where things are

| What | Where |
|---|---|
| This repo | `/mnt/data/git/MiYoo-Mini/pocket-hustle` (local git; GitHub comes in Plan 3) |
| Research notes, Beermat observation logs, targets | `/mnt/data/git/MiYoo-Mini/pocket-hustle-research/` (never in the repo) |
| Beermat installer (Seth's copy) | `/mnt/scratch/Games_Wine/drive_c/Games/Dope Wars 2.1/dw21.exe` (never run it: it carries Gator adware) |
| Mini plumbing to reuse in Plan 3 | `../scorched-emiyoolator/src/platform_miyoo.c` |
| ARM toolchain | Docker image `gta1-miyoo-toolchain` (glibc 2.28) |

## Rules

- Nothing recognizably Beermat's in the game (the content tests enforce a denylist).
- Beermat's game is observed through the screen only: no debugger, memory dumps, API
  tracing, decrypting or disassembling.
- GPL dopewars is a reference for ideas only; no code, text or names from it.
- No em dashes or en dashes anywhere (`scripts/check-copy.sh`, part of `make test`).
- Core math is integer only; money is int64 dollars clamped to 10^15. Same seed and same
  commands give identical results on the PC and on ARM.
- Commits use `Seth Hoots <53498983+uncannyowly@users.noreply.github.com>` (repo-local
  config) and end with the Claude co-author line.

## Build and test

```sh
make test              # every unit test module, the golden transcript, the copy check
make test T=rng        # one module
make text              # build/host/ph-text, the terminal version (from Task 11)
make sim               # build/host/ph-sim, the balance simulator (from Task 13)
scripts/arm-check.sh   # compile and link the tests for the Mini inside Docker
```

New source files are picked up by the Makefile's wildcards; new test modules need a line in
`tests/tests.h` and in `MODULES[]` in `tests/run_tests.c`.

## State

Plan 1 (the rules, ph-text, ph-sim) is done as of 2026-11-06:

- `src/core`, `src/content`: the whole rule set (market, travel, lenders, bank, goons, cops,
  jail, muggings, hospital, store, rumors), game text and jokes, saves and high scores.
- `src/text`: ph-text, the terminal version (`make text && build/host/ph-text`).
- `tools/sim`: ph-sim, the balance simulator. Tuned values and results: `docs/balance.md`.
- Golden transcript: `tests/golden` (regenerate after deliberate rules or text changes; see
  `tests/golden/run.sh`).

Waiting on Seth: review of the content names and joke tone (sent during Plan 1), and a
playtest of ph-text. His notes become follow-up tasks.

How to tune: change `src/core/params.c` or the content ranges, then run ph-sim with
`--targets ../pocket-hustle-research/targets.txt --strict` (observations of Beermat's
game, outside the repo; screen-only, see that folder's notes.md). The check flags our
numbers that land within 5% of theirs (TOO_CLOSE) as well as ones too far off.

ARM: `scripts/arm-check.sh` compiles and links for the Mini. Running ARM binaries under
QEMU (`docker run --platform linux/arm/v7 ...`) needs binfmt registered on the host
(`docker run --privileged --rm tonistiigi/binfmt --install arm`); ask Seth first.

## Next

Plan 2: screens and sound on the PC (mockups for Seth first). Plan 3: the Miyoo build,
packaging and release. Both are outlined at the end of the Plan 1 document.
