#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import tkinter as tk
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: LineHandler
#================================================================================================================================#

class LineHandler(tk.Canvas):

    def __find_nearby_point (m, x, y, threshold=3):
        for i, (px, py) in enumerate(m.points):
            dist = ((x - px) ** 2 + (y - py) ** 2) ** 0.5
            if dist <= threshold:
                return i
        return -1

    def __draw (m):
        m.delete("all")
        m.create_rectangle(0, 0, m.canvas_w, m.canvas_h, fill="", outline="black", width=2)
        mid_y = m.canvas_h * 0.5
        m.create_line(0, mid_y, m.canvas_w, mid_y, fill="gray", width=1)
        
        lower_half_top = mid_y
        lower_half_bottom = m.canvas_h
        lower_half_center_y = (lower_half_top + lower_half_bottom) / 2
        center_x = m.canvas_w / 2
        
        diamond_points = [
            center_x, lower_half_top,
            m.canvas_w, lower_half_center_y,
            center_x, lower_half_bottom,
            0, lower_half_center_y
        ]
        m.create_polygon(diamond_points, fill="", outline="gray", width=1)
        
        if len(m.points) > 1:
            for i in range(len(m.points) - 1):
                x1, y1 = m.points[i]
                x2, y2 = m.points[i + 1]
                m.create_line(x1, y1, x2, y2, fill="yellow", width=m.pencil_size)
        
        for i, (x, y) in enumerate(m.points):
            if i in m.fixed_indices:
                m.create_oval(x - 5, y - 5, x + 5, y + 5, fill="red", outline="darkred", width=2)
            else:
                m.create_oval(x - 4, y - 4, x + 4, y + 4, fill="white", outline="black", width=1)

    def __handle_click (m, event):
        x, y = event.x, event.y
        idx = m.__find_nearby_point(x, y, threshold=10)
        if idx != -1 and idx not in m.fixed_indices:
            m.points.pop(idx)
            for i, fixed_idx in enumerate(m.fixed_indices):
                if fixed_idx > idx:
                    m.fixed_indices[i] = fixed_idx - 1
            m.__draw()

    def __handle_press (m, event):
        x, y = event.x, event.y
        m.dragged_point_idx = m.__find_nearby_point(x, y)
        if m.dragged_point_idx == -1:
            m.points.append((x, y))
            sorted_indices = sorted(range(len(m.points)), key=lambda i: m.points[i][0])
            m.points = [m.points[i] for i in sorted_indices]
            new_fixed = []
            for fixed_idx in m.fixed_indices:
                new_fixed.append(sorted_indices.index(fixed_idx))
            m.fixed_indices = new_fixed
            m.__draw()

    def __handle_drag (m, event):
        if m.dragged_point_idx != -1 and m.dragged_point_idx not in m.fixed_indices:
            x, y = event.x, event.y
            m.points[m.dragged_point_idx] = (x, y)
            sorted_indices = sorted(range(len(m.points)), key=lambda i: m.points[i][0])
            m.points = [m.points[i] for i in sorted_indices]
            m.dragged_point_idx = sorted_indices.index(m.dragged_point_idx)
            new_fixed = []
            for fixed_idx in m.fixed_indices:
                new_fixed.append(sorted_indices.index(fixed_idx))
            m.fixed_indices = new_fixed
            m.__draw()

    def __handle_release (m, event):
        m.dragged_point_idx = -1

    def __init__ (m, parent, bg, pencil_size):
        tk.Canvas.__init__(m, parent, width=400, height=400, bg=bg, highlightthickness=2, highlightbackground="black")
        m.pencil_size = pencil_size
        m.points = []
        m.fixed_indices = []
        m.dragged_point_idx = -1
        m.canvas_w = 400
        m.canvas_h = 400
        
        m.bind("<Button-1>", m.__handle_press)
        m.bind("<Button-3>", m.__handle_click)
        m.bind("<B1-Motion>", m.__handle_drag)
        m.bind("<ButtonRelease-1>", m.__handle_release)
        
        m.after_idle(m.__init_points)

    def __init_points (m):
        canvas_w = 400
        canvas_h = 400
        fixed_y = canvas_h * 0.75
        m.points.append((0, fixed_y))
        m.points.append((canvas_w, fixed_y))
        m.fixed_indices = [0, 1]
        m.__draw()

    def get_points (m):
        return m.points

#================================================================================================================================#
#=> - Class: HeightMorpher
#================================================================================================================================#

class HeightMorpher(tk.Canvas):

    def __get_y_at_x (m, x):
        if x <= m.line_points[0][0]:
            return m.line_points[0][1]
        if x >= m.line_points[-1][0]:
            return m.line_points[-1][1]
        
        for i in range(len(m.line_points) - 1):
            x1, y1 = m.line_points[i]
            x2, y2 = m.line_points[i + 1]
            if x1 <= x <= x2:
                if x1 == x2:
                    return y1
                t = (x - x1) / (x2 - x1)
                return y1 + t * (y2 - y1)
        return m.line_points[-1][1]

    def __init__ (m, parent):
        tk.Canvas.__init__(m, parent, width=400, height=400, bg="darkgray", highlightthickness=2, highlightbackground="black")
        m.img = None
        m.morphed_img = None
        m.line_points = []
        m.y_values = []
        m.canvas_w = 400
        m.canvas_h = 400

    def update_morph (m, img, line_points):
        m.img = img
        m.line_points = line_points
        img_w, img_h = img.size

        m.canvas_w = 400
        m.canvas_h = 400    
        m.y_values = []
        for x in range(img_w):
            canvas_x = (x / img_w) * m.canvas_w
            canvas_y = m.__get_y_at_x(canvas_x)
            m.y_values.append(canvas_y)
        
        img_rgba = img.convert("RGBA")
        i_array = np.array(img_rgba, dtype=np.int16)
        
        result_array = np.zeros((img_h, img_w, 4), dtype=np.int16)
        BEND_CURVE_FACTOR = 0.8
        mid_y = int(img_h * 0.5)
        for x in range(img_w):
            canvas_y_val = m.y_values[x]
            factor = (300 - canvas_y_val) / (300 - 200)
            factor = max(0.0, min(1.0, factor))
            
            if i_array[mid_y, x, 3] > 127:
                gray_val_mid = int(i_array[mid_y, x, 0] * 0.299 + i_array[mid_y, x, 1] * 0.587 + i_array[mid_y, x, 2] * 0.114)
                tentpole_val = int(gray_val_mid * factor)
                tentpole_val = max(0, min(255, tentpole_val))
                delta1 = tentpole_val - gray_val_mid
                low_y = mid_y
                for y in range(mid_y, img_h):
                    if i_array[y, x, 3] > 127:
                        low_y = y
                    else: break
                if low_y > mid_y and i_array[low_y, x, 3] > 127:
                    gray_val_low = int(i_array[low_y, x, 0] * 0.299 + i_array[low_y, x, 1] * 0.587 + i_array[low_y, x, 2] * 0.114)
                    delta2 = 0 - gray_val_low
                    height = low_y - mid_y
                    if height > 0:
                        for i in range(height + 1):
                            y = mid_y + i
                            if i_array[y, x, 3] > 127:
                                t = i / height
                                t_curved = t ** BEND_CURVE_FACTOR
                                delta = delta1 + t_curved * (delta2 - delta1)
                                gray_val = int(i_array[y, x, 0] * 0.299 + i_array[y, x, 1] * 0.587 + i_array[y, x, 2] * 0.114)
                                new_val = gray_val + int(delta)
                                result_array[y, x, 0] = new_val
                                result_array[y, x, 1] = new_val
                                result_array[y, x, 2] = new_val
                                result_array[y, x, 3] = i_array[y, x, 3]
                
                top_y = mid_y
                for y in range(mid_y, -1, -1):
                    if i_array[y, x, 3] > 127:
                        top_y = y
                    else: break
                if top_y < mid_y and i_array[top_y, x, 3] > 127:
                    gray_val_top = int(i_array[top_y, x, 0] * 0.299 + i_array[top_y, x, 1] * 0.587 + i_array[top_y, x, 2] * 0.114)
                    delta2_top = 0 - gray_val_top
                    height_top = mid_y - top_y
                    if height_top > 0:
                        for i in range(height_top + 1):
                            y = mid_y - i
                            if i_array[y, x, 3] > 127:
                                t = i / height_top
                                t_curved = t ** BEND_CURVE_FACTOR
                                delta = delta1 + t_curved * (delta2_top - delta1)
                                gray_val = int(i_array[y, x, 0] * 0.299 + i_array[y, x, 1] * 0.587 + i_array[y, x, 2] * 0.114)
                                new_val = gray_val + int(delta)
                                result_array[y, x, 0] = new_val
                                result_array[y, x, 1] = new_val
                                result_array[y, x, 2] = new_val
                                result_array[y, x, 3] = i_array[y, x, 3]
        
        rgb_min = result_array[:, :, :3].min()
        rgb_max = result_array[:, :, :3].max()
        if rgb_max > rgb_min:
            result_array[:, :, :3] = ((result_array[:, :, :3] - rgb_min) / (rgb_max - rgb_min) * 255).astype(np.int16)
        result_array = np.clip(result_array, 0, 255).astype(np.uint8)
        morphed_img = Image.fromarray(result_array, mode="RGBA")
        morphed_img = morphed_img.resize((400, 400), Image.Resampling.LANCZOS)
        m.morphed_img = morphed_img
        
        m.delete("all")
        photo = ImageTk.PhotoImage(morphed_img)
        m.create_image(200, 200, anchor=tk.CENTER, image=photo)
        m.image = photo

    def get_height (m, x):
        if m.img is None:
            return 0
        return m.img.getpixel((x, 0))

#================================================================================================================================#
#=> - Class: MtnRidgeGen
#================================================================================================================================#

class MtnRidgeGen:

    def __crop_diamond (m, img):
        img_w, img_h = img.size
        center_x = img_w / 2
        top_y = img_h * 0.25
        mid_y = img_h * 0.5
        low_y = img_h * 0.75
        
        img_rgba = img.convert("RGBA")
        i_array = np.array(img_rgba)
        
        for y in range(img_h):
            for x in range(img_w):
                in_diamond = False
                if top_y <= y <= mid_y:
                    t = (y - top_y) / (mid_y - top_y) if mid_y > top_y else 0
                    width = img_w * t
                    left_bound = center_x - width / 2
                    right_bound = center_x + width / 2
                    if int(left_bound) <= x <= int(right_bound):
                        in_diamond = True
                elif mid_y < y <= low_y:
                    t = (low_y - y) / (low_y - mid_y) if low_y > mid_y else 0
                    width = img_w * t
                    left_bound = center_x - width / 2
                    right_bound = center_x + width / 2
                    if int(left_bound) <= x <= int(right_bound):
                        in_diamond = True
                
                if not in_diamond:
                    i_array[y, x, 3] = 0
        
        return Image.fromarray(i_array)

    def __update_left_canvas (m):
        if m.left_img is None:
            return
        m.cnv_left.delete("all")
        
        diamond_img = m.__crop_diamond(m.left_img)
        diamond_img = diamond_img.resize((400, 400), Image.Resampling.LANCZOS)
        
        photo = ImageTk.PhotoImage(diamond_img)
        m.cnv_left.create_image(200, 200, anchor=tk.CENTER, image=photo)
        m.cnv_left.image = photo

    def __save_image (m):
        if m.cnv_mid.morphed_img is None:
            return
        img = m.cnv_mid.morphed_img
        img_w, img_h = img.size
        
        stretched = img.resize((img_w, img_h * 2), Image.Resampling.LANCZOS)
        
        crop_top = img_h // 2
        crop_bottom = img_h * 2 - img_h // 2
        cropped = stretched.crop((0, crop_top, img_w, crop_bottom))
        
        save_path = m.image_path.replace(".png", "_cr.png")
        cropped.save(save_path)
        print(f"Saved image to: {save_path}")

    def __save_line (m):
        points = m.cnv_right.get_points()
        save_path = m.image_path.replace(".png", "_cr.line")
        with open(save_path, "w") as f:
            for x, y in points:
                f.write(f"{x} {y}\n")
        print(f"Saved line data to: {save_path}")

    def __generate_main (m):
        diamond_img = m.__crop_diamond(m.left_img)
        m.cnv_mid.update_morph(diamond_img, m.cnv_right.get_points())

    def __init__ (m, image_path):
        m.root = tk.Tk()
        m.root.title("Mountain Ridge Generator")
        m.image_path = image_path
        m.left_img = Image.open(image_path)
        img_w, img_h = m.left_img.size
        m.mid_y = img_h // 2
        m.pencil_size = 3
        m.line_segments = 30
        
        m.btn_frame = tk.Frame(m.root)
        m.btn_frame.pack(side=tk.TOP, fill=tk.X)
        
        m.btn_gen_main = tk.Button(m.btn_frame, text="Shape", command=m.__generate_main)
        m.btn_gen_main.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.btn_save_img = tk.Button(m.btn_frame, text="Save Image", command=m.__save_image)
        m.btn_save_img.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.btn_save_line = tk.Button(m.btn_frame, text="Save Line", command=m.__save_line)
        m.btn_save_line.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.canvas_frame = tk.Frame(m.root)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        m.cnv_left = tk.Canvas(m.canvas_frame, width=400, height=400, bg="darkgray", highlightbackground="black")
        m.cnv_left.configure(highlightthickness=2)
        m.cnv_left.pack(side=tk.LEFT, fill=tk.NONE)
        m.cnv_mid = HeightMorpher(m.canvas_frame)
        m.cnv_mid.pack(side=tk.LEFT, fill=tk.NONE)
        m.cnv_right = LineHandler(m.canvas_frame, bg="darkgray", pencil_size=m.pencil_size)
        m.cnv_right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        m.__update_left_canvas()

    def run (m):
        m.root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    image_path = "/home/w/Projects/img-content/raw_mtn_hd000.png"
    gen = MtnRidgeGen(image_path)
    gen.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
