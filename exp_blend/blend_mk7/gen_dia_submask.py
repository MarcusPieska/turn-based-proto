#!/usr/bin/env python3
"""Build the diamond tile pixel -> (si, sj) subtile map (same math as DiaBlit::home)."""

import math
import os
import struct

from PIL import Image

from dia_cfg import (
    SUBTILE_DIV,
    TILE_HALF_W,
    TILE_HALF_H,
    TEX_W,
    TEX_H,
)

OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "dia_submask")
PNG_DIR = "/home/w/Projects/simple-map-gen/blending/mk7-dia/dia_submask"
N = SUBTILE_DIV


def in_dia(cx, cy, x, y):
    return abs(x - cx) * TILE_HALF_H + abs(y - cy) * TILE_HALF_W <= TILE_HALF_W * TILE_HALF_H


def home(cx, cy, x, y):
    ax = float(TILE_HALF_W) / float(N)
    ay = float(TILE_HALF_H) / float(N)
    dx = float(x - cx)
    dy = float(y - (cy - TILE_HALF_H))
    si = int(math.floor(0.5 * (dx / ax + dy / ay)))
    sj = int(math.floor(0.5 * (dy / ay - dx / ax)))
    if si < 0:
        si = 0
    if sj < 0:
        sj = 0
    if si >= N:
        si = N - 1
    if sj >= N:
        sj = N - 1
    return si, sj


def to_pat(si, sj):
    return N - 1 - sj, si


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(PNG_DIR, exist_ok=True)
    cx = TILE_HALF_W
    cy = TILE_HALF_H

    pix = bytearray(TEX_W * TEX_H)
    counts = [[0] * N for _ in range(N)]
    img = Image.new("RGB", (TEX_W, TEX_H), (0, 0, 0))
    px = img.load()

    for y in range(TEX_H):
        for x in range(TEX_W):
            i = y * TEX_W + x
            if not in_dia(cx, cy, x, y):
                pix[i] = 0xFF
                continue
            si, sj = home(cx, cy, x, y)
            pix[i] = (si << 4) | sj
            counts[si][sj] += 1
            r = 40 + ((si + sj) * 19) % 180
            g = 40 + ((si * 37 + sj * 17) % 180)
            b = 40 + ((si * 13 + sj * 41) % 180)
            px[x, y] = (r, g, b)

    bin_path = os.path.join(OUT_DIR, "pixel_to_sub.bin")
    with open(bin_path, "wb") as f:
        f.write(struct.pack("<HHBB", TEX_W, TEX_H, N, 1))
        f.write(pix)

    png_path = os.path.join(PNG_DIR, "pixel_to_sub.png")
    img.save(png_path)

    outline = Image.new("RGB", (TEX_W, TEX_H), (20, 20, 20))
    op = outline.load()
    for y in range(TEX_H):
        for x in range(TEX_W):
            v = pix[y * TEX_W + x]
            if v == 0xFF:
                continue
            si, sj = v >> 4, v & 0xF
            edge = False
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                x2, y2 = x + dx, y + dy
                if x2 < 0 or y2 < 0 or x2 >= TEX_W or y2 >= TEX_H:
                    edge = True
                    break
                v2 = pix[y2 * TEX_W + x2]
                if v2 == 0xFF or v2 != v:
                    edge = True
                    break
            if edge:
                op[x, y] = (240, 240, 240)
            else:
                g = 50 + ((si * 37 + sj * 17) % 160)
                op[x, y] = (30, g, 80)

    outline_path = os.path.join(PNG_DIR, "subtile_outlines.png")
    outline.save(outline_path)

    cnt_path = os.path.join(OUT_DIR, "subtile_counts.data")
    with open(cnt_path, "w") as f:
        f.write("# si,sj pixel counts inside one diamond tile (tex %dx%d)\n" % (TEX_W, TEX_H))
        f.write("# home() matches DiaBlit; pattern local li=N-1-sj lj=si\n")
        for si in range(N):
            for sj in range(N):
                li, lj = to_pat(si, sj)
                f.write("%d %d  count=%d  pat_li=%d pat_lj=%d\n" % (si, sj, counts[si][sj], li, lj))

    empty = [(si, sj) for si in range(N) for sj in range(N) if counts[si][sj] == 0]
    print("wrote", bin_path)
    print("wrote", png_path)
    print("wrote", outline_path)
    print("wrote", cnt_path)
    print("tex=%dx%d  subtilles=%dx%d  empty=%s" % (TEX_W, TEX_H, N, N, empty if empty else "none"))


if __name__ == "__main__":
    main()
