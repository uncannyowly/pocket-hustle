# Pocket Hustle (working title)

An original buy-low, sell-high trading game for the Miyoo Mini and other ARM handhelds.
You start broke and in debt, with a fixed number of days to work eighteen cities: fourteen in
the USA (Oklahoma City, Houston, San Jose, New York City, Miami, New Orleans, Portland, Dallas,
San Francisco, Los Angeles, Seattle, Cincinnati, Dayton and Destin) and four across the border
in Mexico (Tijuana, Cozumel, Los Cabos and Sinaloa). Flights across the border cost more, and
your bank charges its out-of-network fee at every ATM in Mexico. Dodge the cops, keep the loan
sharks happy, think twice before backing a chemist's new research chemical or taking the shady
crew's deal, and finish with the biggest pile of cash.

The game plays on the PC (SDL2, with sound effects) and on the Miyoo Mini under OnionOS.

## Play on the PC

```sh
make game                    # needs SDL2 (pkg-config sdl2)
build/host/pocket-hustle
```

Options: `--data DIR` (saves, high scores and options; default `$XDG_DATA_HOME/pocket-hustle`,
else `~/.local/share/pocket-hustle`), `--seed N`, `--fullscreen`, `--mute`. For play-tests:
`--frames N --keys "TOKENS"` scripted buttons, `--param NAME=VALUE` (wild card and crew chances,
for a new game), `--pace MS` (real-time scripted presses) and `--record FILE` (raw RGB24 frames).

| Key | Button |
|---|---|
| Arrow keys | D-pad |
| Enter or Space | A (select, confirm) |
| Esc or Backspace | B (back) |
| `]` or Tab | R1 (next panel) |
| `[` or Shift+Tab | L1 (previous panel) |
| Y, X | Y (max), X (zero) in the amount picker |
| C | SELECT (price chart) |
| M | START (pause menu) |
| F11 or Alt+Enter | fullscreen |

On the new game screen the keyboard types the name instead (letters, digits, punctuation and
Space, up to 15 characters); Y, X, C, M and Space are letters there, Backspace deletes, and Enter
starts the game after you type.

The mouse works too: click anything that looks like a button or a row.

## Install on the Miyoo Mini (OnionOS)

```sh
scripts/package.sh                         # builds in Docker, writes dist/ and dist/pocket-hustle-<version>.zip
scripts/deploy-sd.sh /run/media/$USER/CARD # or extract the zip at the card root
```

Then Games > Ports > Strategy > Pocket Hustle (run "~Import ports" if it is not listed). Buttons:
D-pad, A confirm, B back, L1/R1 panels, Y max, X zero, SELECT chart, START menu; hold MENU for
2 seconds to save and leave. Saves are in `Roms/PORTS/Games/PocketHustle/data`. The install
touches only the game folder, the `.port` shortcut and the box art, and keeps `data/`.
`make miyoo` (inside the toolchain container) or `scripts/build-miyoo.sh` builds only the binary.

## Other tools

- `make test` runs the unit tests, the golden transcript and the copy check.
- `make text` builds the terminal version, `build/host/ph-text`.
- `make sim` builds the balance simulator, `build/host/ph-sim`.
- `make shots` renders a screenshot of every screen to `build/shots/` (needs python3 with PIL).
- `make smoke` runs the game headless with scripted keys and checks the saves.
- `make emu` plays every round in `tests/golden/rounds` on the Miyoo (ARM) build under qemu, in a
  throwaway Docker container (`scripts/emu-play.sh`); `make cross-arch` plays them on the PC and
  the ARM build and compares the saves and the drawn frames byte for byte.
- `scripts/watch-pc.sh ROUND` and `scripts/watch-emu.sh ROUND` (with `make fbview`) play a round
  in a window at watch pace; `scripts/record-rounds.sh` records them as video and key frames
  under `build/rounds/`. See `docs/HANDOFF.md`.

## Credits

The font is Press Start 2P, Copyright 2012 The Press Start 2P Project Authors, used under the
SIL Open Font License 1.1. The license text is in `assets/fonts/OFL.txt`. Everything else in
the game is original.
