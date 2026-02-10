#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from PIL import Image, ImageTk, ImageDraw
import random

#================================================================================================================================#
#=> - Class: MtnTexTester
#================================================================================================================================#

class MtnTexTester:

    def __init__ (m, root, topLight, topDark, bottomLight, bottomDark):
        m.root = root
        m.texturesTopLight = topLight
        m.texturesTopDark = topDark
        m.texturesBottomLight = bottomLight
        m.texturesBottomDark = bottomDark
        m.idxTopLight = 0
        m.idxTopDark = 0
        m.idxBottomLight = 0
        m.idxBottomDark = 0
        m.sizes = [100, 200, 400, 800]
        m.idxValue = 0
        
        m.row1 = tk.Frame(root)
        m.row1.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.btn1_left = tk.Button(m.row1, text="<", command=lambda: m.__cycle("Top light", False), width=5)
        m.btn1_left.pack(side=tk.LEFT, padx=5)
        m.btn1_right = tk.Button(m.row1, text=">", command=lambda: m.__cycle("Top light", True), width=5)
        m.btn1_right.pack(side=tk.LEFT, padx=5)
        m.lbl1 = tk.Label(m.row1, text="Top light")
        m.lbl1.pack(side=tk.LEFT, padx=5)
        
        m.row2 = tk.Frame(root)
        m.row2.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.btn2_left = tk.Button(m.row2, text="<", command=lambda: m.__cycle("Top dark", False), width=5)
        m.btn2_left.pack(side=tk.LEFT, padx=5)
        m.btn2_right = tk.Button(m.row2, text=">", command=lambda: m.__cycle("Top dark", True), width=5)
        m.btn2_right.pack(side=tk.LEFT, padx=5)
        m.lbl2 = tk.Label(m.row2, text="Top dark")
        m.lbl2.pack(side=tk.LEFT, padx=5)
        
        m.row3 = tk.Frame(root)
        m.row3.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.btn3_left = tk.Button(m.row3, text="<", command=lambda: m.__cycle("Bottom light", False), width=5)
        m.btn3_left.pack(side=tk.LEFT, padx=5)
        m.btn3_right = tk.Button(m.row3, text=">", command=lambda: m.__cycle("Bottom light", True), width=5)
        m.btn3_right.pack(side=tk.LEFT, padx=5)
        m.lbl3 = tk.Label(m.row3, text="Bottom light")
        m.lbl3.pack(side=tk.LEFT, padx=5)
        
        m.row4 = tk.Frame(root)
        m.row4.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.btn4_left = tk.Button(m.row4, text="<", command=lambda: m.__cycle("Bottom dark", False), width=5)
        m.btn4_left.pack(side=tk.LEFT, padx=5)
        m.btn4_right = tk.Button(m.row4, text=">", command=lambda: m.__cycle("Bottom dark", True), width=5)
        m.btn4_right.pack(side=tk.LEFT, padx=5)
        m.lbl4 = tk.Label(m.row4, text="Bottom dark")
        m.lbl4.pack(side=tk.LEFT, padx=5)
        
        m.row5 = tk.Frame(root)
        m.row5.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.btn5_left = tk.Button(m.row5, text="<", command=lambda: m.__cycleValue(False), width=5)
        m.btn5_left.pack(side=tk.LEFT, padx=5)
        m.btn5_right = tk.Button(m.row5, text=">", command=lambda: m.__cycleValue(True), width=5)
        m.btn5_right.pack(side=tk.LEFT, padx=5)
        m.lbl5 = tk.Label(m.row5, text="100")
        m.lbl5.pack(side=tk.LEFT, padx=5)
        
        m.row6 = tk.Frame(root)
        m.row6.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.btn6 = tk.Button(m.row6, text="Draw", command=lambda: m.__redraw(), width=10)
        m.btn6.pack(side=tk.LEFT, padx=5)

        m.canvas = tk.Canvas(root, width=800, height=600, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        m.x_center = 400
        m.y_center = 300
        m.__derive_tile_corners()
        m.__draw_tile()

    def __cycle (m, label, forward):
        if label == "Top light":
            lst = m.texturesTopLight
            idx = m.idxTopLight
        elif label == "Top dark":
            lst = m.texturesTopDark
            idx = m.idxTopDark
        elif label == "Bottom light":
            lst = m.texturesBottomLight
            idx = m.idxBottomLight
        else:
            lst = m.texturesBottomDark
            idx = m.idxBottomDark
        
        if not lst:
            return
        
        if forward:
            idx = (idx + 1) % len(lst)
        else:
            idx = (idx - 1) % len(lst)
        
        if label == "Top light":
            m.idxTopLight = idx
        elif label == "Top dark":
            m.idxTopDark = idx
        elif label == "Bottom light":
            m.idxBottomLight = idx
        else:
            m.idxBottomDark = idx
        
        m.__derive_tile_corners()
        m.__draw_tile()

    def __cycleValue (m, forward):
        if forward:
            m.idxValue = (m.idxValue + 1) % len(m.sizes)
        else:
            m.idxValue = (m.idxValue - 1) % len(m.sizes)
        m.lbl5.config(text=str(m.sizes[m.idxValue]))
        m.__derive_tile_corners()
        m.__draw_tile()

    def __redraw (m):
        m.__derive_tile_corners()
        m.__draw_tile()

    def __derive_tile_corners (m):
        m.tile_w = m.sizes[m.idxValue]
        m.tile_h = m.tile_w // 2
        m.tile_x1 = m.x_center - m.tile_w / 2
        m.tile_y1 = m.y_center - m.tile_h / 2
        m.tile_x2 = m.x_center + m.tile_w / 2
        m.tile_y2 = m.y_center + m.tile_h / 2
        m.top = (m.x_center, m.y_center - m.tile_h / 2)
        m.right = (m.x_center + m.tile_w / 2, m.y_center)
        m.bottom = (m.x_center, m.y_center + m.tile_h / 2)
        m.left = (m.x_center - m.tile_w / 2, m.y_center)
        
        rand_xo = random.randint(0, m.sizes[m.idxValue]//10) - m.sizes[m.idxValue]//20
        rand_yo = random.randint(0, m.sizes[m.idxValue]//10) - m.sizes[m.idxValue]//20
        m.top = (m.top[0] + rand_xo, m.top[1] + rand_yo)
        rand_xo = random.randint(0, m.sizes[m.idxValue]//2) - m.sizes[m.idxValue]//4
        m.bottom = (m.bottom[0] + rand_xo, m.bottom[1])
        
        m.center = ((m.top[0] + m.bottom[0]) / 2, (m.top[1] + m.bottom[1]) / 2)
        m.mid_left = ((m.top[0] + m.left[0]) / 2, (m.top[1] + m.left[1]) / 2)
        m.mid_right = ((m.top[0] + m.right[0]) / 2, (m.top[1] + m.right[1]) / 2)

    def __draw_textured_polygon (m, points, texture_path):
        x_coords = [p[0] for p in points]
        y_coords = [p[1] for p in points]
        min_x, max_x = int(min(x_coords)), int(max(x_coords))
        min_y, max_y = int(min(y_coords)), int(max(y_coords))
        w, h = max_x - min_x, max_y - min_y

        mask = Image.new("L", (w, h), 0)
        draw = ImageDraw.Draw(mask)
        poly_points = [(x - min_x, y - min_y) for x, y in points]
        draw.polygon(poly_points, fill=255)
        
        scale = m.sizes[m.idxValue] // m.sizes[0]
        tex_img = Image.open(texture_path).convert("RGB")
        tex_img = tex_img.resize((w * scale, h * scale), Image.Resampling.LANCZOS)
        
        result = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        result.paste(tex_img, (0, 0))
        result.putalpha(mask)
        
        photo = ImageTk.PhotoImage(result)
        m.canvas.create_image(min_x, min_y, anchor=tk.NW, image=photo)
        return photo

    def __draw_tile (m):
        m.canvas.delete("all")
        m.canvas.images = []
        poly_line_width = 0

        pts = [m.top, m.mid_left, m.center]
        photo = m.__draw_textured_polygon(pts, m.texturesTopLight[m.idxTopLight])
        if photo:
            m.canvas.images.append(photo)
        if poly_line_width > 0:
            m.canvas.create_polygon(
                pts[0][0], pts[0][1],
                pts[1][0], pts[1][1],
                pts[2][0], pts[2][1],
                outline="black", fill="", width=poly_line_width
            )

        pts = [m.top, m.mid_right, m.center]
        photo = m.__draw_textured_polygon(pts, m.texturesTopDark[m.idxTopDark])
        if photo:
            m.canvas.images.append(photo)
        if poly_line_width > 0:
            m.canvas.create_polygon(
                pts[0][0], pts[0][1],
                pts[1][0], pts[1][1],
                pts[2][0], pts[2][1],
                outline="black", fill="", width=poly_line_width
            )

        pts = [m.center, m.mid_left, m.left, m.bottom]
        photo = m.__draw_textured_polygon(pts, m.texturesBottomLight[m.idxBottomLight])
        if photo:
            m.canvas.images.append(photo)
        if poly_line_width > 0:
            m.canvas.create_polygon( pts[0][0], pts[0][1],
                pts[1][0], pts[1][1],
                pts[2][0], pts[2][1],
                pts[3][0], pts[3][1],
                outline="black", fill="", width=poly_line_width
            )

        pts = [m.center, m.mid_right, m.right, m.bottom]
        photo = m.__draw_textured_polygon(pts, m.texturesBottomDark[m.idxBottomDark])
        if photo:
            m.canvas.images.append(photo)
        if poly_line_width > 0:
            m.canvas.create_polygon(
                pts[0][0], pts[0][1],
                pts[1][0], pts[1][1],
                pts[2][0], pts[2][1],
                pts[3][0], pts[3][1],
                outline="black", fill="", width=poly_line_width
            )

#================================================================================================================================#
#=> - Helper Functions -
#================================================================================================================================#

def get_texture_paths(base_path):
    topLight, topDark, bottomLight, bottomDark = [], [], [], []
    for root, dirs, files in os.walk(base_path):
        for f in files:
            if not f.endswith(".png"):
                continue
            f_lower = f.lower()
            if "texture_blurred" not in f_lower:
                continue
            has_dark = "dark" in f_lower
            has_light = "light" in f_lower
            has_top = "top" in f_lower
            has_bottom = "bottom" in f_lower
            full_path = os.path.join(root, f)
            if has_top and has_light:
                topLight.append(full_path)
            elif has_top and has_dark:
                topDark.append(full_path)
            elif has_bottom and has_light:
                bottomLight.append(full_path)
            elif has_bottom and has_dark:
                bottomDark.append(full_path)
    return topLight, topDark, bottomLight, bottomDark

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    base_path = "/home/w/Projects/img-content/"
    topLight, topDark, bottomLight, bottomDark = get_texture_paths(base_path)
    root = tk.Tk()
    root.title("Mountain Texture Tester")
    tester = MtnTexTester(root, topLight, topDark, bottomLight, bottomDark)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
