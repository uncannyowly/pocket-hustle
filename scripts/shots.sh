#!/bin/sh
# Renders every screen with build/host/ph-shots and converts the PPMs to PNG.
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT=build/shots
rm -rf "$OUT"
mkdir -p "$OUT"
build/host/ph-shots "$OUT"
python3 -I - "$OUT" <<'PY'
import glob, sys
from PIL import Image
for p in sorted(glob.glob(sys.argv[1] + "/*.ppm")):
    Image.open(p).save(p[:-4] + ".png")
print("shots: PNGs in", sys.argv[1])
PY
