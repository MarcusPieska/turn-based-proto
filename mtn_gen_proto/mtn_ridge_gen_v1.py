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
#=> - Class: MtnRidgeGen
#================================================================================================================================#

class MtnRidgeGen:

    MIN_LINE_SEGMENTS = 5
    MAX_LINE_SEGMENTS = 50

    def __update_left_canvas (m):
        if m.left_img is None:
            return
        m.cnv_left.delete("all")
        canvas_w = m.cnv_left.winfo_width()
        canvas_h = m.cnv_left.winfo_height()
        if canvas_w <= 1 or canvas_h <= 1:
            canvas_w, canvas_h = 400, 400
        img_w, img_h = m.left_img.size
        scale = min(canvas_w / img_w, canvas_h / img_h, 1.0)
        new_w = int(img_w * scale)
        new_h = int(img_h * scale)
        display_img = m.left_img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        photo = ImageTk.PhotoImage(display_img)
        m.cnv_left.create_image(canvas_w // 2, canvas_h // 2, anchor=tk.CENTER, image=photo)
        m.cnv_left.image = photo

    def __update_right_canvas (m):
        m.cnv_right.delete("all")

    def __adjust_line_segments (m, delta):
        m.line_segments += delta
        if m.line_segments > MtnRidgeGen.MAX_LINE_SEGMENTS:
            m.line_segments = MtnRidgeGen.MIN_LINE_SEGMENTS
        elif m.line_segments < MtnRidgeGen.MIN_LINE_SEGMENTS:
            m.line_segments = MtnRidgeGen.MAX_LINE_SEGMENTS
        m.line_segments_label.config(text=str(m.line_segments))

    def __get_curve_y_at_x (m, curve_points, target_x):
        closest = min(curve_points, key=lambda p: abs(p[0] - target_x))
        return closest[1]

    def __rise_to_peak (m, ridge_points, curve_points, peak_x, peak_y, canvas_h):
        adjusted = []
        for i, (x, y) in enumerate(ridge_points):
            if x < peak_x:
                if i == 0:
                    adjusted.append((x, canvas_h))
                else:
                    prev_y = adjusted[-1][1] if adjusted else canvas_h
                    curve_y = m.__get_curve_y_at_x(curve_points, x)
                    new_y = min(y, prev_y)
                    new_y = max(new_y, peak_y)
                    new_y = min(new_y, curve_y)
                    adjusted.append((x, new_y))
            elif x == peak_x:
                adjusted.append((peak_x, peak_y))
                break
        return adjusted

    def __fall_from_peak (m, ridge_points, curve_points, peak_x, peak_y, canvas_h):
        adjusted = []
        for i in range(len(ridge_points) - 1, -1, -1):
            x, y = ridge_points[i]
            if x > peak_x:
                if i == len(ridge_points) - 1:
                    adjusted.append((x, canvas_h))
                else:
                    prev_y = adjusted[-1][1] if adjusted else canvas_h
                    curve_y = m.__get_curve_y_at_x(curve_points, x)
                    new_y = min(y, prev_y)
                    new_y = max(new_y, peak_y)
                    new_y = min(new_y, curve_y)
                    adjusted.append((x, new_y))
            elif x == peak_x:
                adjusted.append((peak_x, peak_y))
                break
        adjusted.reverse()
        return adjusted

    def __segment_touches_curve (m, x1, y1, x2, y2, curve_points, tolerance=2):
        seg_min_x = min(x1, x2)
        seg_max_x = max(x1, x2)
        for cx, cy in curve_points:
            if seg_min_x <= cx <= seg_max_x:
                if x1 == x2:
                    ridge_y = y1
                else:
                    t = (cx - x1) / (x2 - x1)
                    ridge_y = y1 + t * (y2 - y1)
                if abs(ridge_y - cy) <= tolerance:
                    return True
        return False

    def __remove_elevated_points (m, ridge_points, curve_points):
        if len(ridge_points) <= 3:
            return ridge_points
        
        peak_idx = -1
        peak_y = float('inf')
        for i, (x, y) in enumerate(ridge_points):
            if y < peak_y:
                peak_y = y
                peak_idx = i
        
        result = [ridge_points[0]]
        
        for i in range(1, len(ridge_points) - 1):
            if i == peak_idx:
                result.append(ridge_points[i])
                continue
            
            prev_x, prev_y = result[-1]
            curr_x, curr_y = ridge_points[i]
            next_x, next_y = ridge_points[i + 1]
            
            prev_seg_touches = m.__segment_touches_curve(prev_x, prev_y, curr_x, curr_y, curve_points)
            next_seg_touches = m.__segment_touches_curve(curr_x, curr_y, next_x, next_y, curve_points)
            
            if not prev_seg_touches and not next_seg_touches:
                combined_touches = m.__segment_touches_curve(prev_x, prev_y, next_x, next_y, curve_points)
                if combined_touches:
                    continue
            
            result.append(ridge_points[i])
        
        result.append(ridge_points[-1])
        return result

    def __generate_main_ridge (m, curve_points, canvas_w, canvas_h):
        if len(curve_points) == 0:
            return
        
        start_x, start_y = 0, canvas_h
        end_x, end_y = canvas_w, canvas_h
        
        num_segments = m.line_segments
        ridge_points = [(start_x, start_y)]
        
        for seg in range(1, num_segments):
            seg_x = (seg / num_segments) * canvas_w
            curve_y = m.__get_curve_y_at_x(curve_points, seg_x)
            ridge_points.append((seg_x, curve_y))
        
        ridge_points.append((end_x, end_y))
        
        peak_idx = 0
        peak_y = ridge_points[0][1]
        for i, (x, y) in enumerate(ridge_points):
            if y < peak_y:
                peak_y = y
                peak_idx = i
        
        peak_x, peak_y = ridge_points[peak_idx]
        
        rising_points = m.__rise_to_peak(ridge_points, curve_points, peak_x, peak_y, canvas_h)
        falling_points = m.__fall_from_peak(ridge_points, curve_points, peak_x, peak_y, canvas_h)
        ridge_points = rising_points + falling_points[1:]
        ridge_points = m.__remove_elevated_points(ridge_points, curve_points)
        
        peak_point_idx = -1
        for i, (x, y) in enumerate(ridge_points):
            if x == peak_x:
                peak_point_idx = i
                break
        
        for i in range(len(ridge_points) - 1):
            x1, y1 = ridge_points[i]
            x2, y2 = ridge_points[i + 1]
            if i < peak_point_idx:
                m.cnv_right.create_line(x1, y1, x2, y2, fill="green", width=m.pencil_size)
            else:
                m.cnv_right.create_line(x1, y1, x2, y2, fill="red", width=m.pencil_size)
        
        for x, y in ridge_points:
            m.cnv_right.create_oval(x - 3, y - 3, x + 3, y + 3, fill="black", outline="black")

    def __generate_main (m):
        img_array = np.array(m.left_img)
        if len(img_array.shape) == 3:
            if img_array.shape[2] == 4:
                gray_row = img_array[m.mid_y, :, 0]
            else:
                gray_row = img_array[m.mid_y, :, 0]
        else:
            gray_row = img_array[m.mid_y, :]
        
        m.cnv_right.delete("all")
        canvas_w = m.cnv_right.winfo_width()
        canvas_h = m.cnv_right.winfo_height()
        if canvas_w <= 1 or canvas_h <= 1:
            canvas_w, canvas_h = m.left_img.size
        
        max_val = gray_row.max() if gray_row.max() > 0 else 255
        min_val = gray_row.min()
        val_range = max_val - min_val if max_val > min_val else 1
        
        curve_points = []
        prev_x, prev_y = None, None
        num_pixels = len(gray_row)
        for x in range(num_pixels):
            val = gray_row[x]
            normalized = (val - min_val) / val_range
            
            if x < 10:
                fade_factor = x / 9.0 if x > 0 else 0.0
                normalized *= fade_factor
            elif x >= num_pixels - 10:
                fade_factor = (num_pixels - 1 - x) / 9.0 if x < num_pixels - 1 else 0.0
                normalized *= fade_factor
            
            canvas_x = int((x / num_pixels) * canvas_w)
            canvas_y = int(canvas_h - (normalized * canvas_h))
            curve_points.append((canvas_x, canvas_y))
            
            if prev_x is not None:
                m.cnv_right.create_line(prev_x, prev_y, canvas_x, canvas_y, fill="white", width=m.pencil_size)
            prev_x, prev_y = canvas_x, canvas_y
        
        m.__generate_main_ridge(curve_points, canvas_w, canvas_h)

    def __init__ (m, image_path):
        m.root = tk.Tk()
        m.root.title("Mountain Ridge Generator")
        m.left_img = Image.open(image_path)
        img_w, img_h = m.left_img.size
        m.mid_y = img_h // 2
        m.pencil_size = 3
        m.line_segments = 30
        
        m.btn_frame = tk.Frame(m.root)
        m.btn_frame.pack(side=tk.TOP, fill=tk.X)
        
        m.btn_gen_main = tk.Button(m.btn_frame, text="Generate main", command=m.__generate_main)
        m.btn_gen_main.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.btn_line_seg_dec = tk.Button(m.btn_frame, text="<", command=lambda: m.__adjust_line_segments(-5))
        m.btn_line_seg_dec.pack(side=tk.LEFT, padx=2)
        m.btn_line_seg_inc = tk.Button(m.btn_frame, text=">", command=lambda: m.__adjust_line_segments(5))
        m.btn_line_seg_inc.pack(side=tk.LEFT, padx=2)
        m.line_segments_label = tk.Label(m.btn_frame, text=str(m.line_segments))
        m.line_segments_label.pack(side=tk.LEFT, padx=5)
        
        m.canvas_frame = tk.Frame(m.root)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        m.cnv_left = tk.Canvas(m.canvas_frame, width=img_w, height=img_h, bg="darkgray")
        m.cnv_left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        m.cnv_right = tk.Canvas(m.canvas_frame, width=img_w, height=img_h, bg="darkgray")
        m.cnv_right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        m.root.update_idletasks()
        m.__update_left_canvas()
        m.__update_right_canvas()

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
