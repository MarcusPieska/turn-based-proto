#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
import math
import os
import random

#================================================================================================================================#
#=> - Globals -
#================================================================================================================================#

CANVAS_WIDTH  = 800
CANVAS_HEIGHT = 400

P1_X, P1_Y = 200, 200
P2_X, P2_Y = 600, 200

#================================================================================================================================#
#=> - Line Helpers -
#================================================================================================================================#

def _midpoint_displacement (p0, p1, depth, roughness, drama_chance):
    if depth == 0:
        return [p0, p1]
    x0, y0 = p0
    x1, y1 = p1
    mx = (x0 + x1) * 0.5
    my = (y0 + y1) * 0.5
    dx = x1 - x0
    dy = y1 - y0
    length = math.hypot(dx, dy)
    if length == 0:
        nx, ny = 0.0, 0.0
    else:
        nx = -dy / length
        ny = dx / length
    disp_scale = (length * roughness) / (2.0 ** depth)
    disp = (random.random() - 0.5) * 2.0 * disp_scale
    if random.random() < drama_chance:
        disp *= random.uniform(3.0, 4.0)
    mx += nx * disp
    my += ny * disp
    left = _midpoint_displacement(p0, (mx, my), depth - 1, roughness, drama_chance)
    right = _midpoint_displacement((mx, my), p1, depth - 1, roughness, drama_chance)
    return left[:-1] + right

def _chaikin_smooth (points, iterations=2):
    pts = list(points)
    for _ in range(iterations):
        if len(pts) < 3:
            break
        new_pts = [pts[0]]
        for i in range(len(pts) - 1):
            x0, y0 = pts[i]
            x1, y1 = pts[i + 1]
            qx = 0.75 * x0 + 0.25 * x1
            qy = 0.75 * y0 + 0.25 * y1
            rx = 0.25 * x0 + 0.75 * x1
            ry = 0.25 * y0 + 0.75 * y1
            new_pts.append((qx, qy))
            new_pts.append((rx, ry))
        new_pts.append(pts[-1])
        pts = new_pts
    return pts

def generate_river_path (p1, p2, complexity=10, roughness=1.5, drama_chance=0.1):
    depth = max(1, int(complexity))
    raw_points = _midpoint_displacement(p1, p2, depth, roughness, drama_chance)
    smooth_points = _chaikin_smooth(raw_points, iterations=2)
    dense_points = []
    for i in range(len(smooth_points) - 1):
        x0, y0 = smooth_points[i]
        x1, y1 = smooth_points[i + 1]
        dx = x1 - x0
        dy = y1 - y0
        dist = math.hypot(dx, dy)
        steps = max(1, int(dist))
        for s in range(steps):
            t = s / steps
            xt = x0 + dx * t
            yt = y0 + dy * t
            dense_points.append((xt, yt))
    if smooth_points:
        dense_points.append(smooth_points[-1])
    result = []
    last_px, last_py = None, None
    for x, y in dense_points:
        px = int(round(x))
        py = int(round(y))
        if last_px is None or px != last_px or py != last_py:
            result.append((px, py))
            last_px, last_py = px, py
    return result

def draw_river (cnv, points, outer_r=4, inner_r=2, outer_c="#367bb7", inner_c="#7fb8ff", tag="river"):
    for (x, y) in points[1:-1]:
        cnv.create_oval(x - outer_r, y - outer_r, x + outer_r, y + outer_r, outline=outer_c, fill=outer_c, tags=tag)
    for (x, y) in points:
        cnv.create_oval(x - inner_r, y - inner_r, x + inner_r, y + inner_r, outline=inner_c, fill=inner_c, tags=tag)

#================================================================================================================================#
#=> - Class: LineViewer
#================================================================================================================================#

class LineViewer:

    def __drawLine (m):
        m.canv_main.delete("river")
        comp = m.complexity.get()
        rough = m.roughness.get()
        drama  = m.drama.get()
        x0, y0 = P1_X, P1_Y
        dist = m.point_dist.get()
        x1, y1 = x0 + dist, P2_Y
        points = generate_river_path((x0, y0), (x1, y1), complexity=comp, roughness=rough, drama_chance=drama)
        draw_river(m.canv_main, points, outer_r=2, inner_r=1, outer_c="#367bb7", inner_c="#7fb8ff", tag="river")

    def __displayScene (m):
        m.__drawLine ()

    def __init__ (m, root):
        m.root = root

        m.canv_w = CANVAS_WIDTH
        m.canv_h = CANVAS_HEIGHT

        m.btn_frm = tk.Frame(root)
        m.btn_frm.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.slider_frm = tk.Frame(root)
        m.slider_frm.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)

        m.draw_btn = tk.Button(m.btn_frm, text="Draw River", command=m.__drawLine, width=15)
        m.draw_btn.pack(side=tk.LEFT, padx=5)

        m.complexity = tk.DoubleVar()
        m.complexity.set(10.0)
        m.roughness = tk.DoubleVar()
        m.roughness.set(1.5)
        m.drama = tk.DoubleVar()
        m.drama.set(0.1)
        m.point_dist = tk.DoubleVar()
        m.point_dist.set(P2_X - P1_X)

        m.comp_slider = tk.Scale(m.slider_frm, from_=3, to=12, resolution=1, orient=tk.HORIZONTAL, variable=m.complexity)
        m.comp_slider.configure(command=lambda v: m.__displayScene (), label="Complexity", length=160)
        m.comp_slider.pack(side=tk.LEFT, padx=5)

        m.rough_slider = tk.Scale(m.slider_frm, from_=0.5, to=3.0, resolution=0.1, orient=tk.HORIZONTAL, variable=m.roughness)
        m.rough_slider.configure(command=lambda v: m.__displayScene (), label="Roughness", length=160)
        m.rough_slider.pack(side=tk.LEFT, padx=5)

        m.drama_slider = tk.Scale(m.slider_frm, from_=0.0, to=0.4, resolution=0.01, orient=tk.HORIZONTAL, variable=m.drama)
        m.drama_slider.configure(command=lambda v: m.__displayScene (), label="Drama Chance", length=160)
        m.drama_slider.pack(side=tk.LEFT, padx=5)

        m.dist_slider = tk.Scale(m.slider_frm, from_=50, to=400, resolution=1, orient=tk.HORIZONTAL, variable=m.point_dist)
        m.dist_slider.configure(command=lambda v: m.__displayScene (), label="Distance", length=160)
        m.dist_slider.pack(side=tk.LEFT, padx=5)

        m.canv_frm = tk.Frame(root)
        m.canv_frm.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_main = tk.Canvas(m.canv_frm, width=m.canv_w, height=m.canv_h, bg="white")
        m.canv_main.pack(side=tk.LEFT, padx=5)

        bg_path = os.path.join(os.path.dirname(__file__), "bg.png")
        if os.path.exists(bg_path):
            m.bg_img = tk.PhotoImage(file=bg_path)
            m.canv_main.create_image(0, 0, anchor=tk.NW, image=m.bg_img, tags="bg")

        m.__displayScene ()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("River Path Tester")
    viewer = LineViewer (root)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#