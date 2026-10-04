#!/usr/bin/env python3
import sys
from pathlib import Path

from PIL import Image, ImageDraw

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_marks(path: Path):
    staging = None
    groups = []
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) != 3:
                continue
            kind, xs, ys = parts[0], int(parts[1]), int(parts[2])
            if kind == "staging":
                staging = (xs, ys)
            elif kind == "group":
                groups.append((xs, ys))
    return staging, groups

def draw_plus(draw: ImageDraw.ImageDraw, x: int, y: int, color, half: int = 2):
    draw.line((x - half, y, x + half, y), fill=color, width=1)
    draw.line((x, y - half, x, y + half), fill=color, width=1)

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) != 4:
        print("usage: plot_failed_muster_map.py <base.ppm> <marks.txt> <out.ppm>")
        return 1
    base_p = Path(sys.argv[1])
    marks_p = Path(sys.argv[2])
    out_p = Path(sys.argv[3])
    staging, groups = load_marks(marks_p)
    if staging is None:
        print(f"missing staging in {marks_p}", file=sys.stderr)
        return 1
    img = Image.open(base_p).convert("RGB")
    draw = ImageDraw.Draw(img)
    for x, y in groups:
        draw_plus(draw, x, y, (220, 30, 30), half=2)
    draw_plus(draw, staging[0], staging[1], (0, 0, 0), half=2)
    out_p.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_p, format="PPM")
    print(f"wrote {out_p}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
