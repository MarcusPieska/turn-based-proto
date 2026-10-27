#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import math
import glob
import re
import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from PIL import Image, ImageTk
import numpy as np

#================================================================================================================================#
#=> - Class: PaletteRecolor -
#================================================================================================================================#

class PaletteRecolor:

    HIST_MAX = 1000
    TEX_ROOT = "/home/w/Projects/img-content"
    DERIVED_MARK = "DERIVED"

    def __meanRgb (m, arr):
        flat = arr.reshape (-1, 3).astype (np.float64)
        return flat.mean (axis=0)

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

    def __snap (m):
        return (float (m.center1[0]), float (m.center1[1]), float (m.center1[2]),
                int (m.bright_var.get ()), int (m.spread_var.get ()))

    def __push (m):
        if m.hist_lock or m.src is None:
            return
        st = m.__snap ()
        if m.hist and m.hist[m.hist_i] == st:
            return
        m.hist = m.hist[: m.hist_i + 1]
        m.hist.append (st)
        if len (m.hist) > m.HIST_MAX:
            m.hist = m.hist[len (m.hist) - m.HIST_MAX :]
        m.hist_i = len (m.hist) - 1
        m.__histUi ()

    def __restore (m, st):
        m.hist_lock = True
        m.center1 = np.array ([st[0], st[1], st[2]], dtype=np.float64)
        m.bright_var.set (st[3])
        m.spread_var.set (st[4])
        m.hist_lock = False
        m.__markFromRgb (m.center1)
        m.__apply ()
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

    def __apply (m):
        if m.src is None:
            return
        c0 = m.center0
        c1 = m.center1
        spread = m.spread_var.get () / 100.0
        bright = float (m.bright_var.get ())
        src = m.src.astype (np.float64)
        out = c1 + spread * (src - c0) + bright
        m.dst = m.__clampRgb (out)
        m.__show ()

    def __show (m):
        if m.src is None:
            return
        z = m.zoom
        src_s = Image.fromarray (m.src, "RGB").resize (
            (m.src.shape[1] * z, m.src.shape[0] * z), Image.NEAREST)
        dst_s = Image.fromarray (m.dst, "RGB").resize (
            (m.dst.shape[1] * z, m.dst.shape[0] * z), Image.NEAREST)
        m.photo_src = ImageTk.PhotoImage (src_s)
        m.photo_dst = ImageTk.PhotoImage (dst_s)
        m.canv_src.delete ("all")
        m.canv_dst.delete ("all")
        m.canv_src.create_image (0, 0, anchor=tk.NW, image=m.photo_src)
        m.canv_dst.create_image (0, 0, anchor=tk.NW, image=m.photo_dst)
        m.canv_src.config (width=src_s.width, height=src_s.height)
        m.canv_dst.config (width=dst_s.width, height=dst_s.height)
        m.swatch0.config (bg=m.__rgbToHex (m.center0))
        m.swatch1.config (bg=m.__rgbToHex (m.center1))
        m.lbl_c0.config (text="center src  %d %d %d" % tuple (int (round (v)) for v in m.center0))
        m.lbl_c1.config (text="center dst  %d %d %d" % tuple (int (round (v)) for v in m.center1))

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

    def __loadPath (m, path):
        if not path or not os.path.isfile (path):
            return
        img = Image.open (path).convert ("RGB")
        m.path = path
        m.src = np.array (img)
        m.center0 = m.__meanRgb (m.src)
        m.center1 = m.center0.copy ()
        m.hist_lock = True
        m.bright_var.set (0)
        m.spread_var.set (100)
        m.hist_lock = False
        m.init_state = m.__snap ()
        m.hist = [m.init_state]
        m.hist_i = 0
        m.root.title ("Palette Recolor — %s" % os.path.basename (path))
        m.__selectPath (path)
        m.__markFromRgb (m.center1)
        m.__apply ()
        m.__histUi ()

    def __folderOf (m, path):
        return os.path.dirname (path)

    def __isDerivedFolder (m, folder):
        return os.path.isfile (os.path.join (folder, m.DERIVED_MARK))

    def __scanTemplates (m):
        paths = sorted (glob.glob (os.path.join (m.TEX_ROOT, "texture-*", "*_palette.ppm")))
        if not paths:
            paths = sorted (glob.glob (os.path.join (m.TEX_ROOT, "texture-*", "*_palette.png")))
        m.tmpl_paths = paths
        m.tmpl_labels = []
        if not hasattr (m, "protected") or m.protected is None:
            m.protected = set ()
            for p in paths:
                if not m.__isDerivedFolder (m.__folderOf (p)):
                    m.protected.add (os.path.realpath (p))
        for p in paths:
            folder = os.path.basename (m.__folderOf (p))
            tag = "[derived] " if m.__isDerivedFolder (m.__folderOf (p)) else ""
            m.tmpl_labels.append ("%s%s / %s" % (tag, folder, os.path.basename (p)))
        return paths

    def __refreshTmplBox (m):
        cur = m.path
        m.__scanTemplates ()
        m.tmpl_box["values"] = m.tmpl_labels
        if cur and cur in m.tmpl_paths:
            m.__selectPath (cur)
        elif m.tmpl_labels:
            m.tmpl_var.set (m.tmpl_labels[0])

    def __onPick (m, _=None):
        if m.hist_lock:
            return
        lab = m.tmpl_var.get ()
        if lab not in m.tmpl_labels:
            return
        path = m.tmpl_paths[m.tmpl_labels.index (lab)]
        if path != m.path:
            m.__loadPath (path)

    def __selectPath (m, path):
        if path in m.tmpl_paths:
            m.tmpl_var.set (m.tmpl_labels[m.tmpl_paths.index (path)])

    def __onSave (m):
        if m.src is None or m.dst is None:
            return
        tag = simpledialog.askstring (
            "Save derived palette",
            "Folder tag (texture-<tag>):",
            parent=m.root)
        if tag is None:
            return
        tag = tag.strip ()
        if not tag or not re.match (r"^[A-Za-z0-9][A-Za-z0-9_-]*$", tag):
            messagebox.showerror ("Save", "Use a simple tag: letters, digits, _ or -.")
            return
        folder = os.path.join (m.TEX_ROOT, "texture-%s" % tag)
        out = os.path.join (folder, "colors-%s_palette.ppm" % tag)
        real = os.path.realpath (out)
        if real in m.protected:
            messagebox.showerror (
                "Save blocked",
                "Cannot overwrite an original palette:\n%s" % out)
            return
        if os.path.isdir (folder) and not m.__isDerivedFolder (folder):
            messagebox.showerror (
                "Save blocked",
                "Folder exists and is not marked derived:\n%s\n"
                "Original texture folders are protected." % folder)
            return
        if os.path.isfile (out) and not m.__isDerivedFolder (folder):
            messagebox.showerror ("Save blocked", "Refusing to overwrite unprotected file:\n%s" % out)
            return
        try:
            os.makedirs (folder, exist_ok=True)
            Image.fromarray (m.dst, "RGB").save (out)
            mark = os.path.join (folder, m.DERIVED_MARK)
            with open (mark, "w") as f:
                f.write ("derived palette\n")
        except Exception as e:
            messagebox.showerror ("Save failed", str (e))
            return
        m.__refreshTmplBox ()
        m.__loadPath (out)
        messagebox.showinfo ("Saved", "Wrote:\n%s\n(+ %s)" % (out, m.DERIVED_MARK))

    def __onReset (m):
        if m.src is None or m.init_state is None:
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
        if m.hist_lock or m.src is None:
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

    def __buildUi (m):
        top = tk.Frame (m.root)
        top.pack (side=tk.TOP, fill=tk.X, padx=5, pady=5)
        tk.Label (top, text="Template").pack (side=tk.LEFT, padx=3)
        m.__scanTemplates ()
        m.tmpl_var = tk.StringVar (value=m.tmpl_labels[0] if m.tmpl_labels else "")
        m.tmpl_box = ttk.Combobox (top, textvariable=m.tmpl_var, values=m.tmpl_labels,
                                   state="readonly", width=48)
        m.tmpl_box.pack (side=tk.LEFT, padx=3)
        m.tmpl_box.bind ("<<ComboboxSelected>>", m.__onPick)
        tk.Button (top, text="Reset", command=m.__onReset, width=10).pack (side=tk.LEFT, padx=3)
        m.btn_undo = tk.Button (top, text="Undo", command=m.__onUndo, width=8, state=tk.DISABLED)
        m.btn_undo.pack (side=tk.LEFT, padx=3)
        m.btn_redo = tk.Button (top, text="Redo", command=m.__onRedo, width=8, state=tk.DISABLED)
        m.btn_redo.pack (side=tk.LEFT, padx=3)
        tk.Button (top, text="Save derived", command=m.__onSave, width=12).pack (side=tk.LEFT, padx=3)

        mid = tk.Frame (m.root)
        mid.pack (side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        left = tk.Frame (mid)
        left.pack (side=tk.LEFT, fill=tk.Y, padx=5)
        tk.Label (left, text="New central color").pack (side=tk.TOP, anchor=tk.W)
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
        tk.Label (ctrl, text="Spread around center (100 = identity)").pack (side=tk.TOP, anchor=tk.W)
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
        m.lbl_c0 = tk.Label (left, text="center src", anchor=tk.W)
        m.lbl_c0.pack (side=tk.TOP, fill=tk.X)
        m.lbl_c1 = tk.Label (left, text="center dst", anchor=tk.W)
        m.lbl_c1.pack (side=tk.TOP, fill=tk.X)

        right = tk.Frame (mid)
        right.pack (side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)
        row = tk.Frame (right)
        row.pack (side=tk.TOP, fill=tk.BOTH, expand=True)
        col0 = tk.Frame (row)
        col0.pack (side=tk.LEFT, padx=5)
        col1 = tk.Frame (row)
        col1.pack (side=tk.LEFT, padx=5)
        tk.Label (col0, text="Original").pack (side=tk.TOP, anchor=tk.W)
        m.canv_src = tk.Canvas (col0, bg="#303030", highlightthickness=1, highlightbackground="#666")
        m.canv_src.pack (side=tk.TOP)
        tk.Label (col1, text="Preview").pack (side=tk.TOP, anchor=tk.W)
        m.canv_dst = tk.Canvas (col1, bg="#303030", highlightthickness=1, highlightbackground="#666")
        m.canv_dst.pack (side=tk.TOP)

        m.root.bind ("<Control-z>", m.__onUndo)
        m.root.bind ("<Control-Z>", m.__onUndo)
        m.root.bind ("<Control-y>", m.__onRedo)
        m.root.bind ("<Control-Y>", m.__onRedo)

    def __init__ (m, root, path=None):
        m.root = root
        m.path = None
        m.src = None
        m.dst = None
        m.center0 = np.array ([0.0, 0.0, 0.0])
        m.center1 = np.array ([0.0, 0.0, 0.0])
        m.init_state = None
        m.hist = []
        m.hist_i = -1
        m.hist_lock = False
        m.tmpl_paths = []
        m.tmpl_labels = []
        m.protected = None
        m.zoom = 4
        m.wheel_size = 260
        m.wheel_rad = 120
        m.wheel_light = 0.45
        m.photo_src = None
        m.photo_dst = None
        m.wheel_photo = None
        m.__buildUi ()
        if path and path in m.tmpl_paths:
            m.__loadPath (path)
        elif m.tmpl_paths:
            m.__loadPath (m.tmpl_paths[0])

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    default = "/home/w/Projects/img-content/texture-grassland4/colors-grassland4_palette.ppm"
    path = sys.argv[1] if len (sys.argv) > 1 else default
    root = tk.Tk ()
    root.title ("Palette Recolor")
    PaletteRecolor (root, path if os.path.isfile (path) else None)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
