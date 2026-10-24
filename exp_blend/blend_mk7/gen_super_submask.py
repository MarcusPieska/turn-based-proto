#!/usr/bin/env python3
"""
Derive a center-anchored super subtile mask from all diamond subtilles, then
preview tiling. Anchor = DiaGrid::sub_ctr (integer); stamp top-left =
round(ctr) - (ox, oy) with (ox, oy) = stamp coords of that center.
"""

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
    si = max(0, min(N - 1, si))
    sj = max(0, min(N - 1, sj))
    return si, sj


def sub_ctr(cx, cy, si, sj):
    # DiaGrid::sub_ctr for a tile whose center is (cx, cy)
    px = cx + (si - sj) * TILE_HALF_W // N
    py = cy + (si + sj + 1) * TILE_HALF_H // N - TILE_HALF_H
    return px, py


def build_pixel_map():
    cx, cy = TILE_HALF_W, TILE_HALF_H
    cells = [[[] for _ in range(N)] for _ in range(N)]
    for y in range(TEX_H):
        for x in range(TEX_W):
            if not in_dia(cx, cy, x, y):
                continue
            si, sj = home(cx, cy, x, y)
            cells[si][sj].append((x, y))
    return cx, cy, cells


def build_super_mask(cx, cy, cells):
    # relative pixel lists vs sub_ctr
    rels = []
    for si in range(N):
        for sj in range(N):
            ax, ay = sub_ctr(cx, cy, si, sj)
            for x, y in cells[si][sj]:
                rels.append((x - ax, y - ay))
    if not rels:
        raise SystemExit("no subtile pixels")

    min_rx = min(r for r, _ in rels)
    max_rx = max(r for r, _ in rels)
    min_ry = min(r for _, r in rels)
    max_ry = max(r for _, r in rels)
    sw = max_rx - min_rx + 1
    sh = max_ry - min_ry + 1
    ox = -min_rx  # stamp x of center
    oy = -min_ry  # stamp y of center

    mask = [[0] * sw for _ in range(sh)]
    cover = [[0] * sw for _ in range(sh)]  # how many subtilles paint this stamp pixel
    for si in range(N):
        for sj in range(N):
            ax, ay = sub_ctr(cx, cy, si, sj)
            seen = set()
            for x, y in cells[si][sj]:
                sx = (x - ax) - min_rx
                sy = (y - ay) - min_ry
                if (sx, sy) in seen:
                    continue
                seen.add((sx, sy))
                mask[sy][sx] = 1
                cover[sy][sx] += 1

    meta = {
        "sw": sw,
        "sh": sh,
        "ox": ox,
        "oy": oy,
        "min_rx": min_rx,
        "min_ry": min_ry,
        "max_rx": max_rx,
        "max_ry": max_ry,
    }
    return mask, cover, meta


def save_mask_png(mask, cover, meta, path_mask, path_cover):
    sw, sh = meta["sw"], meta["sh"]
    ox, oy = meta["ox"], meta["oy"]
    im = Image.new("RGB", (sw, sh), (0, 0, 0))
    px = im.load()
    for y in range(sh):
        for x in range(sw):
            if mask[y][x]:
                px[x, y] = (220, 220, 80)
            else:
                px[x, y] = (30, 30, 30)
    # mark center anchor
    if 0 <= ox < sw and 0 <= oy < sh:
        px[ox, oy] = (255, 40, 40)
    im.save(path_mask)

    # coverage heat for debug
    mx = max((cover[y][x] for y in range(sh) for x in range(sw)), default=1)
    ic = Image.new("RGB", (sw, sh), (0, 0, 0))
    pc = ic.load()
    for y in range(sh):
        for x in range(sw):
            c = cover[y][x]
            if c == 0:
                pc[x, y] = (20, 20, 20)
            else:
                t = int(80 + 175 * c / mx)
                pc[x, y] = (t, 60, 200 - t // 2)
    if 0 <= ox < sw and 0 <= oy < sh:
        pc[ox, oy] = (255, 255, 255)
    ic.save(path_cover)


def save_mask_bin(mask, meta, path):
    sw, sh = meta["sw"], meta["sh"]
    raw = bytearray(sw * sh)
    for y in range(sh):
        for x in range(sw):
            raw[y * sw + x] = 1 if mask[y][x] else 0
    with open(path, "wb") as f:
        f.write(struct.pack("<HHHHii", sw, sh, meta["ox"], meta["oy"], meta["min_rx"], meta["min_ry"]))
        f.write(raw)


def blit_stamp(dst, mask, meta, ax, ay, rgb, mode="or"):
    sw, sh = meta["sw"], meta["sh"]
    ox, oy = meta["ox"], meta["oy"]
    tlx = ax - ox
    tly = ay - oy
    dw, dh = dst.size
    px = dst.load()
    for sy in range(sh):
        for sx in range(sw):
            if not mask[sy][sx]:
                continue
            x = tlx + sx
            y = tly + sy
            if x < 0 or y < 0 or x >= dw or y >= dh:
                continue
            if mode == "or":
                if px[x, y] == (0, 0, 0):
                    px[x, y] = rgb
                else:
                    # overlap: lighten / blend toward white
                    r, g, b = px[x, y]
                    px[x, y] = (min(255, (r + rgb[0]) // 2 + 40), min(255, (g + rgb[1]) // 2 + 40), min(255, (b + rgb[2]) // 2 + 40))
            else:
                px[x, y] = rgb


def tile_preview(mask, meta, path):
    # One full diamond tile of subtilles + a bit of neighbor tiles for overlap view
    pad = max(meta["sw"], meta["sh"]) + 8
    # tile centers for a small 2x2 axial patch
    tiles = [(0, 0), (0, 1), (1, 0), (1, 1)]
    # place (0,0) center at (pad + HALF_W, pad + HALF_H)
    base_cx = pad + TILE_HALF_W
    base_cy = pad + TILE_HALF_H

    def tile_center(dr, dc):
        return (
            base_cx + (dc + dr) * TILE_HALF_W,
            base_cy + (dr - dc) * TILE_HALF_H,
        )

    # canvas bounds
    max_x = 0
    max_y = 0
    for dr, dc in tiles:
        tcx, tcy = tile_center(dr, dc)
        for si in range(N):
            for sj in range(N):
                ax, ay = sub_ctr(tcx, tcy, si, sj)
                tlx = ax - meta["ox"]
                tly = ay - meta["oy"]
                max_x = max(max_x, tlx + meta["sw"])
                max_y = max(max_y, tly + meta["sh"])
    dw = max_x + pad
    dh = max_y + pad
    dst = Image.new("RGB", (dw, dh), (0, 0, 0))

    for dr, dc in tiles:
        tcx, tcy = tile_center(dr, dc)
        for si in range(N):
            for sj in range(N):
                ax, ay = sub_ctr(tcx, tcy, si, sj)
                rgb = (
                    40 + ((si + sj + dr * 3) * 19) % 160,
                    50 + ((si * 37 + sj * 17 + dc * 11) % 160),
                    70 + ((si * 13 + sj * 41) % 140),
                )
                blit_stamp(dst, mask, meta, ax, ay, rgb, mode="or")

    # also a single-tile clean view
    dst.save(path)

    # single tile only
    pad2 = max(meta["sw"], meta["sh"]) + 4
    tcx, tcy = pad2 + TILE_HALF_W, pad2 + TILE_HALF_H
    max_x = max_y = 0
    for si in range(N):
        for sj in range(N):
            ax, ay = sub_ctr(tcx, tcy, si, sj)
            tlx = ax - meta["ox"]
            tly = ay - meta["oy"]
            max_x = max(max_x, tlx + meta["sw"])
            max_y = max(max_y, tly + meta["sh"])
    one = Image.new("RGB", (max_x + pad2, max_y + pad2), (0, 0, 0))
    for si in range(N):
        for sj in range(N):
            ax, ay = sub_ctr(tcx, tcy, si, sj)
            rgb = (
                50 + (si * 20) % 150,
                80 + (sj * 18) % 150,
                100,
            )
            blit_stamp(one, mask, meta, ax, ay, rgb, mode="or")
    return one


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    os.makedirs(PNG_DIR, exist_ok=True)

    cx, cy, cells = build_pixel_map()
    mask, cover, meta = build_super_mask(cx, cy, cells)

    bin_path = os.path.join(OUT_DIR, "super_submask.bin")
    save_mask_bin(mask, meta, bin_path)

    meta_path = os.path.join(OUT_DIR, "super_submask.meta")
    with open(meta_path, "w") as f:
        f.write("# super subtile stamp  anchor=sub_ctr  tl = ctr - (ox,oy)\n")
        for k in ("sw", "sh", "ox", "oy", "min_rx", "min_ry", "max_rx", "max_ry"):
            f.write("%s=%d\n" % (k, meta[k]))
        n_on = sum(mask[y][x] for y in range(meta["sh"]) for x in range(meta["sw"]))
        f.write("on_pixels=%d\n" % n_on)

    path_mask = os.path.join(PNG_DIR, "super_submask.png")
    path_cover = os.path.join(PNG_DIR, "super_submask_cover.png")
    save_mask_png(mask, cover, meta, path_mask, path_cover)

    path_tile4 = os.path.join(PNG_DIR, "super_submask_tile4.png")
    one = tile_preview(mask, meta, path_tile4)
    path_tile1 = os.path.join(PNG_DIR, "super_submask_tile1.png")
    one.save(path_tile1)

    print("super stamp %dx%d  center@(ox,oy)=(%d,%d)" % (meta["sw"], meta["sh"], meta["ox"], meta["oy"]))
    print("rel bbox x[%d,%d] y[%d,%d]" % (meta["min_rx"], meta["max_rx"], meta["min_ry"], meta["max_ry"]))
    print("wrote", bin_path)
    print("wrote", meta_path)
    print("wrote", path_mask)
    print("wrote", path_cover)
    print("wrote", path_tile1)
    print("wrote", path_tile4)


if __name__ == "__main__":
    main()
