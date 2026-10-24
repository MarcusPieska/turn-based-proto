#!/usr/bin/env python3
import os
import re
import tkinter as tk
from tkinter import ttk

N = 10
HALF = N // 2
CELL = 44
CANVAS = N * CELL
MK5 = os.path.dirname(os.path.abspath(__file__))
MK6 = os.path.join(os.path.dirname(MK5), "blend_mk6")

BANKS = (
    ("corner", "corner"),
    ("hsplit", "hsplit"),
    ("tcross", "tcross"),
    ("diag", "diag"),
    ("quad", "quad"),
)

COL = ["#3a9a40", "#2a5ccc", "#c47a20", "#9a3a8a"]
COL_GRID = "#303030"
COL_CROSS = "#101010"
COL_BG = "#e8e8e0"
COL_X = "#101010"

QUADS = {
    "NW": (0, 0, HALF - 1, HALF - 1),
    "NE": (0, HALF, HALF - 1, N - 1),
    "SW": (HALF, 0, N - 1, HALF - 1),
    "SE": (HALF, HALF, N - 1, N - 1),
}


def base_of(v):
    return v - 4 if v >= 4 else v


def is_beach(v):
    return v >= 4


class PreEditBeach(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk5 pre_edit_beach")
        self.resizable(False, False)

        self.bank_key = BANKS[0][0]
        self.bank_prefix = BANKS[0][1]
        self.cells = [[0] * N for _ in range(N)]
        self.cur_idx = None
        self._build()
        self._goto_first()

    def _src_dir(self):
        return os.path.join(MK5, "%s_bank" % self.bank_key)

    def _dst_dir(self):
        return os.path.join(MK6, "beach_%s_bank" % self.bank_key)

    def _file_re(self):
        return re.compile(r"^%s-(\d+)\.data$" % re.escape(self.bank_prefix))

    def _path_src(self, idx):
        return os.path.join(self._src_dir(), "%s-%03d.data" % (self.bank_prefix, idx))

    def _path_dst(self, idx):
        return os.path.join(self._dst_dir(), "%s-%03d.data" % (self.bank_prefix, idx))

    def _build(self):
        bar = ttk.Frame(self, padding=6)
        bar.pack(side=tk.TOP, fill=tk.X)
        ttk.Label(bar, text="bank").pack(side=tk.LEFT)
        self.bank_var = tk.StringVar(value=self.bank_key)
        cb = ttk.Combobox(
            bar,
            textvariable=self.bank_var,
            values=[b[0] for b in BANKS],
            width=10,
            state="readonly",
        )
        cb.pack(side=tk.LEFT, padx=4)
        cb.bind("<<ComboboxSelected>>", self._on_bank)
        ttk.Button(bar, text="Save", command=self._save).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Prev", command=lambda: self._cycle(-1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Next", command=lambda: self._cycle(1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Reload src", command=self._reload_src).pack(side=tk.LEFT, padx=2)
        self.status = ttk.Label(bar, text="")
        self.status.pack(side=tk.LEFT, padx=10)

        self.cv = tk.Canvas(self, width=CANVAS, height=CANVAS, bg=COL_BG, highlightthickness=1)
        self.cv.pack(side=tk.TOP)
        self.cv.bind("<Button-1>", self._click)

        tip = ttk.Label(
            self,
            text="colors locked to base slot N  |  click toggles beach N↔N+4 (X)  |  save → mk6/beach_*_bank then next",
            padding=6,
        )
        tip.pack(side=tk.TOP, fill=tk.X)

    def _on_bank(self, _e=None):
        key = self.bank_var.get()
        for k, p in BANKS:
            if k == key:
                self.bank_key = k
                self.bank_prefix = p
                break
        self._goto_first()

    def _list_indices(self):
        out = []
        src = self._src_dir()
        if not os.path.isdir(src):
            return out
        cre = self._file_re()
        for name in os.listdir(src):
            m = cre.match(name)
            if m:
                out.append(int(m.group(1)))
        return sorted(out)

    def _read_cells(self, path):
        cells = []
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                if len(line) != N or any(ch not in "01234567" for ch in line):
                    continue
                cells.append([int(ch) for ch in line])
        if len(cells) != N:
            return None
        return cells

    def _goto_first(self):
        ids = self._list_indices()
        if not ids:
            self.cells = [[0] * N for _ in range(N)]
            self.cur_idx = None
            self._set_status()
            self._redraw()
            return
        self._load(ids[0])

    def _load(self, idx):
        dst = self._path_dst(idx)
        src = self._path_src(idx)
        path = dst if os.path.isfile(dst) else src
        cells = self._read_cells(path) if os.path.isfile(path) else None
        if cells is None:
            self.cells = [[0] * N for _ in range(N)]
            self.cur_idx = None
            self._set_status()
            self._redraw()
            return
        self.cells = cells
        self.cur_idx = idx
        self._set_status()
        self._redraw()

    def _reload_src(self):
        if self.cur_idx is None:
            return
        cells = self._read_cells(self._path_src(self.cur_idx))
        if cells is None:
            return
        self.cells = cells
        self._set_status()
        self._redraw()

    def _set_status(self):
        uplift = sum(1 for r in range(N) for c in range(N) if is_beach(self.cells[r][c]))
        dst = self._dst_dir()
        tag = "dst" if (self.cur_idx is not None and os.path.isfile(self._path_dst(self.cur_idx))) else "src"
        if self.cur_idx is None:
            self.status.config(text="%s  (no patterns)" % self.bank_key)
        else:
            self.status.config(
                text="%s-%03d  %s  beach=%d  → %s"
                % (self.bank_prefix, self.cur_idx, tag, uplift, dst)
            )

    def _save(self):
        if self.cur_idx is None:
            return
        os.makedirs(self._dst_dir(), exist_ok=True)
        path = self._path_dst(self.cur_idx)
        with open(path, "w") as f:
            f.write("# mk6 beach pattern  0-3=slot  4-7=beach-if-water(slot N-4)\n")
            f.write("# derived from mk5 %s-%03d  |  click toggles N↔N+4\n" % (self.bank_prefix, self.cur_idx))
            for r in range(N):
                f.write("".join(str(self.cells[r][c]) for c in range(N)) + "\n")
        self._cycle(1)

    def _cycle(self, step):
        ids = self._list_indices()
        if not ids:
            self.cur_idx = None
            self._set_status()
            self._redraw()
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
        v = self.cells[sr][sc]
        b = base_of(v)
        self.cells[sr][sc] = b if is_beach(v) else b + 4
        self._redraw()

    def _redraw(self):
        self.cv.delete("all")
        for sr in range(N):
            for sc in range(N):
                x0 = sc * CELL
                y0 = sr * CELL
                v = self.cells[sr][sc]
                b = base_of(v)
                col = COL[b % 4]
                self.cv.create_rectangle(x0, y0, x0 + CELL, y0 + CELL, fill=col, outline=COL_GRID)
                if is_beach(v):
                    pad = 8
                    self.cv.create_line(
                        x0 + pad, y0 + pad, x0 + CELL - pad, y0 + CELL - pad, fill=COL_X, width=3
                    )
                    self.cv.create_line(
                        x0 + CELL - pad, y0 + pad, x0 + pad, y0 + CELL - pad, fill=COL_X, width=3
                    )
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
    app = PreEditBeach()
    app.mainloop()


if __name__ == "__main__":
    main()
