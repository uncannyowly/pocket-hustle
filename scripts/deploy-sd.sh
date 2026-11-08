#!/bin/sh
# Installs dist/ (scripts/package.sh) on an OnionOS SD card:
#   scripts/deploy-sd.sh /run/media/$USER/CARD
# Touches nothing outside Roms/PORTS/{Games/PocketHustle,Shortcuts/Strategy/Pocket Hustle.port,
# Imgs/Pocket Hustle.png}, and keeps the game folder's data/ (saves, scores, options).
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SD="${1:?SD card root}"
DIST="$ROOT/dist/Roms/PORTS"
DEST="$SD/Roms/PORTS/Games/PocketHustle"

[ -d "$SD/Roms/PORTS" ] || { echo "$SD does not look like an OnionOS card" >&2; exit 1; }
[ -x "$DIST/Games/PocketHustle/pocket-hustle" ] || { echo "run scripts/package.sh first" >&2; exit 1; }

mkdir -p "$DEST" "$SD/Roms/PORTS/Shortcuts/Strategy" "$SD/Roms/PORTS/Imgs"
for f in "$DIST/Games/PocketHustle"/*; do
	[ -f "$f" ] && command cp -f "$f" "$DEST/"
done
command cp -f "$DIST/Shortcuts/Strategy/Pocket Hustle.port" "$SD/Roms/PORTS/Shortcuts/Strategy/"
command cp -f "$DIST/Imgs/Pocket Hustle.png" "$SD/Roms/PORTS/Imgs/"
sync
echo "installed $(cat "$DEST/VERSION") to $DEST"
ls -la "$DEST"
