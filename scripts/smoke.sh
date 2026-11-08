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

# The play-test options: --param, --record, --pace. A bad --param is a usage error (exit 2).
for bad in "nope=1" "start_cash=1" "rc_max_stakes=4" "rc_offer_permille" "rc_offer_permille=x" "=5" \
	"rc_max_stakes=4294967297"; do
	rc=0
	"$BIN" --data build/smoke4 --mute --frames 3 --param "$bad" 2>build/smoke-err.txt || rc=$?
	if [ "$rc" != 2 ] || [ ! -s build/smoke-err.txt ]; then
		echo "smoke: --param $bad should exit 2 with a message (exit $rc)" >&2
		exit 1
	fi
done
echo "smoke: bad --param is a usage error: ok"
rm -rf build/smoke4
"$BIN" --data build/smoke4 --seed 1 --mute --frames 80 --param rc_offer_permille=1000 --param cohort_od_permille=0 \
	--keys "$NEW $MOVE"
"$CHECK" build/smoke4/save.dat 2 15 1000
"$BIN" --data build/smoke4 --mute --frames 3 --keys "D" >/dev/null	# a resumed game keeps its own params
"$CHECK" build/smoke4/save.dat 2 15 1000

# --record: one 320 x 240 RGB24 frame per tick (39 ticks in 40 frames), to a file or stdout.
FRAME=$((320 * 240 * 3))
rm -rf build/smoke5
"$BIN" --data build/smoke5 --seed 1 --mute --frames 40 --keys "$NEW" --record build/smoke5.raw
[ "$(wc -c < build/smoke5.raw)" = "$((39 * FRAME))" ] || { echo "smoke: --record FILE size is wrong" >&2; exit 1; }
rm -rf build/smoke5
"$BIN" --data build/smoke5 --seed 1 --mute --frames 40 --keys "$NEW" --record - >build/smoke5b.raw 2>build/smoke-err.txt
cmp build/smoke5.raw build/smoke5b.raw || { echo "smoke: --record - differs from --record FILE" >&2; exit 1; }
[ ! -s build/smoke-err.txt ] || { echo "smoke: --record - logged on stderr" >&2; exit 1; }
rm -f build/smoke5.raw build/smoke5b.raw build/smoke-err.txt
echo "smoke: --record: ok"

# --pace 600 puts a press every 12 frames and --nowait skips the sleeping: the same game as
# smoke2 (same seed, same keys), to the byte. --pace 150 plays in real time.
rm -rf build/smoke6
"$BIN" --data build/smoke6 --seed 1 --mute --frames 80 --keys "$NEW $MOVE" --pace 600 --nowait
cmp build/smoke2/save.dat build/smoke6/save.dat || { echo "smoke: --pace changed the game" >&2; exit 1; }
rm -rf build/smoke6
t0=$(date +%s%N)
"$BIN" --data build/smoke6 --seed 1 --mute --frames 30 --keys "D A" --pace 150
t1=$(date +%s%N)
[ $(((t1 - t0) / 1000000)) -ge 1000 ] || { echo "smoke: --pace 150 did not run in real time" >&2; exit 1; }
for bad in "--pace 0 --frames 3" "--pace 100" "--nowait --frames 3"; do
	rc=0
	"$BIN" --data build/smoke6 --mute $bad 2>/dev/null || rc=$?
	[ "$rc" = 2 ] || { echo "smoke: $bad should exit 2 (exit $rc)" >&2; exit 1; }
done
echo "smoke: --pace: ok"

# --summary: the final state on one line (day, city, cash, bank, all debt, over), stdout untouched.
rm -rf build/smoke7 build/smoke7.txt
"$BIN" --data build/smoke7 --seed 1 --mute --frames 80 --keys "$NEW $MOVE" --summary build/smoke7.txt >build/smoke7.out
[ "$(cat build/smoke7.txt)" = "day=2 city=0 cash=2600 bank=0 debt=4599 over=0" ] ||
	{ echo "smoke: --summary wrong: $(cat build/smoke7.txt)" >&2; exit 1; }
[ ! -s build/smoke7.out ] || { echo "smoke: --summary wrote to stdout" >&2; exit 1; }
rm -f build/smoke7.out build/smoke7.txt
echo "smoke: --summary: ok"

# plat_poll returns several clicks queued in one poll, in order.
cc -std=c99 -Wall -Wextra -Werror -Isrc -D_POSIX_C_SOURCE=200809L $(pkg-config --cflags sdl2) \
	-o build/host/smoke-clicks tools/smoke/clicks.c src/plat/plat_sdl.c $(pkg-config --libs sdl2)
build/host/smoke-clicks
echo "smoke: ok"
