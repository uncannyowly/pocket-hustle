# Pocket Hustle (working title)

An original buy-low, sell-high trading game for the Miyoo Mini and other ARM handhelds.
You start broke and in debt, with a fixed number of days to work seven cities: Oklahoma City,
Houston, San Jose, New York City, Miami, New Orleans and Portland. Dodge the cops, keep the
loan sharks happy and finish with the biggest pile of cash.

The game plays on the PC today (SDL2, with sound effects). The Miyoo build is next.

## Play on the PC

```sh
make game                    # needs SDL2 (pkg-config sdl2)
build/host/pocket-hustle
```

Options: `--data DIR` (saves, high scores and options; default `$XDG_DATA_HOME/pocket-hustle`,
else `~/.local/share/pocket-hustle`), `--seed N`, `--fullscreen`, `--mute`.

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

## Other tools

- `make test` runs the unit tests, the golden transcript and the copy check.
- `make text` builds the terminal version, `build/host/ph-text`.
- `make sim` builds the balance simulator, `build/host/ph-sim`.
- `make shots` renders a screenshot of every screen to `build/shots/` (needs python3 with PIL).
- `make smoke` runs the game headless with scripted keys and checks the saves.

## Credits

The font is Press Start 2P, Copyright 2012 The Press Start 2P Project Authors, used under the
SIL Open Font License 1.1. The license text is in `assets/fonts/OFL.txt`. Everything else in
the game is original.
