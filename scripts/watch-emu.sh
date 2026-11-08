#!/bin/bash
# Watch a round play itself on the emulated Miyoo: the ARM build runs under qemu in a container
# and draws into its stand-in framebuffer file, and the host viewer (tools/fbview) shows that
# file live, 640 x 480, rotation undone.
#   scripts/watch-emu.sh ROUND [MS_PER_PRESS]       (default 400)
# Closing the window stops the emulated game; the game ending closes the window. The run is saved
# under build/watch-emu/<round>.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
. scripts/lib/round.sh
[ $# -ge 1 ] || { echo "usage: watch-emu.sh ROUND [MS_PER_PRESS]; rounds: $(round_all | tr '\n' ' ')" >&2; exit 2; }
FILE="$(round_file "$1")"
NAME="$(round_name "$FILE")"
PACE="${2:-400}"
FB="build/watch-emu/$NAME-fb.raw"
: "${DISPLAY:=:0}"
export DISPLAY

make -s fbview
mkdir -p build/watch-emu
rm -f "$FB"
EMU_CONTAINER="ph-watch-$$" scripts/emu-play.sh -d "build/watch-emu/$NAME" -f "$FB" "$NAME" --pace "$PACE" &
EMU=$!
build/host/fbview "$FB" 640x480 --pid "$EMU" --title "pocket-hustle on an emulated Miyoo: $NAME" &
VIEW=$!
# either side ending ends both
while kill -0 "$EMU" 2>/dev/null && kill -0 "$VIEW" 2>/dev/null; do
	sleep 0.3
done
if kill -0 "$EMU" 2>/dev/null; then	# the window was closed: stop the container
	docker stop -t 1 "ph-watch-$$" >/dev/null 2>&1 || true
fi
kill "$VIEW" 2>/dev/null || true
wait "$EMU" 2>/dev/null || true
wait "$VIEW" 2>/dev/null || true
