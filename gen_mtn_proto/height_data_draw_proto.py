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
import glob

#================================================================================================================================#
#=> - Class: HeightDataDraw
#================================================================================================================================#

class HeightDataDraw:

    def __prev_file (m):
        m.flip_idx = (m.flip_idx - 1) % len(m.flip_options)
        if m.flip_idx == len(m.flip_options) - 1:
            m.current_idx = (m.current_idx - 1) % len(m.file_list)
        m.load_file(m.current_idx)
        m.update_display()

    def __next_file (m):
        m.flip_idx = (m.flip_idx + 1) % len(m.flip_options)
        if m.flip_idx == 0:
            m.current_idx = (m.current_idx + 1) % len(m.file_list)
        m.load_file(m.current_idx)
        m.update_display()
    
    def __do_height_projection (m, img_array, alpha_array):
        h, w = img_array.shape
        result = np.zeros((h, w), dtype=np.uint8)
        result_alpha = np.zeros((h, w), dtype=np.uint8)
        for x in range(w):
            for y in range(h):
                if alpha_array[y, x] > 0:
                    result[y, x] = 0
                    result_alpha[y, x] = 255
        for x in range(w):
            tile_start_y = None
            for y in range(h - 1, -1, -1):
                if alpha_array[y, x] > 0:
                    tile_start_y = y
                    break
            if tile_start_y is None:
                continue
            top_y = h
            prev_draw_y = None
            prev_color = None
            for y in range(tile_start_y, -1, -1):
                if alpha_array[y, x] == 0:
                    continue
                height_val = img_array[y, x]
                if height_val == 0:
                    draw_y = y
                elif height_val == 255:
                    draw_y = y - m.projection_height
                else:
                    projection = int((height_val / 255.0) * m.projection_height)
                    draw_y = y - projection
                draw_y = max(0, draw_y)
                if draw_y < top_y:
                    if prev_draw_y is not None and prev_draw_y > draw_y:
                        for fill_y in range(draw_y, prev_draw_y + 1):
                            if fill_y < top_y:
                                t = (fill_y - draw_y) / (prev_draw_y - draw_y) if prev_draw_y > draw_y else 0.0
                                result[fill_y, x] = int(height_val * (1.0 - t) + prev_color * t)
                                result_alpha[fill_y, x] = 255
                    result[draw_y, x] = height_val
                    result_alpha[draw_y, x] = 255
                    top_y = draw_y
                    prev_draw_y = draw_y
                    prev_color = height_val
        return result, result_alpha

    def __init__ (m, zoom=4, tile_width=101, tile_height=50, save_path=None):
        m.projection_height = 250
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.save_path = save_path
        m.zoom = zoom
        m.projected_height = m.tile_height
        m.canvas_width = m.tile_width * m.zoom
        m.canvas_height = (m.tile_height + m.projection_height) * m.zoom
        m.file_list = []
        m.current_idx = 0
        m.current_img = None
        m.current_alpha = None
        if m.save_path:
            pattern = os.path.join(m.save_path, "raw_mtn_hd*.png")
            m.file_list = sorted(glob.glob(pattern))
        if len(m.file_list) == 0:
            exit("*** No files found")
        m.flip_idx = 0
        m.flip_options = [(False, False), (True, False), (False, True), (True, True)]
        m.root = tk.Tk()
        m.root.title("Height Data Draw")
        m.btn_frame = tk.Frame(m.root)
        m.btn_frame.pack(side=tk.TOP, fill=tk.X)
        m.prev_btn = tk.Button(m.btn_frame, text="<", command=m.__prev_file)
        m.prev_btn.pack(side=tk.LEFT, padx=5, pady=5)
        m.next_btn = tk.Button(m.btn_frame, text=">", command=m.__next_file)
        m.next_btn.pack(side=tk.LEFT, padx=5, pady=5)
        m.canvas = tk.Canvas(m.root, width=m.canvas_width, height=m.canvas_height, bg="darkgray")
        m.canvas.pack(fill=tk.BOTH, expand=True)
        m.photo = None
        if len(m.file_list) > 0:
            m.load_file(0)
            m.update_display()

    def load_file (m, idx):
        img = Image.open(m.file_list[idx])
        img_array = np.array(img)
        if m.flip_options[m.flip_idx][0]:
            img_array = img_array[:, ::-1]
        if m.flip_options[m.flip_idx][1]:
            img_array = img_array[::-1, :]
        if img_array.shape[2] == 4:
            img_data = img_array[:, :, 0]
            alpha_data = img_array[:, :, 3]
        else:
            img_data = img_array
            alpha_data = np.ones_like(img_array, dtype=np.uint8) * 255
        h, w = img_data.shape
        padded_h = h + m.projection_height
        m.current_img = np.zeros((padded_h, w), dtype=np.uint8)
        m.current_alpha = np.zeros((padded_h, w), dtype=np.uint8)
        m.current_img[m.projection_height:, :] = img_data
        m.current_alpha[m.projection_height:, :] = alpha_data

    def update_display (m):
        if m.current_img is None:
            return
        h, w = m.current_img.shape
        projected, projected_alpha = m.__do_height_projection(m.current_img, m.current_alpha)
        proj_h, proj_w = projected.shape
        new_w = proj_w * m.zoom
        new_h = proj_h * m.zoom
        img_resized = Image.fromarray(projected, mode='L').resize((new_w, new_h), Image.NEAREST)
        alpha_resized = Image.fromarray(projected_alpha, mode='L').resize((new_w, new_h), Image.NEAREST)
        rgba_array = np.zeros((new_h, new_w, 4), dtype=np.uint8)
        img_array = np.array(img_resized)
        rgba_array[:, :, 0] = img_array
        rgba_array[:, :, 1] = img_array
        rgba_array[:, :, 2] = img_array
        rgba_array[:, :, 3] = np.array(alpha_resized)
        img = Image.fromarray(rgba_array, mode='RGBA')
        m.photo = ImageTk.PhotoImage(img)
        m.canvas.delete("all")
        m.canvas.create_image(m.canvas_width // 2, m.canvas_height // 2, image=m.photo, anchor=tk.CENTER)

    def run (m):
        m.root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    save_path = "../../img-content"
    draw = HeightDataDraw(zoom=1, tile_width=101, tile_height=50, save_path=save_path) # leave the 4 alone
    draw.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
