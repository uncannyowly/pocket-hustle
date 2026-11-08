# Handoff: Pocket Hustle

Last updated: 2026-11-07. Keep this file current; it is the notes the next session continues from.

## Goal

An original trading game for the Miyoo Mini Plus and other ARM handhelds that plays like
Beermat Software's Dope Wars 2.1 (2001) but is entirely ours: name, characters, jokes, art,
sounds and cities. Seth chose this over porting Beermat's exe (its license forbids
translating or reverse engineering it, it is wrapped in PC Guard, and its windows do not fit
480 pixels). Design: `docs/superpowers/specs/2026-11-06-pocket-hustle-design.md` (approved).
Plans: Plan 1 (`docs/superpowers/plans/2026-11-06-pocket-hustle-core.md`, done: the rules, a
terminal version, a balance simulator), Plan 2 (screens and sound on the PC, done), Plan 3
(the Miyoo build and release, next).

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
make text              # build/host/ph-text, the terminal version
make sim               # build/host/ph-sim, the balance simulator
make game              # build/host/pocket-hustle, the SDL2 game (needs SDL2)
make smoke             # headless scripted runs of the game binary, checks the saves
make miyoo             # (in the toolchain container) build/miyoo/pocket-hustle; scripts/build-miyoo.sh runs it in Docker
scripts/package.sh     # dist/Roms/PORTS/... and dist/pocket-hustle-<version>.zip (OnionOS package)
scripts/deploy-sd.sh CARD_ROOT   # installs dist/ on a card, keeps data/
make shots             # build/shots/NN-name.png, every screen rendered by the real UI code
scripts/arm-check.sh   # compile and link the tests for the Mini inside Docker
```

New source files are picked up by the Makefile's wildcards; new test modules need a line in
`tests/tests.h` and in `MODULES[]` in `tests/run_tests.c`.

## State

Plan 2 (screens and sound on the PC) is done as of 2026-11-07. Play it with
`make game && build/host/pocket-hustle` (keys in the README, `--mute`, `--data DIR`,
`--seed N`, `--fullscreen`).

- `src/gfx`: 320 x 240 integer canvas, Press Start 2P font, palette, icons, sprites, skylines.
- `src/sfx`: synthesized sound effects (parameter presets) and a six-voice mixer.
- `src/ui`: the screen state machine (`ui.h`), the App with saves, scores and options
  (`app.h`), and input edge detection with D-pad auto-repeat (`input.h`). No SDL here.
- `src/plat/plat.h` and `plat_sdl.c`: the platform interface (framebuffer, events, audio
  with a lock) and its SDL2 backend. `src/app/main.c`: the game loop. Only these two
  directories link SDL; `make test` and `scripts/arm-check.sh` do not need it.
- The loop: poll, `input_step`, `ui_press` per fired button, `ui_click` (redrawing first if a
  press came in the same frame), `ui_tick` per 50 ms, redraw on input or tick, scale 2x, present.
  Closing the window, the title's QUIT, or `--frames` running out saves the game in progress.
- Scripted mode for the smoke test: `--frames N --keys "TOKENS"` (U D L R A B X Y L1 R1 SEL
  START, one press every 3 frames, one tick per frame).
- Screenshots: `make shots` (tools/shots/main.c, deterministic test games). The mockup
  (`docs/mockups/screens-round1.html`) is the layout reference; the shots match it except the
  items listed in the Task 13 report (title QUIT row, chart footer wording, game over stats).

Seth has not played the PC build yet; his notes become follow-up tasks.

Known issues after the Plan 2 reviews (all minor, none blocks play):

- Typing a name on the PC: after a typed character the focus sits on START HUSTLING, so the
  first Backspace moves focus back to the keyboard and only the second one deletes.
- Text input switches on one frame after the new-game screen opens, so a key pressed in that
  frame still acts as a button (the replace prompt still guards a saved game).
- This PC's SDL2 is sdl2-compat over SDL3, which crashes on a synthetic SDL_TEXTINPUT, so the
  smoke check skips its typed-text assertions here; real typing is untested until Seth plays.
- The chase log shows the first 4 of up to 8 lines, so the newest damage line can drop when
  a big posse shoots.
- SFX_NEWS is rendered but never played.
- Pickers that cannot move (REPAY with no cash, BORROW at the limit, DOC under the price of a
  point, DEPOSIT or WITHDRAW with nothing to move) open with no explanation.
- Autosave failures show only at SAVE AND QUIT or exit, and a failed score write is not
  retried at exit when no game is in progress.
- The game over ATM fees cell overlaps its label for fees from $1B to $10B.
- The chart's today label can land on the y-axis labels near day 1.
- Letter keys are bound by key position, so on QWERTZ "Y = max" is the key labeled Z.
- Tests: temp dirs pile up under build/; test_jail_morning_news should assert
  unconditionally; the uppercase-copy loop is repeated in nine places and city_of in three;
  scripts/smoke.sh builds its checker without warning flags.
- Mockup wording or behavior Seth may want changed: "LAST PAID 0 DAYS AGO" on a loan never
  paid (it also resets after a goon visit); crossing the keyboard's bottom row takes several
  presses; START acts as A on the title, QUIT included; the bank and lender highlight resets
  on each visit.
- For Plan 3: SDL_RenderWindowToLogical needs SDL 2.0.18 or newer on a generic ARM build;
  ship OFL.txt with binary packages; a power-off between autosaves loses the day's trades
  and rolls a chase back to its start.

Plan 1 (the rules, ph-text, ph-sim) is done as of 2026-11-06:

- `src/core`, `src/content`: the whole rule set (market, travel, lenders, bank, goons, cops,
  jail, muggings, hospital, store, rumors), game text and jokes, saves and high scores.
- `src/text`: ph-text, the terminal version (`make text && build/host/ph-text`).
- `tools/sim`: ph-sim, the balance simulator. Tuned values and results: `docs/balance.md`.
- Golden transcript: `tests/golden` (regenerate after deliberate rules or text changes; see
  `tests/golden/run.sh`).

Settled 2026-11-07: the bank charges an
out-of-network fee (3%, at least $5) except at the two branch neighborhoods in each city, and
the round 1 screen mockups (docs/mockups/screens-round1.html) are approved: keep Press Start 2P
and the look, with tweaks later. The balance targets
are provisional (20 observed days); see docs/balance.md.

How to tune: change `src/core/params.c` or the content ranges, then run ph-sim with
`--targets ../pocket-hustle-research/targets.txt --strict` (observations of Beermat's
game, outside the repo; screen-only, see that folder's notes.md). The check flags our
numbers that land within 5% of theirs (TOO_CLOSE) as well as ones too far off.

ARM: `scripts/arm-check.sh` compiles and links for the Mini. Running ARM binaries under
QEMU (`docker run --platform linux/arm/v7 ...`) needs binfmt registered on the host
(`docker run --privileged --rm tonistiigi/binfmt --install arm`); ask Seth first.

## Mini build (Plan 3a task 1, 2026-11-08, untested on the device)

`src/plat/plat_miyoo.c` (fb0 double buffer via `src/gfx/panel.c`, evdev, MENU hold via
`src/ui/hold.c`, MI_AO by dlopen, no SDL 1.2 fallback: no MI_AO means silent). Test hooks:
`PH_FAKEFB=WxH:file`, `PH_INPUT=dev`. Package sources are in `pkg/`; version 0.1.0.
Only the device can show: the picture orientation, buttons, MENU hold, audio and volume.

## Next

Plan 3: the Miyoo build and release. Add `src/plat/plat_miyoo.c` beside `plat_sdl.c`, the
same interface (`plat.h`), adapted from `../scorched-emiyoolator/src/platform_miyoo.c`: fb0
with the 180 degree rotation, evdev buttons, MI_AO audio, frame pacing that allows for the
Mini's 9 ms minimum sleep, MENU returns to OnionOS. Then OnionOS packaging, a test on Seth's
card, a generic ARM SDL2 build, the GitHub repo going public on Seth's word and the
MiYoo Mini Ports catalog entry. `src/app/main.c` is platform-neutral apart from `plat.h`.
