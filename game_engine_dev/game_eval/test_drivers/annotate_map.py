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
            text = parts[2].replace("\\n", "\n")
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
    lines = text.split("\n")
    sizes = [text_size(draw, line, font) for line in lines]
    gap = max(2, sizes[0][1] // 5) if sizes else 2
    block_h = sum(h for _, h in sizes) + gap * (len(lines) - 1)
    ty = y - block_h // 2
    for (line, (tw, th)) in zip(lines, sizes):
        tx = x - tw // 2
        for dx in (-2, -1, 0, 1, 2):
            for dy in (-2, -1, 0, 1, 2):
                if dx == 0 and dy == 0:
                    continue
                draw.text((tx + dx, ty + dy), line, font=font, fill=outline)
        draw.text((tx, ty), line, font=font, fill=fill)
        ty += th + gap

def heat_rgb(n: int, lo: int = 0, hi: int = 20):
    if hi <= lo:
        t = 0.0
    else:
        t = (max(lo, min(hi, n)) - lo) / float(hi - lo)
    return (int(255 * t), 0, int(255 * (1.0 - t)))

def parse_dot(text: str):
    if text == "@":
        return 0
    if text.startswith("@") and text[1:].isdigit():
        return int(text[1:])
    return None

def draw_dot(draw: ImageDraw.ImageDraw, x: int, y: int, r_in: int, r_out: int, fill):
    draw.ellipse((x - r_out, y - r_out, x + r_out, y + r_out), fill=(0, 0, 0))
    draw.ellipse((x - r_in, y - r_in, x + r_in, y + r_in), fill=fill)

def annotate(img_path: Path, anno_path: Path, out_path: Path, font_size: int, r_in: int, r_out: int):
    img = Image.open(img_path).convert("RGB")
    draw = ImageDraw.Draw(img)
    font = pick_font(font_size)
    for x, y, text in load_anno(anno_path):
        n = parse_dot(text)
        if n is not None:
            draw_dot(draw, x, y, r_in, r_out, heat_rgb(n))
        else:
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
    ap.add_argument("--dot-r-in", type=int, default=4, help="inner heat-dot radius for @N markers")
    ap.add_argument("--dot-r-out", type=int, default=6, help="outer black dot radius for @N markers")
    args = ap.parse_args()
    if not args.image.is_file():
        print(f"missing image: {args.image}")
        return 1
    if not args.anno.is_file():
        print(f"missing anno: {args.anno}")
        return 1
    annotate(args.image, args.anno, args.out, args.font_size, args.dot_r_in, args.dot_r_out)
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
