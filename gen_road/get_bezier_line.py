#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
import math
import os

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

def get_line_pixels (x0, y0, x1, y1, ang0_deg, ang1_deg):
    pixels = []
    ang0 = math.radians(ang0_deg)
    ang1 = math.radians(ang1_deg)
    dx = x1 - x0
    dy = y1 - y0
    base_len = math.hypot(dx, dy)
    ctrl_len = base_len * 0.5
    cx0 = x0 + ctrl_len * math.cos(ang0)
    cy0 = y0 + ctrl_len * math.sin(ang0)
    cx1 = x1 + ctrl_len * math.cos(ang1)
    cy1 = y1 + ctrl_len * math.sin(ang1)
    steps = max(32, int(base_len))
    last_px, last_py = None, None
    for i in range(steps + 1):
        t = i / steps
        mt = 1.0 - t
        x = (
            mt * mt * mt * x0 +
            3 * mt * mt * t * cx0 +
            3 * mt * t * t * cx1 +
            t * t * t * x1
        )
        y = (
            mt * mt * mt * y0 +
            3 * mt * mt * t * cy0 +
            3 * mt * t * t * cy1 +
            t * t * t * y1
        )
        px = int(round(x))
        py = int(round(y))
        if last_px is None or px != last_px or py != last_py:
            pixels.append((px, py))
            last_px, last_py = px, py
    return pixels

def draw_road (cnv, pixels, outer_r=3, inner_r=2, outer_color="silver", inner_color="sandybrown", tag="line"):
    for (x, y) in pixels:
        cnv.create_oval(x - outer_r, y - outer_r, x + outer_r, y + outer_r, outline=outer_color, fill=outer_color, tags=tag)
    for (x, y) in pixels[1:-1]:
        cnv.create_oval(x - inner_r, y - inner_r, x + inner_r, y + inner_r, outline=inner_color, fill=inner_color, tags=tag)

#================================================================================================================================#
#=> - Class: LineViewer
#================================================================================================================================#

class LineViewer:

    def __drawLine (m):
        m.canv_main.delete("line")
        ang0 = m.left_angle.get()
        ang1 = m.right_angle.get()
        dist = m.point_dist.get()
        x0, y0 = P1_X, P1_Y
        x1, y1 = x0 + dist, P2_Y
        pixels = get_line_pixels(x0, y0, x1, y1, ang0, ang1)
        draw_road(m.canv_main, pixels, inner_r=1, outer_r=2, inner_color="darkgoldenrod", outer_color="saddlebrown")

    def __displayScene (m):
        m.__drawLine ()

    def __init__ (m, root):
        m.root = root

        m.canv_w = CANVAS_WIDTH
        m.canv_h = CANVAS_HEIGHT

        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.draw_btn = tk.Button(m.top_frame, text="Draw Line", command=m.__drawLine, width=15)
        m.draw_btn.pack(side=tk.LEFT, padx=5)
        m.left_angle = tk.DoubleVar()
        m.left_angle.set(0.0)
        m.right_angle = tk.DoubleVar()
        m.right_angle.set(180.0)
        
        m.left_slider = tk.Scale(m.top_frame, from_=-45, to=45, orient=tk.HORIZONTAL, variable=m.left_angle, length=200)
        m.left_slider.configure(command=lambda v: m.__displayScene (), label="Left Angle")
        m.left_slider.pack(side=tk.LEFT, padx=5)
        
        m.right_slider = tk.Scale(m.top_frame, from_=135, to=225, orient=tk.HORIZONTAL, variable=m.right_angle, length=200)
        m.right_slider.configure(command=lambda v: m.__displayScene (), label="Right Angle")
        m.right_slider.pack(side=tk.LEFT, padx=5)

        m.point_dist = tk.DoubleVar()
        m.point_dist.set(P2_X - P1_X)
        m.dist_slider = tk.Scale(m.top_frame, from_=50, to=400, orient=tk.HORIZONTAL, variable=m.point_dist,length=200)
        m.dist_slider.configure(command=lambda v: m.__displayScene (), label="Distance")
        m.dist_slider.pack(side=tk.LEFT, padx=5)

        m.canv_frame = tk.Frame(root)
        m.canv_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_main = tk.Canvas(m.canv_frame, width=m.canv_w, height=m.canv_h, bg="white")
        m.canv_main.pack(side=tk.LEFT, padx=5)
        #
        # Background image
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
    root.title("Bezier Line Tester")
    viewer = LineViewer (root)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#