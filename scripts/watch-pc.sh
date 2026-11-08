#!/bin/sh
# Watch a round play itself in a visible window on the desktop (the PC build, real SDL video).
#   scripts/watch-pc.sh ROUND [MS_PER_PRESS]       (default 400)
# The round's buttons are pressed by the game's scripted mode, one every MS_PER_PRESS ms, in real
# time. Sound is off unless WATCH_SOUND=1. Close the window (or press Esc) to stop; the run
# is saved under build/watch/<round>. List the rounds: ls tests/golden/rounds.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
. scripts/lib/round.sh
[ $# -ge 1 ] || { echo "usage: watch-pc.sh ROUND [MS_PER_PRESS]; rounds: $(round_all | tr '\n' ' ')" >&2; exit 2; }
FILE="$(round_file "$1")"
NAME="$(round_name "$FILE")"
PACE="${2:-400}"
round_load "$FILE"
make -s game
[ "${WATCH_SOUND:-0}" = 1 ] && MUTE="" || MUTE="--mute"
: "${DISPLAY:=:0}"
export DISPLAY
rm -rf "build/watch/$NAME"
# $ROUND_PARAMS and $MUTE are word lists
# shellcheck disable=SC2086
exec build/host/pocket-hustle --data "build/watch/$NAME" --seed "$ROUND_SEED" $MUTE $ROUND_PARAMS \
	--frames "$ROUND_FRAMES" --keys "$ROUND_KEYS" --pace "$PACE"
