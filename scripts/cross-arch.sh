#!/bin/sh
# Plays every round (or the rounds named) twice with the same seed and keys: on the PC build
# and on the Miyoo (ARM) build under qemu (scripts/emu-play.sh). The two data directories (the
# save, the high scores) must be byte-identical, and so must the recorded canvas frames.
#   scripts/cross-arch.sh [ROUND...]
# Each build's final state must also equal the round's @expect line (--summary).
# A difference is a real finding: the first differing byte is printed (cmp -l offset, 1 based) for
# the save to be decoded against src/core/save.c. Exits 1 on any difference.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
. scripts/lib/round.sh
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
BIN=build/host/pocket-hustle

make -s game
scripts/build-miyoo.sh >/dev/null
rounds="$*"
[ -n "$rounds" ] || rounds="$(round_all)"
bad=0

for r in $rounds; do
	FILE="$(round_file "$r")"
	NAME="$(round_name "$FILE")"
	round_load "$FILE"
	D="build/cross/$NAME"
	rm -rf "$D"
	mkdir -p "$D"
	# shellcheck disable=SC2086
	"$BIN" --data "$D/pc" --seed "$ROUND_SEED" --mute $ROUND_PARAMS --frames "$ROUND_FRAMES" \
		--keys "$ROUND_KEYS" --summary "$D/pc.summary" --record - | md5sum | cut -d' ' -f1 > "$D/pc.frames"
	EMU_NO_BUILD=1 scripts/emu-play.sh -d "$D/arm" -f "$D/fb.raw" "$NAME" --record - | md5sum | cut -d' ' -f1 > "$D/arm.frames"
	status=same
	for side in pc arm; do
		round_check "$NAME" "$D/$side.summary" "the $side build" || status=DIFFERENT
	done
	for f in $( (ls "$D/pc"; ls "$D/arm") 2>/dev/null | sort -u); do
		if [ ! -f "$D/pc/$f" ] || [ ! -f "$D/arm/$f" ]; then
			echo "cross-arch: $NAME: $f exists on one side only"
			status=DIFFERENT
		elif ! cmp -s "$D/pc/$f" "$D/arm/$f"; then
			echo "cross-arch: $NAME: $f differs; first bytes (offset pc arm, octal):"
			cmp -l "$D/pc/$f" "$D/arm/$f" | head -5
			status=DIFFERENT
		fi
	done
	if ! cmp -s "$D/pc.frames" "$D/arm.frames"; then
		echo "cross-arch: $NAME: the recorded frames differ (pc $(cat "$D/pc.frames") arm $(cat "$D/arm.frames"))"
		status=DIFFERENT
	fi
	files="$(ls "$D/pc" | tr '\n' ' ')"
	echo "cross-arch: $NAME: saves and frames $status ($files)"
	[ "$status" = same ] || bad=1
done
[ "$bad" = 0 ] && echo "cross-arch: ok" || { echo "cross-arch: FAILED" >&2; exit 1; }
