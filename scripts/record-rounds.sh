#!/bin/sh
# Records rounds as video and key frames, on the PC build and on the Miyoo build under qemu.
#   scripts/record-rounds.sh [-p MS] [ROUND...]        (default: every round, 400 ms per press)
# For each round it writes, under build/rounds/:
#   <round>.mp4             the PC run, 640 x 480, 20 fps (a live-speed watch of the round)
#   <round>-arm.mp4         the same round on the ARM build under qemu
#   <round>-sheet.png       a contact sheet of the round's key frames (the @shot lines of its file)
#   <round>/<label>.png     each key frame at 2x
# and checks that the two runs recorded the identical frames. Needs ffmpeg and python3 with PIL
# on the host. Videos are not committed (build/ is ignored).
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
. scripts/lib/round.sh
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
PACE=400
if [ "${1:-}" = "-p" ]; then PACE="$2"; shift 2; fi
EVERY=$(( (PACE + 25) / 50 ))
[ "$EVERY" -ge 3 ] || EVERY=3
OUT=build/rounds
mkdir -p "$OUT/tmp" build/host

make -s game
scripts/build-miyoo.sh >/dev/null
rounds="$*"
[ -n "$rounds" ] || rounds="$(round_all)"

encode() {	# RAW MP4
	ffmpeg -hide_banner -loglevel error -y -f rawvideo -pix_fmt rgb24 -s 320x240 -framerate 20 -i "$1" \
		-vf scale=640:480:flags=neighbor -c:v libx264 -pix_fmt yuv420p -crf 18 "$2"
}

for r in $rounds; do
	FILE="$(round_file "$r")"
	NAME="$(round_name "$FILE")"
	round_load "$FILE"
	PCRAW="$OUT/tmp/$NAME-pc.raw"
	ARMRAW="$OUT/tmp/$NAME-arm.raw"
	# shellcheck disable=SC2086
	build/host/pocket-hustle --data "$OUT/tmp/$NAME-pc" --seed "$ROUND_SEED" --mute $ROUND_PARAMS \
		--frames "$ROUND_FRAMES" --keys "$ROUND_KEYS" --pace "$PACE" --nowait --record "$PCRAW"
	EMU_NO_BUILD=1 scripts/emu-play.sh -d "$OUT/tmp/$NAME-arm" -f "$OUT/tmp/$NAME-fb.raw" "$NAME" \
		--pace "$PACE" --nowait --record - > "$ARMRAW"
	if cmp -s "$PCRAW" "$ARMRAW"; then same="identical frames on both"; else same="FRAMES DIFFER"; fi
	encode "$PCRAW" "$OUT/$NAME.mp4"
	encode "$ARMRAW" "$OUT/$NAME-arm.mp4"
	python3 -I scripts/lib/sheet.py "$PCRAW" "$EVERY" "$ROUND_SHOTS" "$OUT" "$NAME"
	frames=$(($(wc -c < "$PCRAW") / 230400))
	echo "record-rounds: $NAME: $frames frames, $((frames / 20)) s, $same: $OUT/$NAME.mp4, $OUT/$NAME-arm.mp4, $OUT/$NAME-sheet.png"
	[ "$same" = "FRAMES DIFFER" ] && exit 1
	rm -f "$PCRAW" "$ARMRAW"
done
rm -rf "$OUT/tmp"
