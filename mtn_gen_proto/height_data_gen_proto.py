#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import math
import tkinter as tk
from PIL import Image, ImageTk
import noise

#================================================================================================================================#
#=> - Class: HeightDataGenerator
#================================================================================================================================#

class HeightDataGenerator:

    DIST_TYPE_NONE = 'none'
    DIST_TYPE_LINEAR = 'linear'
    DIST_TYPE_OVAL_LIM = 'oval_lim'

    def __init__ (m, tile_width=100, tile_height=100, octaves=6, persistence=0.5, lacunarity=2.0, scale=50.0, seed=None):
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.octaves = octaves
        m.persistence = persistence
        m.lacunarity = lacunarity
        m.scale = scale
        m.seed = seed if seed is not None else np.random.randint(0, 1000000)
        m.dist_type_idx = 0
        m.dist_type_map = {}
        m.__generate_basic_tile()

    def __generate_basic_tile (m):
        m.base_height_data = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        m.alpha = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        m.boundary_dist_none = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        m.boundary_dist_linear = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        m.boundary_dist_oval_lim = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        cx, cy = m.tile_width / 2.0, m.tile_height / 2.0
        hw, hh = m.tile_width / 2.0, m.tile_height / 2.0
        for y in range(m.tile_height):
            for x in range(m.tile_width):
                dx = abs(x - cx) / hw
                dy = abs(y - cy) / hh
                dist_to_boundary = 1.0 - (dx + dy)
                m.boundary_dist_none[y, x] = 255
                if dist_to_boundary > 0:
                    m.alpha[y, x] = 255
                    m.boundary_dist_linear[y, x] = int(dist_to_boundary * 255.0)
                dx_oval = (x - cx) / hw
                dy_oval = (y - cy) / hh
                dist_oval = math.sqrt(dx_oval * dx_oval + dy_oval * dy_oval)
                dist_to_boundary_oval = 1.0 - dist_oval
                if dist_to_boundary_oval > 0:
                    m.boundary_dist_oval_lim[y, x] = int(dist_to_boundary_oval * 255.0)
        m.dist_type_map[m.DIST_TYPE_NONE] = m.boundary_dist_none
        m.dist_type_map[m.DIST_TYPE_LINEAR] = m.boundary_dist_linear
        m.dist_type_map[m.DIST_TYPE_OVAL_LIM] = m.boundary_dist_oval_lim

    def cycle_dist_types (m):
        m.dist_type_idx = (m.dist_type_idx + 1) % len(m.dist_type_map)

    def get_boundary_dist (m):
        keys = list(m.dist_type_map.keys())
        return m.dist_type_map[keys[m.dist_type_idx]]

    def generate (m, seed=None):
        m.seed = seed if seed is not None else np.random.randint(0, 1000000)
        height_data = np.zeros((m.tile_height, m.tile_width), dtype=np.float64)
        for y in range(m.tile_height):
            for x in range(m.tile_width):
                if m.alpha[y, x] > 0:
                    value, amplitude = 0.0, 1.0
                    freq = 1.0 / m.scale
                    for i in range(m.octaves):
                        nx, ny = x * freq, y * freq
                        nval = noise.pnoise2(nx, ny, repeatx=1024, repeaty=1024, base=m.seed + i)
                        value += nval * amplitude
                        amplitude *= m.persistence
                        freq *= m.lacunarity
                    height_data[y, x] = value
        min_val = height_data[m.alpha > 0].min() if height_data[m.alpha > 0].size > 0 else 0.0
        max_val = height_data[m.alpha > 0].max() if height_data[m.alpha > 0].size > 0 else 1.0
        if max_val > min_val:
            normalized = (height_data[m.alpha > 0] - min_val) / (max_val - min_val)
            height_data_u8 = m.base_height_data.copy()
            height_data_u8[m.alpha > 0] = (normalized * 255.0).astype(np.uint8)
        else:
            height_data_u8 = m.base_height_data.copy()
        return height_data_u8, m.alpha

#================================================================================================================================#
#=> - Class: HeightDataDisplay
#================================================================================================================================#

class HeightDataDisplay:

    def __get_available_save_path (m):
        filename_prefix = "hd-raw"
        suffix = 0
        while True:
            full_path = "%s/raw_mtn_hd%03d.png" % (m.save_path, suffix)
            if not os.path.exists(full_path):
                return full_path
            suffix += 1

    def __save_cb (m):
        if m.save_path is None or m.height_data is None:
            return
        full_path = m.__get_available_save_path()
        combined_u16 = m.boundary_dist.astype(np.uint16) * m.height_data.astype(np.uint16)
        combined = combined_u16.astype(np.float64) / 255.0
        min_val = combined.min()
        max_val = combined.max()
        if max_val > min_val:
            normalized = (combined - min_val) / (max_val - min_val)
            img_array = (normalized * 255.0).astype(np.uint8)
        else:
            img_array = np.zeros_like(combined, dtype=np.uint8)
        alpha_array = m.alpha_data
        rgba_array = np.zeros((img_array.shape[0], img_array.shape[1], 4), dtype=np.uint8)
        rgba_array[:, :, 0] = img_array
        rgba_array[:, :, 1] = img_array
        rgba_array[:, :, 2] = img_array
        rgba_array[:, :, 3] = alpha_array
        img = Image.fromarray(rgba_array, mode='RGBA')
        img.save(full_path)

    def __init__ (m, width=800, height=600, tile_width=100, tile_height=50, save_path=None):
        m.width = width
        m.height = height
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.save_path = save_path
        m.root = tk.Tk()
        m.root.title("Height Data Display")
        m.gen = HeightDataGenerator(m.tile_width, m.tile_height)
        m.height_data = None
        m.alpha_data = m.gen.alpha
        m.boundary_dist = m.gen.get_boundary_dist()
        
        m.btn_frame = tk.Frame(m.root)
        m.btn_frame.pack(side=tk.TOP, fill=tk.X)
        m.gen_btn = tk.Button(m.btn_frame, text="Generate", command=m.generate)
        m.gen_btn.pack(side=tk.LEFT, padx=5, pady=5)
        m.cycle_btn = tk.Button(m.btn_frame, text="Cycle", command=m.cycle_dist)
        m.cycle_btn.pack(side=tk.LEFT, padx=5, pady=5)
        m.save_btn = tk.Button(m.btn_frame, text="Save", command=m.__save_cb)
        m.save_btn.pack(side=tk.LEFT, padx=5, pady=5)

        m.canvas = tk.Canvas(m.root, width=m.width, height=m.height, bg="darkgray")
        m.canvas.pack(fill=tk.BOTH, expand=True)
        m.photo = None
        m.update_display()

    def generate (m):
        m.height_data, m.alpha_data = m.gen.generate()
        m.boundary_dist = m.gen.get_boundary_dist()
        m.update_display()

    def cycle_dist (m):
        m.gen.cycle_dist_types()
        m.boundary_dist = m.gen.get_boundary_dist()
        m.update_display()

    def update_display (m):
        if m.alpha_data is None:
            return
        if m.height_data is None:
            display_data = m.boundary_dist.astype(np.float64)
        else:
            combined_u16 = m.boundary_dist.astype(np.uint16) * m.height_data.astype(np.uint16)
            display_data = combined_u16.astype(np.float64) / 255.0
        h, w = display_data.shape
        min_val = display_data.min()
        max_val = display_data.max()
        if max_val > min_val:
            normalized = (display_data - min_val) / (max_val - min_val)
            img_array = (normalized * 255.0).astype(np.uint8)
        else:
            img_array = np.zeros_like(display_data, dtype=np.uint8)
        alpha_array = m.alpha_data
        rgba_array = np.zeros((h, w, 4), dtype=np.uint8)
        rgba_array[:, :, 0] = img_array
        rgba_array[:, :, 1] = img_array
        rgba_array[:, :, 2] = img_array
        rgba_array[:, :, 3] = alpha_array
        img = Image.fromarray(rgba_array, mode='RGBA')
        margin = 0
        scale_x = (m.width - 2 * margin) / w
        scale_y = (m.height - 2 * margin) / h
        scale = min(scale_x, scale_y)
        new_w = int(w * scale)
        new_h = int(h * scale)
        img = img.resize((new_w, new_h), Image.NEAREST)
        m.photo = ImageTk.PhotoImage(img)
        m.canvas.delete("all")
        m.canvas.create_image(m.width // 2, m.height // 2, image=m.photo, anchor=tk.CENTER)

    def run (m):
        m.root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    save_path = "../../img-content"
    display = HeightDataDisplay(width=800, height=600, tile_width=101, tile_height=100, save_path=save_path)
    display.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
