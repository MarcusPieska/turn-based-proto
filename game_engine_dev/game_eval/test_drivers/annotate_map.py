#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_anno(path: Path):
    items = []
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(None, 2)
            if len(parts) < 3:
                raise ValueError(f"bad annotation line (need x y text): {line}")
            x = int(parts[0])
            y = int(parts[1])
            text = parts[2]
            items.append((x, y, text))
    return items

def pick_font(size: int):
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
    ]
    for p in candidates:
        if Path(p).is_file():
            return ImageFont.truetype(p, size=size)
    return ImageFont.load_default()

#================================================================================================================================
#=> - Draw -
#================================================================================================================================

def text_size(draw: ImageDraw.ImageDraw, text: str, font):
    box = draw.textbbox((0, 0), text, font=font)
    return box[2] - box[0], box[3] - box[1]

def draw_label(draw: ImageDraw.ImageDraw, x: int, y: int, text: str, font, fill, outline):
    tw, th = text_size(draw, text, font)
    tx = x - tw // 2
    ty = y - th // 2
    for dx in (-2, -1, 0, 1, 2):
        for dy in (-2, -1, 0, 1, 2):
            if dx == 0 and dy == 0:
                continue
            draw.text((tx + dx, ty + dy), text, font=font, fill=outline)
    draw.text((tx, ty), text, font=font, fill=fill)

def annotate(img_path: Path, anno_path: Path, out_path: Path, font_size: int):
    img = Image.open(img_path).convert("RGB")
    draw = ImageDraw.Draw(img)
    font = pick_font(font_size)
    for x, y, text in load_anno(anno_path):
        draw_label(draw, x, y, text, font, fill=(255, 255, 40), outline=(0, 0, 0))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(out_path)
    print(f"wrote {out_path}")

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    ap = argparse.ArgumentParser(
        description="Draw text annotations onto a map image. Anno lines: x y text")
    ap.add_argument("image", type=Path, help="input map image (ppm/png/...)")
    ap.add_argument("anno", type=Path, help="annotation text file")
    ap.add_argument("out", type=Path, help="output annotated image path")
    ap.add_argument("--font-size", type=int, default=18, help="label font size (default 18)")
    args = ap.parse_args()
    if not args.image.is_file():
        print(f"missing image: {args.image}")
        return 1
    if not args.anno.is_file():
        print(f"missing anno: {args.anno}")
        return 1
    annotate(args.image, args.anno, args.out, args.font_size)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
