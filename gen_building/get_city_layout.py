#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
from tkinter import filedialog
import random

#================================================================================================================================#
#=> - Globals -
#================================================================================================================================#

CANV_W = 600
CANV_H = 600
GRID_PX = 500
CELL_PX = 5
GRID_CELLS = GRID_PX // CELL_PX
GRID_OX = (CANV_W - GRID_PX) // 2
GRID_OY = (CANV_H - GRID_PX) // 2

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
#=> - Drawing Helpers -
#================================================================================================================================#

def draw_grid_lines (cnv):
    for i in range(GRID_CELLS + 1):
        x = GRID_OX + i * CELL_PX
        y0 = GRID_OY
        y1 = GRID_OY + GRID_PX
        cnv.create_line(x, y0, x, y1, fill="#d0d0d0", tags="grid")
    for j in range(GRID_CELLS + 1):
        y = GRID_OY + j * CELL_PX
        x0 = GRID_OX
        x1 = GRID_OX + GRID_PX
        cnv.create_line(x0, y, x1, y, fill="#d0d0d0", tags="grid")

def draw_tiles (cnv, g, mode=1, bid_colors=None):
    cnv.delete("tiles")
    for y in range(g.h):
        for x in range(g.w):
            tile = g.data[y][x]
            if mode == 2 and tile.t != TILE_BUILDING:
                continue
            px = GRID_OX + x * CELL_PX
            py = GRID_OY + y * CELL_PX
            if mode == 2 and tile.t == TILE_BUILDING and bid_colors is not None:
                fill = bid_colors.get(tile.bid, "#666666")
            else:
                if tile.t == TILE_TOWN_SQ:
                    fill = "#f4d29c"
                elif tile.t == TILE_BUILDING:
                    fill = "#666666"
                elif tile.t == TILE_ROAD:
                    fill = "#bbbbbb"
                elif tile.t == TILE_MAIN_ROAD:
                    fill = "#f4f06a"
                else:
                    fill = "#000000"
            cnv.create_rectangle(px, py, px + CELL_PX, py + CELL_PX, outline="", fill=fill, tags="tiles")

#================================================================================================================================#
#=> - Generation: Step 1 (Town Square + Main Roads)
#================================================================================================================================#

def _find_square_bounds (g):
    x0, y0 = g.w, g.h
    x1, y1 = -1, -1
    for y in range(g.h):
        for x in range(g.w):
            if g.get(x, y).t == TILE_TOWN_SQ:
                if x < x0:
                    x0 = x
                if y < y0:
                    y0 = y
                if x > x1:
                    x1 = x
                if y > y1:
                    y1 = y
    if x1 == -1 or y1 == -1:
        return None, None, None, None
    return x0, y0, x1 + 1, y1 + 1

def make_step1_grid (sq_w, sq_h):
    g = CityGrid()
    cw = g.w // 2
    ch = g.h // 2
    w = max(4, min(g.w - 4, int(sq_w)))
    h = max(4, min(g.h - 4, int(sq_h)))
    x0 = max(0, cw - w // 2)
    y0 = max(0, ch - h // 2)
    x1 = min(g.w, x0 + w)
    y1 = min(g.h, y0 + h)
    for y in range(y0, y1):
        for x in range(x0, x1):
            g.set(x, y, TILE_TOWN_SQ)
    rw = 2
    mx = (x0 + x1) // 2
    my = (y0 + y1) // 2
    for y in range(0, y0):
        for dx in range(-rw // 2, rw // 2 + 1):
            x = mx + dx
            if 0 <= x < g.w and g.get(x, y).t == TILE_EMPTY:
                g.set(x, y, TILE_MAIN_ROAD)
    for y in range(y1, g.h):
        for dx in range(-rw // 2, rw // 2 + 1):
            x = mx + dx
            if 0 <= x < g.w and g.get(x, y).t == TILE_EMPTY:
                g.set(x, y, TILE_MAIN_ROAD)
    for x in range(0, x0):
        for dy in range(-rw // 2, rw // 2 + 1):
            y = my + dy
            if 0 <= y < g.h and g.get(x, y).t == TILE_EMPTY:
                g.set(x, y, TILE_MAIN_ROAD)
    for x in range(x1, g.w):
        for dy in range(-rw // 2, rw // 2 + 1):
            y = my + dy
            if 0 <= y < g.h and g.get(x, y).t == TILE_EMPTY:
                g.set(x, y, TILE_MAIN_ROAD)
    return g

#================================================================================================================================#
#=> - Generation: Step 2 (Blocks Along Main Roads)
#================================================================================================================================#

def _region_empty (g, x0, y0, x1, y1):
    if x0 < 0 or y0 < 0 or x1 > g.w or y1 > g.h:
        return False
    for y in range(y0, y1):
        for x in range(x0, x1):
            if g.get(x, y).t != TILE_EMPTY:
                return False
    return True

def _fill_block_building (g, x0, y0, bw, bh, bid):
    for y in range(y0, y0 + bh):
        for x in range(x0, x0 + bw):
            g.set(x, y, TILE_BUILDING, bid)

def _add_road_ring (g, x0, y0, bw, bh):
    rx0 = max(0, x0 - 1)
    ry0 = max(0, y0 - 1)
    rx1 = min(g.w, x0 + bw + 1)
    ry1 = min(g.h, y0 + bh + 1)
    for y in range(ry0, ry1):
        for x in range(rx0, rx1):
            if x >= x0 and x < x0 + bw and y >= y0 and y < y0 + bh:
                continue
            tile = g.get(x, y)
            if tile.t == TILE_EMPTY:
                g.set(x, y, TILE_ROAD)

def _adjacent_non_empty_count (g, x0, y0, bw, bh):
    rx0 = max(0, x0 - 1)
    ry0 = max(0, y0 - 1)
    rx1 = min(g.w, x0 + bw + 1)
    ry1 = min(g.h, y0 + bh + 1)
    c = 0
    for y in range(ry0, ry1):
        for x in range(rx0, rx1):
            if x >= x0 and x < x0 + bw and y >= y0 and y < y0 + bh:
                continue
            if g.get(x, y).t != TILE_EMPTY:
                c += 1
    return c

def _rect_adjacent_to_main_road (g, x0, y0, bw, bh):
    rx0 = max(0, x0 - 1)
    ry0 = max(0, y0 - 1)
    rx1 = min(g.w, x0 + bw + 1)
    ry1 = min(g.h, y0 + bh + 1)
    for y in range(ry0, ry1):
        for x in range(rx0, rx1):
            if x >= x0 and x < x0 + bw and y >= y0 and y < y0 + bh:
                continue
            if g.get(x, y).t == TILE_MAIN_ROAD:
                return True
    return False

#================================================================================================================================#
#=> - Generation: Step 2 (Blocks Along Main Roads)
#================================================================================================================================#

def _place_blocks (g, x0, y0, x1, y1, require_main):
    bw_opts = [(8, 8), (8, 12), (12, 8), (12, 12)]
    bid = 1
    while True:
        bw, bh = random.choice(bw_opts)
        print("try block", bw, bh)
        best = None
        best_score = -1
        for y in range(0, g.h - bh + 1):
            for x in range(0, g.w - bw + 1):
                if not _region_empty(g, x, y, x + bw, y + bh):
                    continue
                if require_main and not _rect_adjacent_to_main_road(g, x, y, bw, bh):
                    continue
                score = _adjacent_non_empty_count(g, x, y, bw, bh)
                if score > best_score:
                    best_score = score
                    best = (x, y)
        if best is None or best_score <= 0:
            print("no place for block", bw, bh, "stop")
            break
        bx, by = best
        print("place block", bw, bh, "score", best_score, "at", bx, by)
        _fill_block_building(g, bx, by, bw, bh, bid)
        _add_road_ring(g, bx, by, bw, bh)
        bid += 1

def apply_step2_blocks (g):
    x0, y0, x1, y1 = _find_square_bounds(g)
    if x0 is None:
        return g
    _place_blocks(g, x0, y0, x1, y1, True)
    return g

#================================================================================================================================#
#=> - Generation: Step 3 (Fill Remaining Blocks)
#================================================================================================================================#

def apply_step3_blocks (g):
    x0, y0, x1, y1 = _find_square_bounds(g)
    if x0 is None:
        return g
    _place_blocks(g, x0, y0, x1, y1, False)
    return g

#================================================================================================================================#
#=> - Generation: Step 4 (Central Infill Blocks)
#================================================================================================================================#

def apply_step4_infill (g):
    bid = 1
    for y in range(g.h):
        for x in range(g.w):
            t = g.get(x, y)
            if t.bid >= bid:
                bid = t.bid + 1
    vx = [[False for _ in range(g.w)] for _ in range(g.h)]
    ix0 = g.w // 4
    ix1 = g.w - ix0
    iy0 = g.h // 4
    iy1 = g.h - iy0
    for sy in range(iy0, iy1):
        for sx in range(ix0, ix1):
            if vx[sy][sx]:
                continue
            if g.get(sx, sy).t != TILE_EMPTY:
                continue
            stack = [(sx, sy)]
            pts = []
            vx[sy][sx] = True
            while stack:
                x, y = stack.pop()
                pts.append((x, y))
                for dx, dy in ((1,0),(-1,0),(0,1),(0,-1)):
                    nx = x + dx
                    ny = y + dy
                    if nx < ix0 or nx >= ix1 or ny < iy0 or ny >= iy1:
                        continue
                    if vx[ny][nx]:
                        continue
                    if g.get(nx, ny).t != TILE_EMPTY:
                        continue
                    vx[ny][nx] = True
                    stack.append((nx, ny))
            if not pts:
                continue
            min_x = min(p[0] for p in pts)
            max_x = max(p[0] for p in pts)
            min_y = min(p[1] for p in pts)
            max_y = max(p[1] for p in pts)
            w = max_x - min_x + 1
            h = max_y - min_y + 1
            if w < 2 or h < 2:
                continue
            for x, y in pts:
                g.set(x, y, TILE_BUILDING, bid)
            bid += 1
    return g

#================================================================================================================================#
#=> - Class: CityViewer
#================================================================================================================================#

class CityViewer:

    def __drawScene (m):
        m.canv_main.delete("grid")
        draw_grid_lines(m.canv_main)
        mode = m.draw_mode.get()
        draw_tiles(m.canv_main, m.grid, mode=mode, bid_colors=m.bid_colors)

    def __onGenerate (m):
        sq_w = m.sq_w.get()
        sq_h = m.sq_h.get()
        g = make_step1_grid(sq_w, sq_h)
        apply_step2_blocks(g)
        apply_step3_blocks(g)
        apply_step4_infill(g)
        m.bid_colors.clear()
        for y in range(g.h):
            for x in range(g.w):
                t = g.get(x, y)
                if t.t == TILE_BUILDING and t.bid not in m.bid_colors:
                    r = random.randint(80, 220)
                    gcol = random.randint(80, 220)
                    b = random.randint(80, 220)
                    m.bid_colors[t.bid] = "#%02x%02x%02x" % (r, gcol, b)
        m.grid = g
        m.__drawScene ()

    def __onSave (m):
        fpath = filedialog.asksaveasfilename(defaultextension=".txt", filetypes=[("Text files", "*.txt"), ("All files", "*.*")])
        if not fpath:
            return
        try:
            with open(fpath, "w") as f:
                for y in range(m.grid.h):
                    for x in range(m.grid.w):
                        tile = m.grid.get(x, y)
                        f.write("%d:%d:%d:%d\n" % (y, x, tile.t, tile.bid))
        except Exception as e:
            print("Save error:", e)

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

        m.grid = CityGrid()
        m.draw_mode = tk.IntVar()
        m.draw_mode.set(1)
        m.bid_colors = {}

        m.ctrl_frm = tk.Frame(root)
        m.ctrl_frm.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        m.sq_w = tk.DoubleVar()
        m.sq_h = tk.DoubleVar()
        m.sq_w.set(40.0)
        m.sq_h.set(40.0)

        m.w_slider = tk.Scale(m.ctrl_frm, from_=10, to=80, resolution=2, orient=tk.HORIZONTAL, variable=m.sq_w)
        m.w_slider.configure(label="Square W", length=160)
        m.w_slider.pack(side=tk.LEFT, padx=5)

        m.h_slider = tk.Scale(m.ctrl_frm, from_=10, to=80, resolution=2, orient=tk.HORIZONTAL, variable=m.sq_h)
        m.h_slider.configure(label="Square H", length=160)
        m.h_slider.pack(side=tk.LEFT, padx=5)

        m.gen_btn = tk.Button(m.ctrl_frm, text="Generate", command=m.__onGenerate, width=10)
        m.gen_btn.pack(side=tk.LEFT, padx=10)

        def _toggle_mode ():
            if m.draw_mode.get() == 1:
                m.draw_mode.set(2)
            else:
                m.draw_mode.set(1)
            m.__drawScene ()

        m.mode_btn = tk.Button(m.ctrl_frm, text="Toggle Mode", command=_toggle_mode, width=12)
        m.mode_btn.pack(side=tk.LEFT, padx=5)

        m.save_btn = tk.Button(m.ctrl_frm, text="Save", command=m.__onSave, width=8)
        m.save_btn.pack(side=tk.LEFT, padx=5)

        m.load_btn = tk.Button(m.ctrl_frm, text="Load", command=m.__onLoad, width=8)
        m.load_btn.pack(side=tk.LEFT, padx=5)

        m.canv_frm = tk.Frame(root)
        m.canv_frm.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_main = tk.Canvas(m.canv_frm, width=m.canv_w, height=m.canv_h, bg="#ffffff")
        m.canv_main.pack(side=tk.LEFT, padx=5, pady=5)

        m.__onGenerate ()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("City Grid Tester")
    viewer = CityViewer (root)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
