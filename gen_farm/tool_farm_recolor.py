#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir (os.path.dirname (os.path.abspath (__file__)))
sys.path.insert (0, os.getcwd ())

import math
import re
import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from PIL import Image, ImageTk
import numpy as np

#================================================================================================================================#
#=> - Class: FarmRecolor -
#================================================================================================================================#

class FarmRecolor:

    HIST_MAX = 1000
    ROOT = "/home/w/Projects/rts-proto/gen_farm"
    DERIVED_MARK = "DERIVED"
    PROTECTED = ("set01", "set02")
    KEY = (255, 0, 255)
    ZOOM = 3

    def __meanRgb (m, arr):
        mask = m.__mask (arr)
        if not np.any (mask):
            return np.array ([0.0, 0.0, 0.0], dtype=np.float64)
        return arr[mask].astype (np.float64).mean (axis=0)

    def __mask (m, arr):
        return ~((arr[:, :, 0] == m.KEY[0]) & (arr[:, :, 1] == m.KEY[1]) & (arr[:, :, 2] == m.KEY[2]))

    def __clampRgb (m, arr):
        return np.clip (arr, 0, 255).astype (np.uint8)

    def __hslToRgb (m, h, s, l):
        h = h % 360.0
        s = max (0.0, min (1.0, s))
        l = max (0.0, min (1.0, l))
        c = (1.0 - abs (2.0 * l - 1.0)) * s
        x = c * (1.0 - abs ((h / 60.0) % 2.0 - 1.0))
        k = l - c / 2.0
        if 0 <= h < 60: r, g, b = c, x, 0.0
        elif 60 <= h < 120: r, g, b = x, c, 0.0
        elif 120 <= h < 180: r, g, b = 0.0, c, x
        elif 180 <= h < 240: r, g, b = 0.0, x, c
        elif 240 <= h < 300: r, g, b = x, 0.0, c
        else: r, g, b = c, 0.0, x
        return (int ((r + k) * 255), int ((g + k) * 255), int ((b + k) * 255))

    def __rgbToHsl (m, r, g, b):
        r, g, b = r / 255.0, g / 255.0, b / 255.0
        mx, mn = max (r, g, b), min (r, g, b)
        l = (mx + mn) / 2.0
        if mx == mn:
            return 0.0, 0.0, l
        d = mx - mn
        s = d / (2.0 - mx - mn) if l > 0.5 else d / (mx + mn)
        if mx == r:
            h = (g - b) / d + (6.0 if g < b else 0.0)
        elif mx == g:
            h = (b - r) / d + 2.0
        else:
            h = (r - g) / d + 4.0
        return h * 60.0, s, l

    def __rgbToHex (m, rgb):
        return "#%02x%02x%02x" % (int (rgb[0]), int (rgb[1]), int (rgb[2]))

    def __makeWheel (m):
        size = m.wheel_size
        cx = cy = size // 2
        rad = m.wheel_rad
        img = Image.new ("RGB", (size, size), "#202020")
        px = img.load ()
        for y in range (size):
            for x in range (size):
                dx = x - cx
                dy = y - cy
                dist = math.hypot (dx, dy)
                if dist > rad:
                    continue
                ang = math.degrees (math.atan2 (-dy, dx))
                if ang < 0:
                    ang += 360.0
                sat = dist / rad if rad > 0 else 0.0
                px[x, y] = m.__hslToRgb (ang, sat, m.wheel_light)
        return img

    def __isDerived (m, folder):
        return os.path.isfile (os.path.join (folder, m.DERIVED_MARK))

    def __isProtectedName (m, name):
        return name in m.PROTECTED

    def __scanSets (m):
        names = []
        if not os.path.isdir (m.ROOT):
            return names
        for name in sorted (os.listdir (m.ROOT)):
            path = os.path.join (m.ROOT, name)
            if not os.path.isdir (path):
                continue
            pngs = [f for f in os.listdir (path) if f.lower ().endswith (".png")]
            if not pngs:
                continue
            names.append (name)
        return names

    def __listPngs (m, folder):
        names = [f for f in os.listdir (folder) if f.lower ().endswith (".png")]
        return sorted (names)

    def __snap (m):
        imgs = [arr.copy () for arr in m.dst]
        sels = [bool (v.get ()) for v in m.sel_vars]
        return (imgs, sels,
                float (m.center1[0]), float (m.center1[1]), float (m.center1[2]),
                float (m.base_center[0]), float (m.base_center[1]), float (m.base_center[2]),
                int (m.bright_var.get ()), int (m.spread_var.get ()))

    def __push (m):
        if m.hist_lock or not m.src:
            return
        st = m.__snap ()
        if m.hist and m.__sameState (m.hist[m.hist_i], st):
            return
        m.hist = m.hist[: m.hist_i + 1]
        m.hist.append (st)
        if len (m.hist) > m.HIST_MAX:
            m.hist = m.hist[len (m.hist) - m.HIST_MAX :]
        m.hist_i = len (m.hist) - 1
        m.__histUi ()

    def __sameState (m, a, b):
        if a[2:] != b[2:]:
            return False
        if a[1] != b[1]:
            return False
        for x, y in zip (a[0], b[0]):
            if not np.array_equal (x, y):
                return False
        return True

    def __restore (m, st):
        m.hist_lock = True
        imgs, sels, c10, c11, c12, b0, b1, b2, bright, spread = st
        m.dst = [arr.copy () for arr in imgs]
        for i, v in enumerate (m.sel_vars):
            if i < len (sels):
                v.set (sels[i])
        m.center1 = np.array ([c10, c11, c12], dtype=np.float64)
        m.base_center = np.array ([b0, b1, b2], dtype=np.float64)
        m.bright_var.set (bright)
        m.spread_var.set (spread)
        m.hist_lock = False
        m.__markFromRgb (m.center1)
        m.__showAll ()
        m.__histUi ()

    def __histUi (m):
        m.btn_undo.config (state=tk.NORMAL if m.hist_i > 0 else tk.DISABLED)
        m.btn_redo.config (state=tk.NORMAL if m.hist_i + 1 < len (m.hist) else tk.DISABLED)

    def __onUndo (m, _=None):
        if m.hist_i <= 0:
            return
        m.hist_i -= 1
        m.__restore (m.hist[m.hist_i])

    def __onRedo (m, _=None):
        if m.hist_i + 1 >= len (m.hist):
            return
        m.hist_i += 1
        m.__restore (m.hist[m.hist_i])

    def __xformOne (m, src):
        mask = m.__mask (src)
        c0 = m.__meanRgb (src)
        delta = m.center1 - m.base_center
        spread = m.spread_var.get () / 100.0
        bright = float (m.bright_var.get ())
        out = src.astype (np.float64).copy ()
        if np.any (mask):
            out[mask] = c0 + delta + spread * (src[mask].astype (np.float64) - c0) + bright
        out = m.__clampRgb (out)
        out[~mask] = src[~mask]
        return out

    def __apply (m):
        if not m.src:
            return
        for i, src in enumerate (m.src):
            if i < len (m.sel_vars) and m.sel_vars[i].get ():
                m.dst[i] = m.__xformOne (src)
            else:
                m.dst[i] = src.copy ()
        m.__showAll ()
        m.__swatchUi ()

    def __swatchUi (m):
        m.swatch0.config (bg=m.__rgbToHex (m.base_center))
        m.swatch1.config (bg=m.__rgbToHex (m.center1))
        m.lbl_c0.config (text="base center  %d %d %d" % tuple (int (round (v)) for v in m.base_center))
        m.lbl_c1.config (text="new center   %d %d %d" % tuple (int (round (v)) for v in m.center1))

    def __showAll (m):
        z = m.ZOOM
        m.photos = []
        for i, arr in enumerate (m.dst):
            im = Image.fromarray (arr, "RGB").resize ((arr.shape[1] * z, arr.shape[0] * z), Image.NEAREST)
            ph = ImageTk.PhotoImage (im)
            m.photos.append (ph)
            canv = m.tile_canv[i]
            canv.delete ("all")
            canv.create_image (0, 0, anchor=tk.NW, image=ph)
            canv.config (width=im.width, height=im.height)

    def __markFromRgb (m, rgb):
        h, s, _l = m.__rgbToHsl (float (rgb[0]), float (rgb[1]), float (rgb[2]))
        cx = cy = m.wheel_size // 2
        rad = min (max (s, 0.0), 1.0) * m.wheel_rad
        ang = math.radians (h)
        x = cx + rad * math.cos (ang)
        y = cy - rad * math.sin (ang)
        m.__drawMark (x, y)

    def __drawMark (m, x, y):
        m.canv_wheel.delete ("mark")
        r = 6
        m.canv_wheel.create_oval (x - r, y - r, x + r, y + r, outline="white", width=2, tags="mark")
        m.canv_wheel.create_oval (x - r + 1, y - r + 1, x + r - 1, y + r - 1, outline="black", width=1, tags="mark")

    def __clearTiles (m):
        for w in m.tile_frame.winfo_children ():
            w.destroy ()
        m.tile_canv = []
        m.sel_vars = []
        m.photos = []
        m.names = []
        m.src = []
        m.dst = []

    def __buildTileGrid (m):
        m.__clearTiles ()
        if not m.set_dir:
            return
        names = m.__listPngs (m.set_dir)
        cols = 5
        for i, name in enumerate (names):
            path = os.path.join (m.set_dir, name)
            img = Image.open (path).convert ("RGB")
            arr = np.array (img)
            m.names.append (name)
            m.src.append (arr)
            m.dst.append (arr.copy ())
            cell = tk.Frame (m.tile_frame)
            r, c = divmod (i, cols)
            cell.grid (row=r, column=c, padx=6, pady=6)
            canv = tk.Canvas (cell, bg="#303030", highlightthickness=1, highlightbackground="#666")
            canv.pack (side=tk.TOP)
            m.tile_canv.append (canv)
            var = tk.BooleanVar (value=True)
            m.sel_vars.append (var)
            tk.Checkbutton (cell, text=name, variable=var, command=m.__onSel).pack (side=tk.TOP)

    def __baseFromSrc (m):
        if not m.src:
            m.base_center = np.array ([0.0, 0.0, 0.0])
            return
        means = [m.__meanRgb (a) for a in m.src]
        m.base_center = np.mean (np.stack (means, axis=0), axis=0)

    def __loadSet (m, name):
        folder = os.path.join (m.ROOT, name)
        if not os.path.isdir (folder):
            return
        m.set_name = name
        m.set_dir = folder
        m.__buildTileGrid ()
        m.__baseFromSrc ()
        m.center1 = m.base_center.copy ()
        m.hist_lock = True
        m.bright_var.set (0)
        m.spread_var.set (100)
        m.hist_lock = False
        m.__markFromRgb (m.center1)
        m.__apply ()
        m.init_state = m.__snap ()
        m.hist = [m.init_state]
        m.hist_i = 0
        m.root.title ("Farm Recolor — %s" % name)
        m.__histUi ()
        m.__swatchUi ()

    def __onPick (m, _=None):
        if m.hist_lock:
            return
        name = m.set_var.get ()
        if name and name != m.set_name:
            m.__loadSet (name)

    def __onSel (m):
        if m.hist_lock or not m.src:
            return
        m.__apply ()
        m.__push ()

    def __onSelAll (m):
        for v in m.sel_vars:
            v.set (True)
        m.__onSel ()

    def __onSelNone (m):
        for v in m.sel_vars:
            v.set (False)
        m.__onSel ()

    def __onSave (m):
        if not m.src or not m.dst:
            return
        tag = simpledialog.askstring (
            "Save derived farm set",
            "Set folder name (e.g. set03):",
            parent=m.root)
        if tag is None:
            return
        tag = tag.strip ()
        if not tag or not re.match (r"^[A-Za-z0-9][A-Za-z0-9_-]*$", tag):
            messagebox.showerror ("Save", "Use a simple name: letters, digits, _ or -.")
            return
        if m.__isProtectedName (tag):
            messagebox.showerror ("Save blocked", "Cannot overwrite protected set:\n%s" % tag)
            return
        folder = os.path.join (m.ROOT, tag)
        if os.path.isdir (folder) and not m.__isDerived (folder):
            messagebox.showerror (
                "Save blocked",
                "Folder exists and is not marked derived:\n%s" % folder)
            return
        try:
            os.makedirs (folder, exist_ok=True)
            for name, arr in zip (m.names, m.dst):
                Image.fromarray (arr, "RGB").save (os.path.join (folder, name))
            with open (os.path.join (folder, m.DERIVED_MARK), "w") as f:
                f.write ("derived farm set from %s\n" % m.set_name)
        except Exception as e:
            messagebox.showerror ("Save failed", str (e))
            return
        m.__refreshSets ()
        m.set_var.set (tag)
        m.__loadSet (tag)
        messagebox.showinfo ("Saved", "Wrote set:\n%s\n(+ %s)" % (folder, m.DERIVED_MARK))

    def __onReset (m):
        if not m.src or m.init_state is None:
            return
        m.__restore (m.init_state)
        m.__push ()

    def __onBright (m, _=None):
        if m.hist_lock:
            return
        m.__apply ()

    def __onSpread (m, _=None):
        if m.hist_lock:
            return
        m.__apply ()

    def __onSlideDone (m, _=None):
        if m.hist_lock or not m.src:
            return
        m.__push ()

    def __onWheel (m, ev):
        cx = cy = m.wheel_size // 2
        dx = ev.x - cx
        dy = ev.y - cy
        dist = math.hypot (dx, dy)
        if dist > m.wheel_rad or dist < 1e-6:
            return
        ang = math.degrees (math.atan2 (-dy, dx))
        if ang < 0:
            ang += 360.0
        sat = min (1.0, dist / m.wheel_rad)
        r, g, b = m.__hslToRgb (ang, sat, m.wheel_light)
        m.center1 = np.array ([r, g, b], dtype=np.float64)
        m.__drawMark (ev.x, ev.y)
        m.__apply ()
        m.__push ()

    def __refreshSets (m):
        names = m.__scanSets ()
        m.set_box["values"] = names
        if m.set_name in names:
            m.set_var.set (m.set_name)
        elif names:
            m.set_var.set (names[0])

    def __buildUi (m):
        top = tk.Frame (m.root)
        top.pack (side=tk.TOP, fill=tk.X, padx=5, pady=5)
        tk.Label (top, text="Farm set").pack (side=tk.LEFT, padx=3)
        names = m.__scanSets ()
        m.set_var = tk.StringVar (value=names[0] if names else "")
        m.set_box = ttk.Combobox (top, textvariable=m.set_var, values=names, state="readonly", width=16)
        m.set_box.pack (side=tk.LEFT, padx=3)
        m.set_box.bind ("<<ComboboxSelected>>", m.__onPick)
        tk.Button (top, text="All", command=m.__onSelAll, width=6).pack (side=tk.LEFT, padx=2)
        tk.Button (top, text="None", command=m.__onSelNone, width=6).pack (side=tk.LEFT, padx=2)
        tk.Button (top, text="Reset", command=m.__onReset, width=8).pack (side=tk.LEFT, padx=3)
        m.btn_undo = tk.Button (top, text="Undo", command=m.__onUndo, width=8, state=tk.DISABLED)
        m.btn_undo.pack (side=tk.LEFT, padx=3)
        m.btn_redo = tk.Button (top, text="Redo", command=m.__onRedo, width=8, state=tk.DISABLED)
        m.btn_redo.pack (side=tk.LEFT, padx=3)
        tk.Button (top, text="Save derived", command=m.__onSave, width=12).pack (side=tk.LEFT, padx=3)

        mid = tk.Frame (m.root)
        mid.pack (side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        left = tk.Frame (mid)
        left.pack (side=tk.LEFT, fill=tk.Y, padx=5)
        tk.Label (left, text="Recolor center").pack (side=tk.TOP, anchor=tk.W)
        m.canv_wheel = tk.Canvas (left, width=m.wheel_size, height=m.wheel_size, bg="#202020", highlightthickness=0)
        m.canv_wheel.pack (side=tk.TOP)
        m.canv_wheel.bind ("<Button-1>", m.__onWheel)
        m.wheel_photo = ImageTk.PhotoImage (m.__makeWheel ())
        m.canv_wheel.create_image (m.wheel_size // 2, m.wheel_size // 2, image=m.wheel_photo)

        ctrl = tk.Frame (left)
        ctrl.pack (side=tk.TOP, fill=tk.X, pady=8)
        tk.Label (ctrl, text="Brightness").pack (side=tk.TOP, anchor=tk.W)
        m.bright_var = tk.IntVar (value=0)
        m.bright_scale = tk.Scale (ctrl, from_=-128, to=128, orient=tk.HORIZONTAL, length=260,
                                   variable=m.bright_var, command=m.__onBright)
        m.bright_scale.pack (side=tk.TOP, fill=tk.X)
        m.bright_scale.bind ("<ButtonRelease-1>", m.__onSlideDone)
        tk.Label (ctrl, text="Contrast around mean (100 = identity)").pack (side=tk.TOP, anchor=tk.W)
        m.spread_var = tk.IntVar (value=100)
        m.spread_scale = tk.Scale (ctrl, from_=0, to=250, orient=tk.HORIZONTAL, length=260,
                                   variable=m.spread_var, command=m.__onSpread)
        m.spread_scale.pack (side=tk.TOP, fill=tk.X)
        m.spread_scale.bind ("<ButtonRelease-1>", m.__onSlideDone)

        sw = tk.Frame (left)
        sw.pack (side=tk.TOP, fill=tk.X, pady=6)
        m.swatch0 = tk.Label (sw, text="      ", width=10, height=2, relief=tk.SUNKEN)
        m.swatch0.pack (side=tk.LEFT, padx=3)
        m.swatch1 = tk.Label (sw, text="      ", width=10, height=2, relief=tk.SUNKEN)
        m.swatch1.pack (side=tk.LEFT, padx=3)
        m.lbl_c0 = tk.Label (left, text="base center", anchor=tk.W)
        m.lbl_c0.pack (side=tk.TOP, fill=tk.X)
        m.lbl_c1 = tk.Label (left, text="new center", anchor=tk.W)
        m.lbl_c1.pack (side=tk.TOP, fill=tk.X)

        right = tk.Frame (mid)
        right.pack (side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)
        wrap = tk.Frame (right)
        wrap.pack (side=tk.TOP, fill=tk.BOTH, expand=True)
        m.tile_canvas = tk.Canvas (wrap, bg="#252525", highlightthickness=0)
        scroll = tk.Scrollbar (wrap, orient=tk.VERTICAL, command=m.tile_canvas.yview)
        m.tile_canvas.configure (yscrollcommand=scroll.set)
        scroll.pack (side=tk.RIGHT, fill=tk.Y)
        m.tile_canvas.pack (side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.tile_frame = tk.Frame (m.tile_canvas, bg="#252525")
        m.tile_canvas.create_window ((0, 0), window=m.tile_frame, anchor=tk.NW)
        m.tile_frame.bind ("<Configure>",
            lambda e: m.tile_canvas.configure (scrollregion=m.tile_canvas.bbox ("all")))

        m.root.bind ("<Control-z>", m.__onUndo)
        m.root.bind ("<Control-Z>", m.__onUndo)
        m.root.bind ("<Control-y>", m.__onRedo)
        m.root.bind ("<Control-Y>", m.__onRedo)

    def __init__ (m, root, set_name=None):
        m.root = root
        m.set_name = None
        m.set_dir = None
        m.names = []
        m.src = []
        m.dst = []
        m.tile_canv = []
        m.sel_vars = []
        m.photos = []
        m.base_center = np.array ([0.0, 0.0, 0.0])
        m.center1 = np.array ([0.0, 0.0, 0.0])
        m.init_state = None
        m.hist = []
        m.hist_i = -1
        m.hist_lock = False
        m.wheel_size = 260
        m.wheel_rad = 120
        m.wheel_light = 0.45
        m.wheel_photo = None
        m.__buildUi ()
        names = m.__scanSets ()
        pick = set_name if set_name in names else (names[0] if names else None)
        if pick:
            m.set_var.set (pick)
            m.__loadSet (pick)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    name = sys.argv[1] if len (sys.argv) > 1 else "set01"
    root = tk.Tk ()
    root.title ("Farm Recolor")
    root.geometry ("1200x720")
    FarmRecolor (root, name)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
