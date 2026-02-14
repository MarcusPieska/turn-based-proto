#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
from tkinter import filedialog
import math
import random

#================================================================================================================================#
#=> - Globals -
#================================================================================================================================#

CANV_W = 800
CANV_H = 800
CELL_PX = 5
GRID_CELLS = 100

TILE_EMPTY      = 0
TILE_TOWN_SQ    = 1
TILE_BUILDING   = 2
TILE_ROAD       = 3
TILE_MAIN_ROAD  = 4

#================================================================================================================================#
#=> - Class: CityTile
#================================================================================================================================#

class CityTile:

    def __init__ (m, t=TILE_EMPTY, bid=-1):
        m.t = t
        m.bid = bid

#================================================================================================================================#
#=> - Class: CityGrid
#================================================================================================================================#

class CityGrid:

    def __init__ (m, w=GRID_CELLS, h=GRID_CELLS):
        m.w = w
        m.h = h
        m.data = [[CityTile() for _ in range(w)] for _ in range(h)]

    def __in_bounds (m, x, y):
        return 0 <= x < m.w and 0 <= y < m.h

    def get (m, x, y):
        if not m.__in_bounds(x, y):
            return None
        return m.data[y][x]

    def set (m, x, y, t, bid=-1):
        if not m.__in_bounds(x, y):
            return
        tile = m.data[y][x]
        tile.t = t
        tile.bid = bid

#================================================================================================================================#
#=> - Isometric Projection
#================================================================================================================================#

def to_iso (px, py):
    angle = math.radians(45)
    cos_a = math.cos(angle)
    sin_a = math.sin(angle)
    rx = px * cos_a - py * sin_a
    ry = px * sin_a + py * cos_a
    ry = ry // 2
    return rx, ry

def draw_iso_buildings (cnv, g, bid_colors):
    cnv.delete("buildings")
    cx, cy = CANV_W // 2, CANV_H // 2
    for y in range(g.h):
        for x in range(g.w):
            tile = g.get(x, y)
            if tile.t != TILE_BUILDING or tile.bid < 0:
                continue
            bid = tile.bid
            color = bid_colors.get(bid, "#666666")
            x0 = x * CELL_PX
            y0 = y * CELL_PX
            x1 = (x + 1) * CELL_PX
            y1 = (y + 1) * CELL_PX
            corners = [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]
            iso_pts = []
            for px, py in corners:
                ix, iy = to_iso(px, py)
                iso_pts.append(cx + ix)
                iso_pts.append(cy + iy)
            cnv.create_polygon(iso_pts, fill=color, width=2, tags="buildings")

#================================================================================================================================#
#=> - Class: IsoViewer
#================================================================================================================================#

class IsoViewer:

    def __drawScene (m):
        m.canv_main.delete("all")
        if m.grid is None:
            return
        draw_iso_buildings(m.canv_main, m.grid, m.bid_colors)

    def __onLoad (m):
        fpath = filedialog.askopenfilename(filetypes=[("Text files", "*.txt"), ("All files", "*.*")])
        if not fpath:
            return
        try:
            g = CityGrid()
            m.bid_colors.clear()
            with open(fpath, "r") as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue
                    parts = line.split(":")
                    if len(parts) != 4:
                        continue
                    y = int(parts[0])
                    x = int(parts[1])
                    t = int(parts[2])
                    bid = int(parts[3])
                    if 0 <= x < g.w and 0 <= y < g.h:
                        g.set(x, y, t, bid)
                        if t == TILE_BUILDING and bid >= 0 and bid not in m.bid_colors:
                            r = random.randint(80, 220)
                            gcol = random.randint(80, 220)
                            b = random.randint(80, 220)
                            m.bid_colors[bid] = "#%02x%02x%02x" % (r, gcol, b)
            m.grid = g
            m.__drawScene()
        except Exception as e:
            print("Load error:", e)

    def __init__ (m, root):
        m.root = root

        m.canv_w = CANV_W
        m.canv_h = CANV_H

        m.grid = None
        m.bid_colors = {}

        m.ctrl_frm = tk.Frame(root)
        m.ctrl_frm.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        m.load_btn = tk.Button(m.ctrl_frm, text="Load", command=m.__onLoad, width=12)
        m.load_btn.pack(side=tk.LEFT, padx=5)

        m.canv_frm = tk.Frame(root)
        m.canv_frm.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_main = tk.Canvas(m.canv_frm, width=m.canv_w, height=m.canv_h, bg="#ffffff")
        m.canv_main.pack(side=tk.LEFT, padx=5, pady=5)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Isometric City Projection")
    viewer = IsoViewer(root)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
