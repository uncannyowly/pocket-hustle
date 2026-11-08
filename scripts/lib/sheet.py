"""Cuts the key frames of a recorded round (raw RGB24, 320 x 240, one frame per 20 Hz tick) out
as PNGs and lays them out as a contact sheet.

  python3 -I sheet.py RAW EVERY "label:N label:N ..." OUTDIR NAME

EVERY is the frames between presses of the recording (the --pace in ticks, at least 3). The shot
"label:N" is the screen after the first N keys: the last frame before key N + 1 is pressed.
Writes OUTDIR/NAME/<label>.png (640 x 480) and OUTDIR/NAME-sheet.png."""
import os
import sys

from PIL import Image, ImageDraw

W, H = 320, 240
FRAME = W * H * 3
COLS = 3
PAD = 6
LABEL_H = 14


def main():
    raw_path, every, shots, outdir, name = sys.argv[1], int(sys.argv[2]), sys.argv[3].split(), sys.argv[4], sys.argv[5]
    size = os.path.getsize(raw_path)
    nframes = size // FRAME
    os.makedirs(os.path.join(outdir, name), exist_ok=True)
    tiles = []
    with open(raw_path, "rb") as f:
        for shot in shots:
            label, n = shot.rsplit(":", 1)
            # frame 0 of the run records nothing, so recorded frame j is frame j + 1 of the run
            idx = min(int(n) * every - 2, nframes - 1)
            if idx < 0:
                sys.exit("sheet: shot %s is before the first recorded frame" % shot)
            f.seek(idx * FRAME)
            img = Image.frombytes("RGB", (W, H), f.read(FRAME))
            img.resize((W * 2, H * 2), Image.NEAREST).save(os.path.join(outdir, name, label + ".png"))
            tiles.append((label, img))
    rows = (len(tiles) + COLS - 1) // COLS
    sheet = Image.new("RGB", (COLS * (W + PAD) + PAD, rows * (H + LABEL_H + PAD) + PAD), (24, 24, 32))
    draw = ImageDraw.Draw(sheet)
    for i, (label, img) in enumerate(tiles):
        x = PAD + (i % COLS) * (W + PAD)
        y = PAD + (i // COLS) * (H + LABEL_H + PAD)
        sheet.paste(img, (x, y + LABEL_H))
        draw.text((x, y + 1), "%d %s" % (i + 1, label), fill=(230, 230, 230))
    sheet.save(os.path.join(outdir, name + "-sheet.png"))


main()
