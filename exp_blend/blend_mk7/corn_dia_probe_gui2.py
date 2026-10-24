#!/usr/bin/env python3
"""
mk7-style 2x2 diamond tile field (20x20 subtilles), super_submask.png (black=clear),
C++ corn + tip splat. Two textures only. Each stamp pixel = SCALE x SCALE box.
Click a subtile to toggle texture (starts all tex0). Nothing saved.
"""

import ctypes
import os
import struct
import subprocess
import tkinter as tk
from tkinter import font as tkfont

from PIL import Image, ImageTk

from dia_cfg import SUBTILE_DIV, TILE_HALF_W, TILE_HALF_H

MK7 = os.path.dirname(os.path.abspath(__file__))
SO = os.path.join(MK7, "libdia_corn.so")
COMP = os.path.join(MK7, "corn_comp")
MASK_BIN = os.path.join(MK7, "dia_submask", "super_submask.bin")
MASK_PNG = "/home/w/Projects/simple-map-gen/blending/mk7-dia/dia_submask/super_submask.png"
SCALE = 8
N = SUBTILE_DIV
HW = TILE_HALF_W
HH = TILE_HALF_H
MARGIN = 12
TR = 2
TC = 2
SROWS = TR * N
SCOLS = TC * N

COLS = [
    (238, 214, 120),
    (40, 90, 160),
]

CORN_N, CORN_E, CORN_S, CORN_W = 1, 2, 4, 8
TIP_ORDER = (CORN_N, CORN_E, CORN_S, CORN_W)


def darken(rgb, t=0.42):
    return tuple(max(0, int(c * t)) for c in rgb)


def is_black(rgb):
    return rgb[0] < 40 and rgb[1] < 40 and rgb[2] < 40


def tips_si_sj(bits):
    o = 0
    if bits & CORN_N:
        o |= CORN_W
    if bits & CORN_W:
        o |= CORN_N
    if bits & CORN_E:
        o |= CORN_S
    if bits & CORN_S:
        o |= CORN_E
    return o


def ensure_so():
    if not os.path.isfile(SO):
        subprocess.check_call(["bash", COMP], cwd=MK7)


def load_super(png_path, bin_path):
    with open(bin_path, "rb") as f:
        sw, sh, ox, oy, min_rx, min_ry = struct.unpack("<HHHHii", f.read(16))
    im = Image.open(png_path).convert("RGB")
    if im.size != (sw, sh):
        raise SystemExit("png size %s != bin %dx%d" % (im.size, sw, sh))
    pix = im.load()
    stamp = [[pix[x, y] for x in range(sw)] for y in range(sh)]
    return {"sw": sw, "sh": sh, "ox": ox, "oy": oy, "stamp": stamp}


def load_splat(path, sw, sh):
    rows = []
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if len(line) != sw or any(ch not in "01" for ch in line):
                continue
            rows.append([int(ch) for ch in line])
    if len(rows) != sh:
        raise SystemExit("bad splat %s" % path)
    return rows


class CornLib:
    def __init__(self):
        ensure_so()
        self.lib = ctypes.CDLL(SO)
        self.lib.dia_corn_grid_c.argtypes = [
            ctypes.POINTER(ctypes.c_int),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.dia_corn_grid_c.restype = None

    def eval_grid(self, grid, rows, cols):
        n = rows * cols
        inp = (ctypes.c_int * n)(*grid)
        out = (ctypes.c_int * n)()
        self.lib.dia_corn_grid_c(inp, rows, cols, out)
        return [tips_si_sj(int(out[i])) for i in range(n)]


def tile_center(r, c, x0, y0):
    return x0 + (c + r) * HW, y0 + (r - c) * HH


def sub_ctr(r, c, si, sj, x0, y0):
    cx, cy = tile_center(r, c, x0, y0)
    px = cx + (si - sj) * HW // N
    py = cy + (si + sj + 1) * HH // N - HH
    return px, py


class CornDiaProbe2(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk7 corn_dia_probe2 (20x20 in 2x2 diamonds)")
        self.resizable(False, False)
        self.lib = CornLib()
        self.sup = load_super(MASK_PNG, MASK_BIN)
        sw, sh = self.sup["sw"], self.sup["sh"]
        self.splats = {
            CORN_N: load_splat(os.path.join(MK7, "splat_n_bank", "n-000.splat"), sw, sh),
            CORN_E: load_splat(os.path.join(MK7, "splat_e_bank", "e-000.splat"), sw, sh),
            CORN_S: load_splat(os.path.join(MK7, "splat_s_bank", "s-000.splat"), sw, sh),
            CORN_W: load_splat(os.path.join(MK7, "splat_w_bank", "w-000.splat"), sw, sh),
        }
        self.cells = [0] * (SROWS * SCOLS)
        self.photo = None
        self.lab_font = tkfont.Font(family="TkDefaultFont", size=18, weight="bold")
        self.x0 = MARGIN + HW
        self.y0 = MARGIN + HH
        self._layout()
        cw, ch = self.img_w * SCALE, self.img_h * SCALE
        self.cv = tk.Canvas(self, width=cw, height=ch, bg="#000000", highlightthickness=0)
        self.cv.pack()
        self.cv.bind("<Button-1>", self._click)
        tk.Label(
            self,
            text="2x2 tiles / 20x20 subtilles  |  2 textures  |  click subtile to toggle  |  "
            "1px=%dx%d  |  nothing saved" % (SCALE, SCALE),
            padx=8,
            pady=6,
        ).pack(fill=tk.X)
        self.status = tk.Label(self, text="", padx=8, pady=4, anchor="w")
        self.status.pack(fill=tk.X)
        self._redraw()

    def _layout(self):
        sw, sh = self.sup["sw"], self.sup["sh"]
        ox, oy = self.sup["ox"], self.sup["oy"]
        min_x = min_y = 10 ** 9
        max_x = max_y = -10 ** 9
        for tr in range(TR):
            for tc in range(TC):
                for si in range(N):
                    for sj in range(N):
                        ax, ay = sub_ctr(tr, tc, si, sj, self.x0, self.y0)
                        tlx, tly = ax - ox, ay - oy
                        min_x = min(min_x, tlx)
                        min_y = min(min_y, tly)
                        max_x = max(max_x, tlx + sw - 1)
                        max_y = max(max_y, tly + sh - 1)
        self.off_x = MARGIN - min_x
        self.off_y = MARGIN - min_y
        self.img_w = (max_x - min_x + 1) + MARGIN * 2
        self.img_h = (max_y - min_y + 1) + MARGIN * 2
        self.x0 += self.off_x
        self.y0 += self.off_y

    def _hit_sub(self, mx, my):
        gx = mx // SCALE
        gy = my // SCALE
        sw, sh = self.sup["sw"], self.sup["sh"]
        ox, oy = self.sup["ox"], self.sup["oy"]
        best, best_d = None, 1e18
        for tr in range(TR):
            for tc in range(TC):
                for si in range(N):
                    for sj in range(N):
                        ax, ay = sub_ctr(tr, tc, si, sj, self.x0, self.y0)
                        dx, dy = gx - ax, gy - ay
                        if abs(dx) > sw or abs(dy) > sh:
                            continue
                        d = dx * dx + dy * dy
                        if d < best_d:
                            best_d = d
                            best = (tr * N + si) * SCOLS + (tc * N + sj)
        return best

    def _click(self, e):
        i = self._hit_sub(e.x, e.y)
        if i is None:
            return
        self.cells[i] = 1 - self.cells[i]
        self._redraw()

    def _blow(self, im):
        bw, bh = self.img_w * SCALE, self.img_h * SCALE
        big = Image.new("RGB", (bw, bh), (0, 0, 0))
        sp = im.load()
        bp = big.load()
        for y in range(self.img_h):
            for x in range(self.img_w):
                col = sp[x, y]
                if col == (0, 0, 0):
                    continue
                x0, y0 = x * SCALE, y * SCALE
                for dy in range(SCALE):
                    for dx in range(SCALE):
                        bp[x0 + dx, y0 + dy] = col
        return big

    def _tip_hit(self, bits, sx, sy):
        for tip in TIP_ORDER:
            if not (bits & tip):
                continue
            if self.splats[tip][sy][sx]:
                return tip
        return 0

    def _stamp_col(self, stamp_rgb, tid, splat):
        if stamp_rgb[0] > 200 and stamp_rgb[1] < 80:
            return darken(stamp_rgb, 0.55) if splat else stamp_rgb
        base = COLS[tid]
        y = stamp_rgb[0] / 220.0 if stamp_rgb[0] else 1.0
        y = max(0.35, min(1.0, y))
        col = tuple(min(255, int(base[i] * y)) for i in range(3))
        return darken(col) if splat else col

    def _redraw(self):
        sw, sh = self.sup["sw"], self.sup["sh"]
        ox, oy = self.sup["ox"], self.sup["oy"]
        stamp = self.sup["stamp"]
        grid = self.cells
        corn = self.lib.eval_grid(grid, SROWS, SCOLS)
        im = Image.new("RGB", (self.img_w, self.img_h), (0, 0, 0))
        px = im.load()
        hit = [[0] * self.img_w for _ in range(self.img_h)]
        n_splat = 0
        for tr in range(TR):
            for tc in range(TC):
                for si in range(N):
                    for sj in range(N):
                        sr = tr * N + si
                        sc = tc * N + sj
                        tid = grid[sr * SCOLS + sc]
                        ax, ay = sub_ctr(tr, tc, si, sj, self.x0, self.y0)
                        tlx, tly = ax - ox, ay - oy
                        for sy in range(sh):
                            for sx in range(sw):
                                srgb = stamp[sy][sx]
                                if is_black(srgb):
                                    continue
                                x = tlx + sx
                                y = tly + sy
                                if x < 0 or y < 0 or x >= self.img_w or y >= self.img_h:
                                    continue
                                if hit[y][x]:
                                    continue
                                hit[y][x] = 1
                                px[x, y] = self._stamp_col(srgb, tid, False)
        for tr in range(TR):
            for tc in range(TC):
                for si in range(N):
                    for sj in range(N):
                        sr = tr * N + si
                        sc = tc * N + sj
                        cb = corn[sr * SCOLS + sc]
                        if not cb:
                            continue
                        tid = grid[sr * SCOLS + sc]
                        ax, ay = sub_ctr(tr, tc, si, sj, self.x0, self.y0)
                        tlx, tly = ax - ox, ay - oy
                        for sy in range(sh):
                            for sx in range(sw):
                                srgb = stamp[sy][sx]
                                if is_black(srgb):
                                    continue
                                if not self._tip_hit(cb, sx, sy):
                                    continue
                                x = tlx + sx
                                y = tly + sy
                                if x < 0 or y < 0 or x >= self.img_w or y >= self.img_h:
                                    continue
                                px[x, y] = self._stamp_col(srgb, tid, True)
                                n_splat += 1
        self.photo = ImageTk.PhotoImage(self._blow(im))
        self.cv.delete("all")
        self.cv.create_image(0, 0, anchor=tk.NW, image=self.photo)
        for tr in range(TR):
            for tc in range(TC):
                i = tr * TC + tc
                cx, cy = tile_center(tr, tc, self.x0, self.y0)
                lx, ly = cx * SCALE, cy * SCALE - 3 * SCALE
                name = str(i + 1)
                self.cv.create_text(lx + 1, ly + 1, text=name, fill="#000000", font=self.lab_font)
                self.cv.create_text(lx, ly, text=name, fill="#ffffff", font=self.lab_font)
        n0 = sum(1 for v in grid if v == 0)
        n1 = sum(1 for v in grid if v == 1)
        n_corn = sum(1 for b in corn if b)
        self.status.config(
            text="tex0=%d  tex1=%d  |  corn_cells=%d  splat_px=%d" % (n0, n1, n_corn, n_splat)
        )


def main():
    CornDiaProbe2().mainloop()


if __name__ == "__main__":
    main()
