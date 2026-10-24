#!/usr/bin/env python3
"""
2x2 square-tile probe for mk7 corner detection (calls libdia_corn.so).
Click a tile to cycle its texture (4 colors). Dots mark inward tips from C++.
Nothing is saved.
"""

import ctypes
import os
import subprocess
import tkinter as tk

MK7 = os.path.dirname(os.path.abspath(__file__))
SO = os.path.join(MK7, "libdia_corn.so")
COMP = os.path.join(MK7, "corn_comp")

CELL = 160
PAD = 24
DOT_R = 10
INSET = 0.32

COLS = [
    (238, 214, 120),
    (70, 150, 65),
    (40, 90, 160),
    (210, 170, 70),
]

CORN_N, CORN_E, CORN_S, CORN_W = 1, 2, 4, 8


def darken(rgb, t=0.45):
    return tuple(max(0, int(c * t)) for c in rgb)


def hex_of(rgb):
    return "#%02x%02x%02x" % rgb


def ensure_so():
    if os.path.isfile(SO):
        return
    subprocess.check_call(["bash", COMP], cwd=MK7)


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


class CornProbe(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk7 corn_probe (2x2 squares)")
        self.resizable(False, False)
        self.lib = CornLib()
        self.cells = [0, 1, 2, 0]
        self.cv = tk.Canvas(
            self,
            width=PAD * 2 + CELL * 2,
            height=PAD * 2 + CELL * 2,
            bg="#1a1a1e",
            highlightthickness=0,
        )
        self.cv.pack()
        self.cv.bind("<Button-1>", self._click)
        tip = tk.Label(
            self,
            text="click tile to cycle texture  |  dark dots = C++ inward corner tips  |  nothing saved",
            padx=8,
            pady=6,
        )
        tip.pack(fill=tk.X)
        self.status = tk.Label(self, text="", padx=8, pady=4, anchor="w")
        self.status.pack(fill=tk.X)
        self._redraw()

    def _cell_rect(self, i):
        r, c = divmod(i, 2)
        x0 = PAD + c * CELL
        y0 = PAD + r * CELL
        return x0, y0, x0 + CELL, y0 + CELL

    def _corner_pt(self, i, tip):
        x0, y0, x1, y1 = self._cell_rect(i)
        if tip == CORN_W:
            return x0, y0
        if tip == CORN_N:
            return x1, y0
        if tip == CORN_E:
            return x1, y1
        return x0, y1

    def _dot_xy(self, i, tip):
        x0, y0, x1, y1 = self._cell_rect(i)
        cx = (x0 + x1) * 0.5
        cy = (y0 + y1) * 0.5
        jx, jy = self._corner_pt(i, tip)
        return jx + (cx - jx) * INSET, jy + (cy - jy) * INSET

    def _click(self, e):
        for i in range(4):
            x0, y0, x1, y1 = self._cell_rect(i)
            if x0 <= e.x < x1 and y0 <= e.y < y1:
                self.cells[i] = (self.cells[i] + 1) % 4
                self._redraw()
                return

    def _redraw(self):
        self.cv.delete("all")
        bits = self.lib.eval_2x2(self.cells)
        for i in range(4):
            rgb = COLS[self.cells[i]]
            x0, y0, x1, y1 = self._cell_rect(i)
            self.cv.create_rectangle(x0, y0, x1, y1, fill=hex_of(rgb), outline="#303038", width=2)
            b = bits[i]
            drgb = darken(rgb)
            for tip, ch in ((CORN_N, "N"), (CORN_E, "E"), (CORN_S, "S"), (CORN_W, "W")):
                if not (b & tip):
                    continue
                dx, dy = self._dot_xy(i, tip)
                self.cv.create_oval(
                    dx - DOT_R,
                    dy - DOT_R,
                    dx + DOT_R,
                    dy + DOT_R,
                    fill=hex_of(drgb),
                    outline="#101010",
                )
        labels = ["NW", "NE", "SW", "SE"]
        parts = []
        for i in range(4):
            b = bits[i]
            tips = "".join(ch for tip, ch in ((CORN_N, "N"), (CORN_E, "E"), (CORN_S, "S"), (CORN_W, "W")) if b & tip)
            parts.append("%s=%d%s" % (labels[i], self.cells[i], (":" + tips) if tips else ""))
        self.status.config(text="  ".join(parts))


def main():
    CornProbe().mainloop()


if __name__ == "__main__":
    main()
