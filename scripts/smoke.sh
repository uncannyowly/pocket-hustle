#!/bin/sh
# Headless runs of the game binary with scripted keys (SDL dummy video and audio drivers).
# Run 1: new 15 day game, buy, move, open the menu, SAVE AND QUIT, then QUIT from the title.
# Run 2: new 15 day game, move once, then run out of frames (treated like closing the window).
# Both must exit 0 and leave a save.dat that save_read loads, on day 2 of 15.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
BIN=build/host/pocket-hustle
CHECK=build/host/smoke-check

mkdir -p build/host
cc -std=c99 -Isrc -D_POSIX_C_SOURCE=200809L -o "$CHECK" tools/smoke/check.c src/core/*.c src/content/*.c -lm

NEW="D A L1 L START A"		# title: NEW GAME, the 15 day chip, START, the opening card
MOVE="R1 R1 A D A"		# actions panel, MOVE, the second destination

rm -rf build/smoke build/smoke2
"$BIN" --data build/smoke --seed 1 --mute --frames 400 \
	--keys "$NEW A Y A $MOVE A A A START U U A U A"
"$CHECK" build/smoke/save.dat 2 15

"$BIN" --data build/smoke2 --seed 1 --mute --frames 80 --keys "$NEW $MOVE"
"$CHECK" build/smoke2/save.dat 2 15
# Run 3: an exit save that fails (save.dat is a directory) must make the exit status non-zero.
rm -rf build/smoke3
mkdir -p build/smoke3/save.dat
if "$BIN" --data build/smoke3 --seed 1 --mute --frames 80 --keys "$NEW $MOVE" 2>/dev/null; then
	echo "smoke: a failed exit save still exited 0" >&2
	exit 1
fi
echo "smoke: failed exit save exits non-zero: ok"

# plat_poll returns several clicks queued in one poll, in order.
cc -std=c99 -Wall -Wextra -Werror -Isrc -D_POSIX_C_SOURCE=200809L $(pkg-config --cflags sdl2) \
	-o build/host/smoke-clicks tools/smoke/clicks.c src/plat/plat_sdl.c $(pkg-config --libs sdl2)
build/host/smoke-clicks
echo "smoke: ok"
