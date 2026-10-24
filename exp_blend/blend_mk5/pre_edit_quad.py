#!/usr/bin/env python3
import os
import re
import shutil
import tkinter as tk
from tkinter import ttk

N = 10
HALF = N // 2
CELL = 44
CANVAS = N * CELL
DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "quad_bank")
TRASH_DIR = "/home/w/Projects/trash"
FILE_RE = re.compile(r"^quad-(\d+)\.data$")

COL = ["#3a9a40", "#2a5ccc", "#c47a20", "#9a3a8a"]
COL_GRID = "#303030"
COL_CROSS = "#101010"
COL_BG = "#e8e8e0"

QUADS = {
    "NW": (0, 0, HALF - 1, HALF - 1),
    "NE": (0, HALF, HALF - 1, N - 1),
    "SW": (HALF, 0, N - 1, HALF - 1),
    "SE": (HALF, HALF, N - 1, N - 1),
}


class PreEditQuad(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk5 pre_edit_quad")
        self.resizable(False, False)
        os.makedirs(DATA_DIR, exist_ok=True)
        os.makedirs(TRASH_DIR, exist_ok=True)

        self.cells = [[0] * N for _ in range(N)]
        self.cur_idx = None
        self._build()
        self._new_draft()
        self._redraw()

    def _build(self):
        bar = ttk.Frame(self, padding=6)
        bar.pack(side=tk.TOP, fill=tk.X)
        ttk.Button(bar, text="Save", command=self._save).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Delete", command=self._delete).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Prev", command=lambda: self._cycle(-1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Next", command=lambda: self._cycle(1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Reset", command=self._new_draft).pack(side=tk.LEFT, padx=2)
        self.status = ttk.Label(bar, text="")
        self.status.pack(side=tk.LEFT, padx=10)

        self.cv = tk.Canvas(self, width=CANVAS, height=CANVAS, bg=COL_BG, highlightthickness=1)
        self.cv.pack(side=tk.TOP)
        self.cv.bind("<Button-1>", self._click)

        tip = ttk.Label(
            self,
            text="quad fixed: NW=0  NE=1  SW=2  SE=3  |  click cycles 0/1/2/3  |  permute at use time",
            padding=6,
        )
        tip.pack(side=tk.TOP, fill=tk.X)

    def _fill_solid(self):
        for sr in range(N):
            for sc in range(N):
                if sr < HALF and sc < HALF:
                    self.cells[sr][sc] = 0
                elif sr < HALF and sc >= HALF:
                    self.cells[sr][sc] = 1
                elif sr >= HALF and sc < HALF:
                    self.cells[sr][sc] = 2
                else:
                    self.cells[sr][sc] = 3

    def _new_draft(self):
        self._fill_solid()
        self.cur_idx = None
        self._set_status()
        self._redraw()

    def _list_indices(self):
        out = []
        for name in os.listdir(DATA_DIR):
            m = FILE_RE.match(name)
            if m:
                out.append(int(m.group(1)))
        return sorted(out)

    def _path_for(self, idx):
        return os.path.join(DATA_DIR, "quad-%03d.data" % idx)

    def _lowest_free(self):
        have = set(self._list_indices())
        i = 0
        while i in have:
            i += 1
        return i

    def _set_status(self):
        counts = [0, 0, 0, 0]
        for r in range(N):
            for c in range(N):
                counts[self.cells[r][c]] += 1
        if self.cur_idx is None:
            self.status.config(text="draft  0/1/2/3=%d/%d/%d/%d" % tuple(counts))
        else:
            self.status.config(text="quad-%03d.data  0/1/2/3=%d/%d/%d/%d" % ((self.cur_idx,) + tuple(counts)))

    def _load(self, idx):
        path = self._path_for(idx)
        cells = []
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                if len(line) != N or any(ch not in "0123" for ch in line):
                    continue
                cells.append([int(ch) for ch in line])
        if len(cells) != N:
            self._new_draft()
            return
        self.cells = cells
        self.cur_idx = idx
        self._set_status()
        self._redraw()

    def _save(self):
        idx = self._lowest_free()
        path = self._path_for(idx)
        with open(path, "w") as f:
            f.write("# mk5 quad pattern  NW=0  NE=1  SW=2  SE=3\n")
            f.write("# 10x10 subtilles at the shared corner (four 5x5 tile quadrants)\n")
            for r in range(N):
                f.write("".join(str(self.cells[r][c]) for c in range(N)) + "\n")
        self.cur_idx = idx
        self._set_status()
        self._new_draft()

    def _delete(self):
        if self.cur_idx is None:
            self._new_draft()
            return
        src = self._path_for(self.cur_idx)
        if not os.path.isfile(src):
            self._new_draft()
            return
        base = "quad-%03d.data" % self.cur_idx
        dst = os.path.join(TRASH_DIR, base)
        if os.path.exists(dst):
            n = 1
            while True:
                dst = os.path.join(TRASH_DIR, "quad-%03d.%d.data" % (self.cur_idx, n))
                if not os.path.exists(dst):
                    break
                n += 1
        shutil.move(src, dst)
        self._new_draft()

    def _cycle(self, step):
        ids = self._list_indices()
        if not ids:
            self._new_draft()
            return
        if self.cur_idx is None or self.cur_idx not in ids:
            pick = ids[0] if step > 0 else ids[-1]
        else:
            i = ids.index(self.cur_idx)
            pick = ids[(i + step) % len(ids)]
        self._load(pick)

    def _click(self, e):
        sc = e.x // CELL
        sr = e.y // CELL
        if sr < 0 or sc < 0 or sr >= N or sc >= N:
            return
        self.cells[sr][sc] = (self.cells[sr][sc] + 1) % 4
        self.cur_idx = None
        self._redraw()

    def _redraw(self):
        self.cv.delete("all")
        for sr in range(N):
            for sc in range(N):
                x0 = sc * CELL
                y0 = sr * CELL
                col = COL[self.cells[sr][sc]]
                self.cv.create_rectangle(x0, y0, x0 + CELL, y0 + CELL, fill=col, outline=COL_GRID)
        mid = HALF * CELL
        self.cv.create_line(mid, 0, mid, CANVAS, fill=COL_CROSS, width=2)
        self.cv.create_line(0, mid, CANVAS, mid, fill=COL_CROSS, width=2)
        self.cv.create_oval(mid - 4, mid - 4, mid + 4, mid + 4, fill=COL_CROSS, outline=COL_CROSS)
        for name, (r0, c0, r1, c1) in QUADS.items():
            cx = (c0 + c1 + 1) * CELL // 2
            cy = (r0 + r1 + 1) * CELL // 2
            self.cv.create_text(cx, cy, text=name, fill="#f8f8f8", font=("TkDefaultFont", 10, "bold"))
        self._set_status()


def main():
    app = PreEditQuad()
    app.mainloop()


if __name__ == "__main__":
    main()
