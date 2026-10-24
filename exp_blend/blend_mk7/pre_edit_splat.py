#!/usr/bin/env python3
"""
mk7 splat bitmask editor on the super-subtile stamp (same pixels as
super_submask / super_submask_cover.png). Tips N/E/S/W are authored separately
(no rotate). Click toggles splat on stamp-on pixels only.
"""

import os
import re
import struct
import tkinter as tk
from tkinter import ttk

MK7 = os.path.dirname(os.path.abspath(__file__))
MASK_BIN = os.path.join(MK7, "dia_submask", "super_submask.bin")
CELL = 28

TIPS = (
    ("N", "n"),
    ("E", "e"),
    ("S", "s"),
    ("W", "w"),
)

COL_ON = "#dcdc50"
COL_OFF = "#1e1e1e"
COL_BG = "#141418"
COL_X = "#101010"
COL_GRID = "#404048"
COL_CTR = "#ff2828"
COL_EDGE = "#909088"


def load_super_mask(path):
    with open(path, "rb") as f:
        sw, sh, ox, oy, min_rx, min_ry = struct.unpack("<HHHHii", f.read(16))
        raw = f.read(sw * sh)
    if len(raw) != sw * sh:
        raise SystemExit("bad super_submask.bin")
    mask = [[1 if raw[y * sw + x] else 0 for x in range(sw)] for y in range(sh)]
    return mask, {"sw": sw, "sh": sh, "ox": ox, "oy": oy, "min_rx": min_rx, "min_ry": min_ry}


def empty_splat(sw, sh):
    return [[0] * sw for _ in range(sh)]


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
    return rows if len(rows) == sh else None


class PreEditSplat(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("mk7 pre_edit_splat (super stamp N/E/S/W)")
        self.resizable(False, False)

        self.mask, self.meta = load_super_mask(MASK_BIN)
        self.sw = self.meta["sw"]
        self.sh = self.meta["sh"]
        self.ox = self.meta["ox"]
        self.oy = self.meta["oy"]

        self.tip_key = "N"
        self.tip_prefix = "n"
        self.splat = empty_splat(self.sw, self.sh)
        self.cur_idx = None
        self._hit = {}

        self.prev_w = self.sw * CELL
        self.prev_h = self.sh * CELL

        self._build()
        self._new_draft()

    def _bank_dir(self):
        return os.path.join(MK7, "splat_%s_bank" % self.tip_prefix)

    def _file_re(self):
        return re.compile(r"^%s-(\d+)\.splat$" % re.escape(self.tip_prefix))

    def _path_for(self, idx):
        return os.path.join(self._bank_dir(), "%s-%03d.splat" % (self.tip_prefix, idx))

    def _build(self):
        bar = ttk.Frame(self, padding=6)
        bar.pack(side=tk.TOP, fill=tk.X)
        ttk.Label(bar, text="tip").pack(side=tk.LEFT)
        self.tip_var = tk.StringVar(value=self.tip_key)
        cb = ttk.Combobox(
            bar,
            textvariable=self.tip_var,
            values=[t[0] for t in TIPS],
            width=4,
            state="readonly",
        )
        cb.pack(side=tk.LEFT, padx=4)
        cb.bind("<<ComboboxSelected>>", self._on_tip)
        ttk.Button(bar, text="Save", command=self._save).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Prev", command=lambda: self._cycle(-1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Next", command=lambda: self._cycle(1)).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="New", command=self._new_draft).pack(side=tk.LEFT, padx=2)
        ttk.Button(bar, text="Clear", command=self._clear).pack(side=tk.LEFT, padx=2)
        self.status = ttk.Label(bar, text="")
        self.status.pack(side=tk.LEFT, padx=10)

        self.cv = tk.Canvas(
            self,
            width=self.prev_w,
            height=self.prev_h,
            bg=COL_BG,
            highlightthickness=1,
        )
        self.cv.pack(side=tk.TOP)
        self.cv.bind("<Button-1>", self._click)

        tip = ttk.Label(
            self,
            text="stamp %dx%d (same as super_submask_cover.png)  |  yellow = mask-on  |  "
            "red = anchor  |  click toggles splat (X)  |  N/E/S/W separate  |  save → splat_*_bank"
            % (self.sw, self.sh),
            padding=6,
        )
        tip.pack(side=tk.TOP, fill=tk.X)

    def _on_tip(self, _e=None):
        key = self.tip_var.get()
        for k, p in TIPS:
            if k == key:
                self.tip_key = k
                self.tip_prefix = p
                break
        ids = self._list_indices()
        if ids:
            self._load(ids[0])
        else:
            self._new_draft()

    def _list_indices(self):
        out = []
        d = self._bank_dir()
        if not os.path.isdir(d):
            return out
        cre = self._file_re()
        for name in os.listdir(d):
            m = cre.match(name)
            if m:
                out.append(int(m.group(1)))
        return sorted(out)

    def _lowest_free(self):
        have = set(self._list_indices())
        i = 0
        while i in have:
            i += 1
        return i

    def _new_draft(self):
        self.splat = empty_splat(self.sw, self.sh)
        self.cur_idx = None
        self._set_status()
        self._redraw()

    def _clear(self):
        self.splat = empty_splat(self.sw, self.sh)
        self.cur_idx = None
        self._redraw()

    def _load(self, idx):
        path = self._path_for(idx)
        cells = load_splat(path, self.sw, self.sh) if os.path.isfile(path) else None
        if cells is None:
            self._new_draft()
            return
        self.splat = cells
        self.cur_idx = idx
        self._set_status()
        self._redraw()

    def _set_status(self):
        n = sum(
            self.splat[y][x]
            for y in range(self.sh)
            for x in range(self.sw)
            if self.mask[y][x] and self.splat[y][x]
        )
        if self.cur_idx is None:
            self.status.config(
                text="draft  tip=%s  splat=%d  → %s" % (self.tip_key, n, self._bank_dir())
            )
        else:
            self.status.config(
                text="%s-%03d.splat  splat=%d" % (self.tip_prefix, self.cur_idx, n)
            )

    def _save(self):
        os.makedirs(self._bank_dir(), exist_ok=True)
        idx = self.cur_idx if self.cur_idx is not None else self._lowest_free()
        path = self._path_for(idx)
        with open(path, "w") as f:
            f.write(
                "# mk7 super-stamp splat  tip=%s  %dx%d  1=splatter  0=plain\n"
                % (self.tip_key, self.sw, self.sh)
            )
            f.write("# same pixels as super_submask  ox=%d oy=%d  (no rotate)\n" % (self.ox, self.oy))
            for y in range(self.sh):
                f.write("".join(str(self.splat[y][x]) for x in range(self.sw)) + "\n")
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

    def _redraw(self):
        self.cv.delete("all")
        self._hit.clear()
        for y in range(self.sh):
            for x in range(self.sw):
                x0, y0 = x * CELL, y * CELL
                x1, y1 = x0 + CELL, y0 + CELL
                on = self.mask[y][x]
                fill = COL_ON if on else COL_OFF
                outline = COL_EDGE if on else COL_GRID
                item = self.cv.create_rectangle(
                    x0, y0, x1, y1, fill=fill, outline=outline, width=1
                )
                if on:
                    self._hit[item] = (x, y)
                if on and self.splat[y][x]:
                    mx = (x0 + x1) * 0.5
                    my = (y0 + y1) * 0.5
                    d = CELL * 0.28
                    self.cv.create_line(mx - d, my - d, mx + d, my + d, fill=COL_X, width=2)
                    self.cv.create_line(mx + d, my - d, mx - d, my + d, fill=COL_X, width=2)
                if x == self.ox and y == self.oy:
                    self.cv.create_rectangle(
                        x0 + 2, y0 + 2, x1 - 2, y1 - 2, outline=COL_CTR, width=2
                    )
        self.cv.create_text(
            self.prev_w - 14,
            12,
            text=self.tip_key,
            fill="#f0f0f0",
            font=("TkDefaultFont", 12, "bold"),
        )
        self._set_status()

    def _click(self, e):
        items = self.cv.find_overlapping(e.x, e.y, e.x, e.y)
        for item in reversed(items):
            if item not in self._hit:
                continue
            x, y = self._hit[item]
            self.splat[y][x] = 0 if self.splat[y][x] else 1
            self.cur_idx = None
            self._redraw()
            return


def main():
    app = PreEditSplat()
    app.mainloop()


if __name__ == "__main__":
    main()
