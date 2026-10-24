#!/usr/bin/env python3
"""
2x2 super-subtile probe: left = interactive (click to cycle + C++ splat),
right = static numbered key (tile 1..4). Same layout; black stamp px clear.
Nothing saved.
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
SCALE = 20
N = SUBTILE_DIV
HW = TILE_HALF_W
HH = TILE_HALF_H
MARGIN = 8

COLS = [
    (238, 214, 120),
    (70, 150, 65),
    (40, 90, 160),
    (210, 170, 70),
]

CORN_N, CORN_E, CORN_S, CORN_W = 1, 2, 4, 8
TIP_ORDER = (CORN_N, CORN_E, CORN_S, CORN_W)
TILE_NAMES = ("1", "2", "3", "4")


def darken(rgb, t=0.42):
    return tuple(max(0, int(c * t)) for c in rgb)


def tips_si_sj(bits):
    # Pattern-space corn (+col=+sj) does not match diamond screen axes:
    # tile2 is left of tile1, tile3 right. Remap N<->W, E<->S for (si,sj) stamps.
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



def is_black(rgb):
    return rgb[0] < 40 and rgb[1] < 40 and rgb[2] < 40


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
        self.lib.dia_corn_2x2.argtypes = [
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.dia_corn_2x2.restype = None

    def eval_2x2(self, cells):
        inp = (ctypes.c_int * 4)(*cells)
        out = (ctypes.c_int * 4)()
        self.lib.dia_corn_2x2(inp, out)
        return [int(out[i]) for i in range(4)]


def sub_ctr_local(si, sj, cx, cy):
    px = cx + (si - sj) * HW // N
    py = cy + (si + sj + 1) * HH // N - HH
    return px, py


class CornDiaProbe(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk7 corn_dia_probe (2x2 subtilles)")
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
        self.cells = [0, 1, 2, 0]
        self.si0, self.sj0 = 4, 4
        self.photo_l = None
        self.photo_r = None
        self.cx = MARGIN + HW
        self.cy = MARGIN + HH
        self._layout()
        self.lab_font = tkfont.Font(family="TkDefaultFont", size=28, weight="bold")

        row = tk.Frame(self)
        row.pack()
        left = tk.Frame(row)
        right = tk.Frame(row)
        left.pack(side=tk.LEFT, padx=4, pady=4)
        right.pack(side=tk.LEFT, padx=4, pady=4)
        tk.Label(left, text="interactive").pack()
        tk.Label(right, text="tile key (1..4)").pack()
        cw, ch = self.img_w * SCALE, self.img_h * SCALE
        self.cv_l = tk.Canvas(left, width=cw, height=ch, bg="#000000", highlightthickness=1)
        self.cv_r = tk.Canvas(right, width=cw, height=ch, bg="#000000", highlightthickness=1)
        self.cv_l.pack()
        self.cv_r.pack()
        self.cv_l.bind("<Button-1>", self._click)

        tk.Label(
            self,
            text="tile 1=(si,sj)  tile 2=(si,sj+1)  tile 3=(si+1,sj)  tile 4=(si+1,sj+1)  |  "
            "left: click to cycle  |  1px=%dx%d  |  nothing saved" % (SCALE, SCALE),
            padx=8,
            pady=6,
        ).pack(fill=tk.X)
        self.status = tk.Label(self, text="", padx=8, pady=4, anchor="w")
        self.status.pack(fill=tk.X)
        self._redraw_key()
        self._redraw()

    def _subs(self):
        return (
            (self.si0, self.sj0),
            (self.si0, self.sj0 + 1),
            (self.si0 + 1, self.sj0),
            (self.si0 + 1, self.sj0 + 1),
        )

    def _layout(self):
        sw, sh = self.sup["sw"], self.sup["sh"]
        ox, oy = self.sup["ox"], self.sup["oy"]
        min_x = min_y = 10 ** 9
        max_x = max_y = -10 ** 9
        for si, sj in self._subs():
            ax, ay = sub_ctr_local(si, sj, self.cx, self.cy)
            tlx, tly = ax - ox, ay - oy
            min_x = min(min_x, tlx)
            min_y = min(min_y, tly)
            max_x = max(max_x, tlx + sw - 1)
            max_y = max(max_y, tly + sh - 1)
        self.off_x = MARGIN - min_x
        self.off_y = MARGIN - min_y
        self.img_w = (max_x - min_x + 1) + MARGIN * 2
        self.img_h = (max_y - min_y + 1) + MARGIN * 2
        self.cx += self.off_x
        self.cy += self.off_y

    def _hit_cell(self, mx, my):
        gx = mx // SCALE
        gy = my // SCALE
        sw, sh = self.sup["sw"], self.sup["sh"]
        best, best_d = None, 1e18
        for i, (si, sj) in enumerate(self._subs()):
            ax, ay = sub_ctr_local(si, sj, self.cx, self.cy)
            dx, dy = gx - ax, gy - ay
            if abs(dx) > sw or abs(dy) > sh:
                continue
            d = dx * dx + dy * dy
            if d < best_d:
                best_d = d
                best = i
        return best

    def _click(self, e):
        i = self._hit_cell(e.x, e.y)
        if i is None:
            return
        self.cells[i] = (self.cells[i] + 1) % 4
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

    def _render(self, cells, use_splat):
        sw, sh = self.sup["sw"], self.sup["sh"]
        ox, oy = self.sup["ox"], self.sup["oy"]
        stamp = self.sup["stamp"]
        raw = self.lib.eval_2x2(cells) if use_splat else [0, 0, 0, 0]
        bits = [tips_si_sj(b) for b in raw]
        im = Image.new("RGB", (self.img_w, self.img_h), (0, 0, 0))
        px = im.load()
        hit = [[0] * self.img_w for _ in range(self.img_h)]
        n_splat = 0
        # pass 1: base stamps (first wins)
        for i, (si, sj) in enumerate(self._subs()):
            tid = cells[i]
            ax, ay = sub_ctr_local(si, sj, self.cx, self.cy)
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
        # pass 2: splat overwrites (tile4 N tip sits under tiles 1-3 otherwise)
        if use_splat:
            for i, (si, sj) in enumerate(self._subs()):
                tid = cells[i]
                cb = bits[i]
                if not cb:
                    continue
                ax, ay = sub_ctr_local(si, sj, self.cx, self.cy)
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
        return self._blow(im), bits, n_splat

    def _label_tiles(self, cv):
        for i, (si, sj) in enumerate(self._subs()):
            ax, ay = sub_ctr_local(si, sj, self.cx, self.cy)
            lx = ax * SCALE
            ly = ay * SCALE - int(2.2 * SCALE)
            name = TILE_NAMES[i]
            cv.create_text(lx + 1, ly + 1, text=name, fill="#000000", font=self.lab_font)
            cv.create_text(lx, ly, text=name, fill="#ffffff", font=self.lab_font)

    def _redraw_key(self):
        key_cells = [0, 1, 2, 3]
        big, _, _ = self._render(key_cells, use_splat=False)
        self.photo_r = ImageTk.PhotoImage(big)
        self.cv_r.delete("all")
        self.cv_r.create_image(0, 0, anchor=tk.NW, image=self.photo_r)
        self._label_tiles(self.cv_r)

    def _redraw(self):
        big, bits, n_splat = self._render(self.cells, use_splat=True)
        self.photo_l = ImageTk.PhotoImage(big)
        self.cv_l.delete("all")
        self.cv_l.create_image(0, 0, anchor=tk.NW, image=self.photo_l)
        parts = []
        for i in range(4):
            b = bits[i]
            t = "".join(
                ch for tip, ch in ((CORN_N, "N"), (CORN_E, "E"), (CORN_S, "S"), (CORN_W, "W")) if b & tip
            )
            parts.append("tile %d tex=%d%s" % (i + 1, self.cells[i], (":" + t) if t else ""))
        self.status.config(text="  ".join(parts) + "  |  splat_px=%d" % n_splat)


def main():
    CornDiaProbe().mainloop()


if __name__ == "__main__":
    main()
