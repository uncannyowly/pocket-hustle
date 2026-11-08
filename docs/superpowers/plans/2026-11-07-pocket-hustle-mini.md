# Pocket Hustle, Plan 3a: The Miyoo Mini Build Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Pocket Hustle running on Seth's Miyoo Mini Plus under OnionOS: a Mini backend for the platform layer, a cross-compiled binary, an OnionOS port package, and the package installed on his SD card.

**Architecture:** Plan 2 left one seam for this: `src/plat/plat.h`. A new `src/plat/plat_miyoo.c` implements it on the Mini's hardware, adapted from Seth's own Scorched port (`../scorched-emiyoolator/src/platform_miyoo.c`, MIT, proven on this device): `/dev/fb0` rotated 180 degrees, evdev buttons, SigmaStar MI_AO audio. `src/app/main.c` and everything above it stay as they are. The Docker toolchain builds `build/miyoo/pocket-hustle`; `scripts/package.sh` lays out an OnionOS port under `dist/`, and `scripts/deploy-sd.sh` copies it to a card. The rest of Plan 3 (a generic ARM SDL2 build, making the repo public, the MiyooMini-Ports catalog) comes later.

**Tech Stack:** C99, the `gta1-miyoo-toolchain` Docker image (`arm-linux-gnueabihf-gcc`, glibc 2.28), `-ldl -lpthread -lm`, POSIX shell for OnionOS scripts.

**Spec:** `docs/superpowers/specs/2026-11-06-pocket-hustle-design.md` (Goals 3, 4, 6; Constraints: device and toolchain).

## Global Constraints

- Everything in Plan 2's Global Constraints still holds (no em or en dashes anywhere, printable ASCII player text, commit identity from the repo config and a `Co-Authored-By:` line naming the model that wrote the commit, `make test` pristine, `scripts/arm-check.sh` passing).
- Device: Miyoo Mini Plus, OnionOS 4.4.0 beta, 640 x 480 panel mounted upside down (rotate 180 degrees). Other Mini models may have a larger panel (752 x 560): center the 640 x 480 picture on it.
- Only `src/plat/plat_miyoo.c` is new platform code; it implements `src/plat/plat.h` exactly (PAD_* ids from `src/ui/input.h`; `plat_text_input` does nothing; no clicks ever). `src/app/main.c` changes only if the Mini needs something the PC does not, and then behind the platform interface.
- Buttons from evdev (`/dev/input/event0`), not grabbed, so OnionOS keeps POWER and volume: D-pad = KEY_UP/DOWN/LEFT/RIGHT, A = KEY_SPACE, B = KEY_LEFTCTRL, X = KEY_LEFTSHIFT, Y = KEY_LEFTALT, L1 = KEY_E, R1 = KEY_T, L2 = KEY_TAB, R2 = KEY_BACKSPACE, SELECT = KEY_RIGHTCTRL, START = KEY_ENTER, MENU = KEY_ESC (Scorched's map). Holding MENU for 2 seconds quits (the game saves first, like closing the PC window), as in Scorched.
- Audio through MI_AO loaded at run time (dlopen), honoring OnionOS's volume and mute like Scorched; the launcher stops Onion's audioserver and restores it on exit. No audio is not an error: the game plays silent.
- The Mini's shortest sleep is about 9 ms: never rely on short sleeps for timing; the loop's 20 Hz ticks and redraw-on-input already fit.
- Saves, scores and options live in the game folder: `launch.sh` passes `--data "$GAMEDIR/data"`. A power-off loses at most the current day (autosave policy from Plan 2).
- OnionOS package (same shape as Scorched's): `Roms/PORTS/Games/PocketHustle/` (binary `pocket-hustle`, `launch.sh`, `README.txt`, `OFL.txt`, `VERSION`), `Roms/PORTS/Shortcuts/Strategy/Pocket Hustle.port`, `Roms/PORTS/Imgs/Pocket Hustle.png` (box art from the title screen, sized like Scorched's). `scripts/deploy-sd.sh CARD` touches nothing outside those three paths and never deletes the card's `data/` folder.
- VERSION becomes 0.1.0 (the first playable build).

## Review Focus

1. **Power-off and MENU-hold quit:** the game in progress is saved before exit, and the launcher restores Onion's audioserver and CPU settings on every exit path.
2. **The audio thread:** the MI_AO feeding thread calls the callback under the platform's audio lock and never blocks the main loop; closing joins it cleanly.
3. **The picture:** rotation, centering on a larger panel, double buffering without tearing or a stale page after exit (the console comes back usable).
4. **Input edges:** a press and release inside one poll still counts once; holding the D-pad repeats through input_step, not evdev auto-repeat events.
5. **The deploy script:** never touches other ports, never overwrites the player's saves.

---

### Task 1: Mini backend, Miyoo build and the OnionOS package

**Files:**
- Create: `src/plat/plat_miyoo.c`, `scripts/build-miyoo.sh`, `scripts/package.sh`, `scripts/deploy-sd.sh`, `dist/` layout produced by package.sh (gitignored), `pkg/launch.sh`, `pkg/Pocket Hustle.port`, `pkg/README.txt` (sources the package script copies)
- Modify: `Makefile` (target `miyoo`: build/miyoo/pocket-hustle from CORE + UI_SRC + src/app/main.c + src/plat/plat_miyoo.c with MIYOO_CFLAGS and `-lm -ldl -lpthread`; `make test` must not depend on it), `VERSION`, `.gitignore` (dist/), `README.md` (installing on the Mini), `docs/HANDOFF.md`

**Interfaces:**
- Consumes: `src/plat/plat.h` (implement every function exactly; `plat_init(fullscreen)` ignores its argument; `plat_poll` fills `held` and sets `quit` after MENU has been held 2 s; `nclicks` is always 0 and `text` always empty), `src/app/main.c` (unchanged unless strictly needed).
- Produces: `make miyoo` (inside the container) and `scripts/build-miyoo.sh` (runs it in Docker), `scripts/package.sh` (writes `dist/Roms/PORTS/...`), `scripts/deploy-sd.sh CARD_ROOT`.

- [ ] **Step 1:** Read `../scorched-emiyoolator/src/platform_miyoo.c`, `../scorched-emiyoolator/scripts/{build-miyoo.sh,package.sh,deploy-sd.sh}`, and the installed Scorched `launch.sh` and `.port` on Seth's card (read-only) for the proven patterns.
- [ ] **Step 2:** Write `src/plat/plat_miyoo.c` against `plat.h`: fb0 double buffer, rotate 180 and center, evdev with edge-safe held state and MENU-hold quit, MI_AO thread under a pthread mutex for plat_audio_lock/unlock, `plat_ms` from CLOCK_MONOTONIC, `plat_sleep_ms` via nanosleep, clean shutdown (restore the original fb pan and mode, close devices, join the audio thread).
- [ ] **Step 3:** `make miyoo` target and `scripts/build-miyoo.sh`. Expected: `file build/miyoo/pocket-hustle` reports a 32-bit ARM EABI5 executable; host `make test` and `scripts/arm-check.sh` unchanged and passing.
- [ ] **Step 4:** `pkg/` sources and `scripts/package.sh`: launch.sh (Scorched's structure: cd to the game dir, log to `pocket-hustle.log`, stop and restore audioserver, `LD_LIBRARY_PATH` as Scorched sets it, run `./pocket-hustle --data "$GAMEDIR/data"`, `sync` on exit), the `.port` shortcut (Scorched's format, GameDir PocketHustle, GameExecutable launch.sh, GameDataFile pocket-hustle, KillAudioserver=1), README.txt (Mini controls, saves location, font credit), OFL.txt, VERSION, box art PNG made from the title screen (`build/shots/01-title.png` or a dedicated render) at Scorched's image size.
- [ ] **Step 5:** `scripts/deploy-sd.sh CARD_ROOT` (checks the card looks like OnionOS, copies the three paths, keeps `data/`, `sync`).
- [ ] **Step 6:** Verify on the host what can be verified without the device: the binary links, `scripts/package.sh` produces the layout, and `scripts/deploy-sd.sh` into a temp directory shaped like a card installs only the three paths and keeps a pre-existing `data/` folder. Commit `feat: Miyoo Mini build and OnionOS package`.

### Task 2: Install on Seth's card (controller)

- [ ] Run `scripts/build-miyoo.sh && scripts/package.sh && scripts/deploy-sd.sh /run/media/shoots/E010-96EB`, list what was written, `sync`, and hand the card back for a device test.
