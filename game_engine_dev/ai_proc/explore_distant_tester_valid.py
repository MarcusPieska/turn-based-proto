#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import tkinter as tk
from tkinter import ttk

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

MAP_W = 1000
MAP_H = 1000
CANVAS_W = 1200
CANVAS_H = 800

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
TERR_PATH = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm"
CLIM_PATH = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm"
RIV_PATH = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm"
TRACE_PATH = os.path.join(THIS_DIR, "explore_distant_test.trace")

SPEEDS_MS = (10, 100, 1000)
MAP_NAMES = ("terrain", "climate", "rivers")
UNEXPLORED = (24, 24, 32)
P0_TINT = (0, 180, 80)
P1_TINT = (80, 120, 220)
BOTH_TINT = (200, 180, 40)

#================================================================================================================================#
#=> - PPM -
#================================================================================================================================#

def ppm_read_token (f):
    tok = b""
    while True:
        c = f.read(1)
        if not c:
            break
        if c in b" \t\r\n":
            if tok:
                break
            if c == b"#":
                while c not in (b"\n", b""):
                    c = f.read(1)
            continue
        if c == b"#":
            while c not in (b"\n", b""):
                c = f.read(1)
            continue
        tok += c
    if not tok:
        return None
    return tok.decode("ascii")

def load_ppm_rgb (path):
    with open(path, "rb") as f:
        if f.read(2) != b"P6":
            raise ValueError("not P6 ppm: " + path)
        w = int(ppm_read_token(f))
        h = int(ppm_read_token(f))
        maxv = int(ppm_read_token(f))
        if maxv != 255:
            raise ValueError("expected maxval 255: " + path)
        raw = f.read(w * h * 3)
        if len(raw) != w * h * 3:
            raise ValueError("short ppm body: " + path)
    return w, h, raw

def rgb_view_ppm (w, h, rgb):
    return b"P6\n" + ("%d %d\n" % (w, h)).encode("ascii") + b"255\n" + rgb

#================================================================================================================================#
#=> - Trace -
#================================================================================================================================#

def parse_trace (path):
    explored = {0: set(), 1: set()}
    snapshots = []
    with open(path, "r", encoding="utf-8") as ptr:
        for line in ptr:
            line = line.strip()
            if line == "":
                continue
            if line.startswith("EXPLORE_DISCOVER:"):
                parts = line.split(":")
                if len(parts) != 4:
                    continue
                x = int(parts[1])
                y = int(parts[2])
                p = int(parts[3])
                explored[p].add((x, y))
            elif line.startswith("NEW_TURN:"):
                snapshots.append({0: set(explored[0]), 1: set(explored[1])})
    snapshots.append({0: set(explored[0]), 1: set(explored[1])})
    if len(snapshots) == 0:
        snapshots.append({0: set(), 1: set()})
    return snapshots

#================================================================================================================================#
#=> - Compose -
#================================================================================================================================#

def blend_px (base, tint, alpha):
    br, bg, bb = base
    tr, tg, tb = tint
    a = alpha / 255.0
    return (
        int(br * (1.0 - a) + tr * a),
        int(bg * (1.0 - a) + tg * a),
        int(bb * (1.0 - a) + tb * a),
    )

def compose_frame (base_rgb, exp0, exp1):
    out = bytearray(len(base_rgb))
    for y in range(MAP_H):
        for x in range(MAP_W):
            i = (y * MAP_W + x) * 3
            in0 = (x, y) in exp0
            in1 = (x, y) in exp1
            if not in0 and not in1:
                out[i] = UNEXPLORED[0]
                out[i + 1] = UNEXPLORED[1]
                out[i + 2] = UNEXPLORED[2]
                continue
            r, g, b = base_rgb[i], base_rgb[i + 1], base_rgb[i + 2]
            if in0 and in1:
                r, g, b = blend_px((r, g, b), BOTH_TINT, 72)
            elif in0:
                r, g, b = blend_px((r, g, b), P0_TINT, 96)
            else:
                r, g, b = blend_px((r, g, b), P1_TINT, 96)
            out[i] = r
            out[i + 1] = g
            out[i + 2] = b
    return bytes(out)

def scale_rgb (rgb, sw, sh, dw, dh):
    out = bytearray(dw * dh * 3)
    for dy in range(dh):
        sy = int(dy * sh / dh)
        if sy >= sh:
            sy = sh - 1
        for dx in range(dw):
            sx = int(dx * sw / dw)
            if sx >= sw:
                sx = sw - 1
            si = (sy * sw + sx) * 3
            di = (dy * dw + dx) * 3
            out[di] = rgb[si]
            out[di + 1] = rgb[si + 1]
            out[di + 2] = rgb[si + 2]
    return bytes(out)

#================================================================================================================================#
#=> - ExploreDistantValid -
#================================================================================================================================#

class ExploreDistantValid:
    def __init__ (m, root):
        m.root = root
        m.root.title("explore_distant validation")
        m.maps = {}
        for name, path in zip(MAP_NAMES, (TERR_PATH, CLIM_PATH, RIV_PATH)):
            w, h, rgb = load_ppm_rgb(path)
            if w != MAP_W or h != MAP_H:
                raise ValueError("%s map size %dx%d" % (name, w, h))
            m.maps[name] = rgb
        m.snapshots = parse_trace(TRACE_PATH)
        m.max_turn = max(0, len(m.snapshots) - 1)
        m.map_idx = 0
        m.speed_idx = 1
        m.turn = 0
        m.playing = False
        m.view_x = 0.0
        m.view_y = 0.0
        m.px_per_tile = 1.0
        m.drag_x = 0
        m.drag_y = 0
        m.drag_vx = 0.0
        m.drag_vy = 0.0
        m.photo = None
        m.frame_cache = {}
        m.build_ui()
        m.render()
        m.root.after(SPEEDS_MS[m.speed_idx], m.tick)

    def build_ui (m):
        panel = ttk.Frame(m.root, padding=6)
        panel.pack(side=tk.TOP, fill=tk.X)
        m.map_btn = ttk.Button(panel, text="map: terrain", command=m.toggle_map)
        m.map_btn.pack(side=tk.LEFT, padx=4)
        m.play_btn = ttk.Button(panel, text="play", command=m.toggle_play)
        m.play_btn.pack(side=tk.LEFT, padx=4)
        m.speed_btn = ttk.Button(panel, text="speed: 100ms", command=m.toggle_speed)
        m.speed_btn.pack(side=tk.LEFT, padx=4)
        ttk.Label(panel, text="turn").pack(side=tk.LEFT, padx=(12, 4))
        m.turn_var = tk.IntVar(value=0)
        m.turn_slider = ttk.Scale(
            panel,
            from_=0,
            to=m.max_turn,
            orient=tk.HORIZONTAL,
            length=320,
            variable=m.turn_var,
            command=m.on_turn_slider,
        )
        m.turn_slider.pack(side=tk.LEFT, padx=4)
        m.turn_lbl = ttk.Label(panel, text="0 / %d" % m.max_turn)
        m.turn_lbl.pack(side=tk.LEFT, padx=4)
        m.stat_lbl = ttk.Label(panel, text="")
        m.stat_lbl.pack(side=tk.LEFT, padx=12)
        ttk.Label(panel, text="drag=pan  wheel=zoom").pack(side=tk.RIGHT, padx=4)
        m.canvas = tk.Canvas(
            m.root,
            width=CANVAS_W,
            height=CANVAS_H,
            highlightthickness=0,
            bg="#101018",
        )
        m.canvas.pack(side=tk.TOP)
        m.canvas.bind("<ButtonPress-1>", m.on_drag_start)
        m.canvas.bind("<B1-Motion>", m.on_drag_move)
        m.canvas.bind("<MouseWheel>", m.on_wheel)
        m.canvas.bind("<Button-4>", m.on_wheel_up)
        m.canvas.bind("<Button-5>", m.on_wheel_down)

    def toggle_map (m):
        m.map_idx = (m.map_idx + 1) % len(MAP_NAMES)
        m.map_btn.config(text="map: " + MAP_NAMES[m.map_idx])
        m.frame_cache.clear()
        m.render()

    def toggle_play (m):
        m.playing = not m.playing
        m.play_btn.config(text="pause" if m.playing else "play")

    def toggle_speed (m):
        m.speed_idx = (m.speed_idx + 1) % len(SPEEDS_MS)
        m.speed_btn.config(text="speed: %dms" % SPEEDS_MS[m.speed_idx])

    def on_turn_slider (m, _value):
        m.turn = int(float(m.turn_var.get()))
        m.turn_lbl.config(text="%d / %d" % (m.turn, m.max_turn))
        m.render()

    def clamp_view (m):
        tiles_w = CANVAS_W / m.px_per_tile
        tiles_h = CANVAS_H / m.px_per_tile
        max_x = max(0.0, MAP_W - tiles_w)
        max_y = max(0.0, MAP_H - tiles_h)
        m.view_x = min(max(m.view_x, 0.0), max_x)
        m.view_y = min(max(m.view_y, 0.0), max_y)

    def on_drag_start (m, event):
        m.drag_x = event.x
        m.drag_y = event.y
        m.drag_vx = m.view_x
        m.drag_vy = m.view_y

    def on_drag_move (m, event):
        dx = event.x - m.drag_x
        dy = event.y - m.drag_y
        m.view_x = m.drag_vx - dx / m.px_per_tile
        m.view_y = m.drag_vy - dy / m.px_per_tile
        m.clamp_view()
        m.render()

    def zoom_at (m, mx, my, factor):
        old = m.px_per_tile
        new = min(16.0, max(0.25, old * factor))
        if abs(new - old) < 1e-6:
            return
        map_x = m.view_x + mx / old
        map_y = m.view_y + my / old
        m.px_per_tile = new
        m.view_x = map_x - mx / new
        m.view_y = map_y - my / new
        m.clamp_view()
        m.render()

    def on_wheel (m, event):
        factor = 1.15 if event.delta > 0 else (1.0 / 1.15)
        m.zoom_at(event.x, event.y, factor)

    def on_wheel_up (m, event):
        m.zoom_at(event.x, event.y, 1.15)

    def on_wheel_down (m, event):
        m.zoom_at(event.x, event.y, 1.0 / 1.15)

    def frame_rgb (m, turn):
        key = (MAP_NAMES[m.map_idx], turn)
        if key not in m.frame_cache:
            base = m.maps[MAP_NAMES[m.map_idx]]
            exp = m.snapshots[min(turn, len(m.snapshots) - 1)]
            m.frame_cache[key] = compose_frame(base, exp[0], exp[1])
        return m.frame_cache[key]

    def render (m):
        turn = m.turn
        exp = m.snapshots[min(turn, len(m.snapshots) - 1)]
        m.stat_lbl.config(
            text="p0=%d p1=%d zoom=%.2fx" % (len(exp[0]), len(exp[1]), m.px_per_tile)
        )
        full = m.frame_rgb(turn)
        tiles_w = CANVAS_W / m.px_per_tile
        tiles_h = CANVAS_H / m.px_per_tile
        x0 = int(m.view_x)
        y0 = int(m.view_y)
        x1 = min(MAP_W, int(m.view_x + tiles_w) + 1)
        y1 = min(MAP_H, int(m.view_y + tiles_h) + 1)
        vw = max(1, x1 - x0)
        vh = max(1, y1 - y0)
        buf = bytearray(vw * vh * 3)
        for vy in range(vh):
            sy = y0 + vy
            if sy >= MAP_H:
                break
            src_off = (sy * MAP_W + x0) * 3
            dst_off = vy * vw * 3
            row_len = vw * 3
            buf[dst_off:dst_off + row_len] = full[src_off:src_off + row_len]
        if vw != CANVAS_W or vh != CANVAS_H:
            scaled = scale_rgb(buf, vw, vh, CANVAS_W, CANVAS_H)
            ppm = rgb_view_ppm(CANVAS_W, CANVAS_H, scaled)
        else:
            ppm = rgb_view_ppm(vw, vh, bytes(buf))
        m.photo = tk.PhotoImage(data=ppm, format="PPM")
        m.canvas.delete("all")
        m.canvas.create_image(0, 0, anchor=tk.NW, image=m.photo)

    def tick (m):
        if m.playing and m.turn < m.max_turn:
            m.turn += 1
            m.turn_var.set(m.turn)
            m.turn_lbl.config(text="%d / %d" % (m.turn, m.max_turn))
            m.render()
        elif m.playing and m.turn >= m.max_turn:
            m.playing = False
            m.play_btn.config(text="play")
        m.root.after(SPEEDS_MS[m.speed_idx], m.tick)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    os.chdir(THIS_DIR)
    root = tk.Tk()
    ExploreDistantValid(root)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
