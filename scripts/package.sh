#!/bin/sh
# Builds the Miyoo binary and assembles the SD card layout in dist/:
#   dist/Roms/PORTS/...                 copy to the SD card root
#   dist/pocket-hustle-<version>.zip    the same, zipped
# Needs Docker (the toolchain image), python3 with PIL (box art) and make shots' dependencies.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
VERSION="$(cat VERSION)"
OUT="$ROOT/dist"
GAME="$OUT/Roms/PORTS/Games/PocketHustle"
IMAGE=gta1-miyoo-toolchain

scripts/build-miyoo.sh >/dev/null
make shots >/dev/null

rm -rf "$OUT/Roms" "$OUT"/pocket-hustle-*.zip
mkdir -p "$GAME" "$OUT/Roms/PORTS/Shortcuts/Strategy" "$OUT/Roms/PORTS/Imgs"
docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT:/src" -w /src "$IMAGE" \
	arm-linux-gnueabihf-strip -o dist/Roms/PORTS/Games/PocketHustle/pocket-hustle build/miyoo/pocket-hustle
command cp -f pkg/launch.sh pkg/README.txt "$GAME/"
command cp -f assets/fonts/OFL.txt "$GAME/OFL.txt"
echo "$VERSION" > "$GAME/VERSION"
command cp -f "pkg/Pocket Hustle.port" "$OUT/Roms/PORTS/Shortcuts/Strategy/"
# box art: the title screen on a square 256 x 256 tile, the size Scorched Earth's art has
python3 -I - build/shots/01-title.png "$OUT/Roms/PORTS/Imgs/Pocket Hustle.png" <<'PY'
import sys
from PIL import Image
src = Image.open(sys.argv[1]).convert("RGB")
w = 256
h = src.height * w // src.width
shot = src.resize((w, h), Image.NEAREST)
tile = Image.new("RGB", (256, 256), src.getpixel((0, 0)))
tile.paste(shot, (0, (256 - h) // 2))
tile.save(sys.argv[2])
PY
chmod +x "$GAME/pocket-hustle" "$GAME/launch.sh" "$OUT/Roms/PORTS/Shortcuts/Strategy/Pocket Hustle.port"
(cd "$OUT" && zip -qr "pocket-hustle-$VERSION.zip" Roms)
echo "packaged $OUT/pocket-hustle-$VERSION.zip"
