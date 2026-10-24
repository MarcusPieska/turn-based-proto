#!/usr/bin/env python3
import os
import re
import shutil
import tkinter as tk
from tkinter import ttk

CANVAS_W = 500
CANVAS_H = 500
MID_Y = CANVAS_H // 2
HIT_PX = 10
VERT_R = 5
DATA_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "pre_bank")
TRASH_DIR = "/home/w/Projects/trash"
FILE_RE = re.compile(r"^threading-(\d+)\.data$")


class PreEdit(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk3 pre_edit")
        self.resizable(False, False)
        os.makedirs(DATA_DIR, exist_ok=True)
        os.makedirs(TRASH_DIR, exist_ok=True)

        self.pts = []
        self.cur_idx = None
        self.drag_i = None
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
        self.status = ttk.Label(bar, text="")
        self.status.pack(side=tk.LEFT, padx=10)

        self.cv = tk.Canvas(self, width=CANVAS_W, height=CANVAS_H, bg="#f0f0e8", highlightthickness=1)
        self.cv.pack(side=tk.TOP)
        self.cv.bind("<Button-1>", self._left_down)
        self.cv.bind("<B1-Motion>", self._left_drag)
        self.cv.bind("<ButtonRelease-1>", self._left_up)
        self.cv.bind("<Button-3>", self._right_down)

    def _locked(self, i):
        return i == 0 or i == len(self.pts) - 1

    def _new_draft(self):
        self.pts = [(0, MID_Y), (CANVAS_W - 1, MID_Y)]
        self.cur_idx = None
        self._set_status()

    def _list_indices(self):
        out = []
        for name in os.listdir(DATA_DIR):
            m = FILE_RE.match(name)
            if m:
                out.append(int(m.group(1)))
        return sorted(out)

    def _path_for(self, idx):
        return os.path.join(DATA_DIR, "threading-%03d.data" % idx)

    def _lowest_free(self):
        have = set(self._list_indices())
        i = 0
        while i in have:
            i += 1
        return i

    def _set_status(self):
        if self.cur_idx is None:
            self.status.config(text="draft (unsaved)  pts=%d" % len(self.pts))
        else:
            self.status.config(text="threading-%03d.data  pts=%d" % (self.cur_idx, len(self.pts)))

    def _load(self, idx):
        path = self._path_for(idx)
        pts = []
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                a, b = line.split()
                pts.append((float(a), float(b)))
        if len(pts) < 2:
            pts = [(0, MID_Y), (CANVAS_W - 1, MID_Y)]
        pts[0] = (0, MID_Y)
        pts[-1] = (CANVAS_W - 1, MID_Y)
        self.pts = pts
        self.cur_idx = idx
        self._set_status()
        self._redraw()

    def _save(self):
        idx = self._lowest_free()
        path = self._path_for(idx)
        with open(path, "w") as f:
            f.write("# mk3 manual threading\n")
            for x, y in self.pts:
                f.write("%g %g\n" % (x, y))
        self.cur_idx = idx
        self._set_status()
        self._new_draft()
        self._redraw()

    def _delete(self):
        if self.cur_idx is None:
            self._new_draft()
            self._redraw()
            return
        src = self._path_for(self.cur_idx)
        if not os.path.isfile(src):
            self._new_draft()
            self._redraw()
            return
        base = "threading-%03d.data" % self.cur_idx
        dst = os.path.join(TRASH_DIR, base)
        if os.path.exists(dst):
            n = 1
            while True:
                dst = os.path.join(TRASH_DIR, "threading-%03d.%d.data" % (self.cur_idx, n))
                if not os.path.exists(dst):
                    break
                n += 1
        shutil.move(src, dst)
        self._new_draft()
        self._redraw()

    def _cycle(self, step):
        ids = self._list_indices()
        if not ids:
            self._new_draft()
            self._redraw()
            return
        if self.cur_idx is None or self.cur_idx not in ids:
            pick = ids[0] if step > 0 else ids[-1]
        else:
            i = ids.index(self.cur_idx)
            pick = ids[(i + step) % len(ids)]
        self._load(pick)

    def _hit(self, x, y):
        best_i = None
        best_d = HIT_PX + 1
        for i, (px, py) in enumerate(self.pts):
            d = ((px - x) ** 2 + (py - y) ** 2) ** 0.5
            if d <= HIT_PX and d < best_d:
                best_d = d
                best_i = i
        return best_i

    def _nearest_seg(self, x, y):
        best_i = 0
        best_d = 1e30
        for i in range(len(self.pts) - 1):
            x0, y0 = self.pts[i]
            x1, y1 = self.pts[i + 1]
            dx, dy = x1 - x0, y1 - y0
            L2 = dx * dx + dy * dy
            if L2 <= 1e-9:
                t = 0.0
            else:
                t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / L2))
            qx, qy = x0 + t * dx, y0 + t * dy
            d = (qx - x) ** 2 + (qy - y) ** 2
            if d < best_d:
                best_d = d
                best_i = i
        return best_i

    def _left_down(self, e):
        i = self._hit(e.x, e.y)
        if i is not None:
            if not self._locked(i):
                self.drag_i = i
            return
        j = self._nearest_seg(e.x, e.y) + 1
        self.pts.insert(j, (float(e.x), float(e.y)))
        self.drag_i = j
        self._redraw()

    def _left_drag(self, e):
        if self.drag_i is None or self._locked(self.drag_i):
            return
        x = max(0, min(CANVAS_W - 1, e.x))
        y = max(0, min(CANVAS_H - 1, e.y))
        self.pts[self.drag_i] = (float(x), float(y))
        self._redraw()

    def _left_up(self, _e):
        self.drag_i = None

    def _right_down(self, e):
        i = self._hit(e.x, e.y)
        if i is None or self._locked(i):
            return
        del self.pts[i]
        self._redraw()

    def _redraw(self):
        self.cv.delete("all")
        self.cv.create_line(0, MID_Y, CANVAS_W, MID_Y, fill="#cccccc", dash=(4, 4))
        if len(self.pts) >= 2:
            flat = []
            for x, y in self.pts:
                flat.extend([x, y])
            self.cv.create_line(*flat, fill="#b41e28", width=2)
        for i, (x, y) in enumerate(self.pts):
            col = "#404040" if self._locked(i) else "#1414dc"
            self.cv.create_oval(x - VERT_R, y - VERT_R, x + VERT_R, y + VERT_R, fill=col, outline=col)
        self._set_status()


def main():
    app = PreEdit()
    app.mainloop()


if __name__ == "__main__":
    main()
