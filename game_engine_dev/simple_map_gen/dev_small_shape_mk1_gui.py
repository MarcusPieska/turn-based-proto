#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
from tkinter import ttk

from dev_small_shape_mk1_shape import blob_mask

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

CANVAS_PX = 801
GRID_STEP = 16
GRID_N = CANVAS_PX // GRID_STEP
GRID_COL = "#c0c0c0"
LIM_COL = "#a8c8e8"
BG_COL = "#ffffff"
FILL_COL = "#6b8e4e"
SLIDER_MIN = 5
SLIDER_MAX = 50
SLIDER_DEF = 20

#================================================================================================================================#
#=> - Shape generation -
#================================================================================================================================#

def _seed_from_click(cx, cy, lim_x, lim_y):
    return (cx * 73856093) ^ (cy * 19349663) ^ (lim_x * 83492791) ^ (lim_y * 50331653)

def generate_shape(canvas, limit_x, limit_y, click_x, click_y):
    gx = max(0, min(GRID_N - 1, click_x // GRID_STEP))
    gy = max(0, min(GRID_N - 1, click_y // GRID_STEP))
    seed = _seed_from_click(gx, gy, limit_x, limit_y)
    mask = blob_mask(GRID_N, GRID_N, gx, gy, limit_x, limit_y, seed)
    for cy in range(GRID_N):
        for cx in range(GRID_N):
            if not mask[cy][cx]:
                continue
            x0 = cx * GRID_STEP
            y0 = cy * GRID_STEP
            x1 = x0 + GRID_STEP - 1
            y1 = y0 + GRID_STEP - 1
            canvas.create_rectangle(x0, y0, x1, y1, fill=FILL_COL, outline="", tags="shape")

#================================================================================================================================#
#=> - GUI -
#================================================================================================================================#

class SmallShapeMk1App:
    def __init__(self, root):
        self._root = root
        self._root.title("small shape mk1")
        self._lim_x = tk.IntVar(value=SLIDER_DEF)
        self._lim_y = tk.IntVar(value=SLIDER_DEF)
        self._last_gx = None
        self._last_gy = None
        self._build_ui()
        self._draw_grid()

    def _build_ui(self):
        frm = ttk.Frame(self._root, padding=8)
        frm.pack(fill=tk.BOTH, expand=True)
        row0 = ttk.Frame(frm)
        row0.pack(fill=tk.X, pady=(0, 6))
        ttk.Label(row0, text="size limit x").pack(side=tk.LEFT)
        tk.Scale(
            row0,
            from_=SLIDER_MIN,
            to=SLIDER_MAX,
            orient=tk.HORIZONTAL,
            resolution=1,
            showvalue=False,
            variable=self._lim_x,
            command=self._on_lim_change,
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(8, 0))
        ttk.Label(row0, textvariable=self._lim_x, width=3).pack(side=tk.LEFT, padx=(8, 0))
        row1 = ttk.Frame(frm)
        row1.pack(fill=tk.X, pady=(0, 6))
        ttk.Label(row1, text="size limit y").pack(side=tk.LEFT)
        tk.Scale(
            row1,
            from_=SLIDER_MIN,
            to=SLIDER_MAX,
            orient=tk.HORIZONTAL,
            resolution=1,
            showvalue=False,
            variable=self._lim_y,
            command=self._on_lim_change,
        ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(8, 0))
        ttk.Label(row1, textvariable=self._lim_y, width=3).pack(side=tk.LEFT, padx=(8, 0))
        row2 = ttk.Frame(frm)
        row2.pack(fill=tk.BOTH, expand=True)
        self._cvs = tk.Canvas(
            row2,
            width=CANVAS_PX,
            height=CANVAS_PX,
            bg=BG_COL,
            highlightthickness=1,
            highlightbackground="#808080",
        )
        self._cvs.pack()
        self._cvs.bind("<Button-1>", self._on_click)

    def _draw_grid(self):
        self._cvs.delete("grid")
        self._cvs.delete("limit")
        for x in range(0, CANVAS_PX, GRID_STEP):
            self._cvs.create_line(x, 0, x, CANVAS_PX - 1, fill=GRID_COL, tags="grid")
        for y in range(0, CANVAS_PX, GRID_STEP):
            self._cvs.create_line(0, y, CANVAS_PX - 1, y, fill=GRID_COL, tags="grid")
        self._draw_limit()

    def _draw_limit(self):
        if self._last_gx is None or self._last_gy is None:
            return
        cx = self._last_gx * GRID_STEP + GRID_STEP // 2
        cy = self._last_gy * GRID_STEP + GRID_STEP // 2
        rx = int(self._lim_x.get()) * GRID_STEP
        ry = int(self._lim_y.get()) * GRID_STEP
        self._cvs.create_oval(
            cx - rx, cy - ry, cx + rx, cy + ry,
            outline=LIM_COL, width=1, tags="limit",
        )

    def _regen_shape(self):
        self._cvs.delete("shape")
        if self._last_gx is None or self._last_gy is None:
            return
        px = self._last_gx * GRID_STEP + GRID_STEP // 2
        py = self._last_gy * GRID_STEP + GRID_STEP // 2
        generate_shape(
            self._cvs,
            int(self._lim_x.get()),
            int(self._lim_y.get()),
            px,
            py,
        )

    def _on_lim_change(self, _val):
        self._draw_grid()
        self._regen_shape()

    def _on_click(self, evt):
        self._last_gx = max(0, min(GRID_N - 1, evt.x // GRID_STEP))
        self._last_gy = max(0, min(GRID_N - 1, evt.y // GRID_STEP))
        self._draw_grid()
        self._regen_shape()

def run_app(argv):
    root = tk.Tk()
    SmallShapeMk1App(root)
    root.mainloop()
    return 0

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    raise SystemExit(run_app(sys.argv[1:]))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
