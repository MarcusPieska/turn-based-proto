#!/usr/bin/env python3

#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import subprocess
import sys
import tkinter as tk
from tkinter import ttk

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT_ROOT = "/home/w/Projects/simple-map-gen"
SEED_FILE = os.path.join(ROOT, "seed.txt")
MAP_W_DEF = 1000
MAP_H_DEF = 1000

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def read_seed_file():
    with open(SEED_FILE, "r", encoding="utf-8") as f:
        return int(f.read().strip())

def image_path(out_image, seed=None):
    if seed is None:
        s = read_seed_file()
        return os.path.join(OUT_ROOT, "p1-seed-%03d" % s, out_image)
    return os.path.join(OUT_ROOT, "p1_seed_%03d_%s" % (seed, out_image))

def _run_comp(comp_script):
    comp = os.path.join(ROOT, comp_script)
    if not os.path.isfile(comp):
        return False, "missing comp: %s" % comp_script
    proc = subprocess.run(["bash", comp], cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "compile failed"
        return False, msg
    return True, ""

def run_tester(tester_mod, out_image, seed=None, map_w=MAP_W_DEF, map_h=MAP_H_DEF):
    tester_exe = "%s_tester" % tester_mod
    comp_script = "%s_comp" % tester_mod
    exe = os.path.join(ROOT, tester_exe)
    if not os.path.isfile(exe):
        ok, msg = _run_comp(comp_script)
        if not ok:
            return False, msg
    if seed is None:
        args = [exe]
    else:
        args = [exe, str(seed), str(map_w), str(map_h)]
    proc = subprocess.run(args, cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "tester failed"
        return False, msg
    path = image_path(out_image, seed)
    if not os.path.isfile(path):
        return False, "missing image: %s" % path
    return True, path

def run_viewer(tester_mod, out_image, view_title, start_seed=None, cli_seed=False):
    if start_seed is None:
        start_seed = read_seed_file()
    root = tk.Tk()
    P1StepViewerApp(root, tester_mod, out_image, view_title, start_seed, cli_seed)
    root.mainloop()

def run_viewer_main(tester_mod, out_image, view_title):
    if len(sys.argv) >= 2:
        run_viewer(tester_mod, out_image, view_title, int(sys.argv[1]), True)
    else:
        run_viewer(tester_mod, out_image, view_title, read_seed_file(), False)

#================================================================================================================================#
#=> - P1StepViewerApp -
#================================================================================================================================#

class P1StepViewerApp(object):
    def __init__(m, root, tester_mod, out_image, view_title, start_seed, cli_seed):
        m.m_root = root
        m.m_tester_mod = tester_mod
        m.m_out_image = out_image
        m.m_cli_seed = cli_seed
        m.m_seed = start_seed
        m.m_photo = None
        m.m_root.title(view_title)
        m._build_ui()
        m._refresh()

    def _build_ui(m):
        frm = ttk.Frame(m.m_root, padding=8)
        frm.pack(fill=tk.BOTH, expand=True)
        top = ttk.Frame(frm)
        top.pack(fill=tk.X, pady=(0, 8))
        m.m_seed_lbl = ttk.Label(top, text="seed 000")
        m.m_seed_lbl.pack(side=tk.LEFT)
        ttk.Button(top, text="next seed", command=m._on_next).pack(side=tk.RIGHT)
        ttk.Button(top, text="re-run", command=m._refresh).pack(side=tk.RIGHT, padx=(0, 8))
        m.m_status = ttk.Label(frm, text="")
        m.m_status.pack(fill=tk.X, pady=(0, 8))
        m.m_cvs = tk.Canvas(frm, width=800, height=800, bg="#202020", highlightthickness=1)
        m.m_cvs.pack()

    def _on_next(m):
        m.m_cli_seed = True
        m.m_seed = (m.m_seed + 1) % 1000
        m._refresh()

    def _refresh(m):
        m.m_seed_lbl.config(text="seed %03d" % m.m_seed)
        m.m_status.config(text="running %s_tester ..." % m.m_tester_mod)
        m.m_root.update_idletasks()
        run_seed = m.m_seed if m.m_cli_seed else None
        ok, msg = run_tester(m.m_tester_mod, m.m_out_image, run_seed)
        if not ok:
            m.m_cvs.delete("all")
            m.m_status.config(text=msg)
            return
        try:
            m.m_photo = tk.PhotoImage(file=msg)
            w = m.m_photo.width()
            h = m.m_photo.height()
            scale = 1
            while w // scale > 800 or h // scale > 800:
                scale += 1
            if scale > 1:
                m.m_photo = m.m_photo.subsample(scale, scale)
            m.m_cvs.delete("all")
            m.m_cvs.config(width=m.m_photo.width(), height=m.m_photo.height())
            m.m_cvs.create_image(0, 0, image=m.m_photo, anchor=tk.NW)
            m.m_status.config(text=msg)
        except tk.TclError as exc:
            m.m_cvs.delete("all")
            m.m_status.config(text="display failed: %s" % exc)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
