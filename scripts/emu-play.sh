#!/bin/sh
# Plays a round on the Miyoo (ARM) build under qemu-arm in a throwaway Docker container.
#   scripts/emu-play.sh [-d DATA_DIR] [-f FB_FILE] ROUND [GAME ARGS...]
#   scripts/emu-play.sh --all
# ROUND is a name from tests/golden/rounds (or a .keys path). GAME ARGS are passed to the game
# (for example --pace 400 --nowait --record -). The game draws into a stand-in framebuffer file
# (PH_FAKEFB, FB_FILE, default build/emu/<round>-fb.raw; the picture is rotated 180 degrees as on the
# panel) and writes its save and high scores to DATA_DIR (default build/emu/<round>).
# The image pocket-hustle-emu (scripts/emu/Dockerfile) is built on first use. The Miyoo binary is
# rebuilt first unless EMU_NO_BUILD=1. EMU_CONTAINER=NAME names the container (so it can be
# stopped). Nothing is installed on the host and no container outlives its run. The game's
# stdout (--record -) passes through; this script's own lines go to stderr. A round that ends
# before the game does leaves save.dat; it is loaded and printed. The game's final state
# (--summary, written to DATA_DIR.summary) must equal the round's @expect line, else exit 1.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
. scripts/lib/round.sh
IMAGE=pocket-hustle-emu
SYSROOT=/opt/miyoomini-toolchain/arm-linux-gnueabihf/libc

if [ "${1:-}" = "--all" ]; then
	for r in $(round_all); do
		scripts/emu-play.sh "$r"
	done
	exit 0
fi

DATA="" FB=""
while [ $# -gt 0 ]; do
	case "$1" in
	-d) DATA="$2"; shift 2 ;;
	-f) FB="$2"; shift 2 ;;
	*) break ;;
	esac
done
[ $# -ge 1 ] || { echo "usage: emu-play.sh [-d DATA_DIR] [-f FB_FILE] ROUND [GAME ARGS...] | --all" >&2; exit 2; }
FILE="$(round_file "$1")"
NAME="$(round_name "$FILE")"
shift
round_load "$FILE"
: "${DATA:=build/emu/$NAME}"
: "${FB:=build/emu/$NAME-fb.raw}"
mkdir -p build/emu
rm -rf "$DATA"
# the container sees the repo as /src: paths under the repo are the same relative to it
case "$DATA$FB" in /*) echo "emu-play: DATA_DIR and FB_FILE are paths relative to the repo" >&2; exit 2 ;; esac

if ! docker image inspect "$IMAGE" >/dev/null 2>&1; then
	echo "emu-play: building the $IMAGE image (once)" >&2
	docker build -q -t "$IMAGE" scripts/emu >&2
fi
[ "${EMU_NO_BUILD:-}" = 1 ] || scripts/build-miyoo.sh >/dev/null

# $ROUND_PARAMS is "--param A=1 --param B=2" or empty: it must split into words
# shellcheck disable=SC2086
docker run --rm ${EMU_CONTAINER:+--name "$EMU_CONTAINER"} --user "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src \
	-e "PH_FAKEFB=640x480:/src/$FB" -e PH_INPUT=/dev/null "$IMAGE" \
	qemu-arm -L "$SYSROOT" build/miyoo/pocket-hustle --data "$DATA" --seed "$ROUND_SEED" --mute \
	$ROUND_PARAMS --frames "$ROUND_FRAMES" --keys "$ROUND_KEYS" --summary "$DATA.summary" "$@"

round_check "$NAME" "$DATA.summary" "the ARM build" || exit 1

if [ -f "$DATA/save.dat" ]; then
	mkdir -p build/host
	cc -std=c99 -Isrc -D_POSIX_C_SOURCE=200809L -o build/host/smoke-check tools/smoke/check.c src/core/*.c src/content/*.c -lm
	echo "emu-play: $NAME: $(build/host/smoke-check "$DATA/save.dat" 1 15)" >&2
else
	echo "emu-play: $NAME: the game ended (no save left); files: $(ls "$DATA" | tr '\n' ' ')" >&2
fi
