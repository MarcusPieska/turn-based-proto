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
import random
import tkinter as tk
from PIL import Image, ImageTk, ImageFilter
import cv2
import ctypes
import os

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "dat15_io.so")
if not os.path.exists(lib_path):
    lib_path = os.path.join(os.path.dirname(__file__), "..", "tile_gen_proto", "dat15_io.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile dat15_io.cpp first." % (lib_path))

lib = ctypes.CDLL(lib_path)

lib.dat15_start_writer.restype = None
lib.dat15_start_writer.argtypes = [ctypes.c_char_p]

lib.dat15_write_item.restype = None
lib.dat15_write_item.argtypes = [ctypes.c_void_p, ctypes.c_int]

lib.dat15_finish.restype = None
lib.dat15_finish.argtypes = []

lib.dat15_start_reader.restype = None
lib.dat15_start_reader.argtypes = [ctypes.c_char_p]

lib.dat15_get_item_count.restype = ctypes.c_int
lib.dat15_get_item_count.argtypes = []

lib.dat15_get_item.restype = ctypes.c_void_p
lib.dat15_get_item.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]

def dat15_start_writer(path):
    lib.dat15_start_writer(path.encode('utf-8'))

def dat15_write_item(data, size):
    if isinstance(data, np.ndarray):
        data_ptr = data.ctypes.data_as(ctypes.c_void_p)
    else:
        arr = (ctypes.c_uint8 * size).from_buffer_copy(data)
        data_ptr = ctypes.cast(arr, ctypes.c_void_p)
    lib.dat15_write_item(data_ptr, size)

def dat15_finish():
    lib.dat15_finish()

def dat15_start_reader(path):
    lib.dat15_start_reader(path.encode('utf-8'))

def dat15_get_item_count():
    return lib.dat15_get_item_count()

def dat15_get_item(idx):
    size_out = ctypes.c_int()
    ptr = lib.dat15_get_item(idx, ctypes.byref(size_out))
    if ptr and size_out.value > 0:
        return ctypes.string_at(ptr, size_out.value)
    return None

#================================================================================================================================#
#=> - Class: TileCropper
#================================================================================================================================#

class TileCropper:

    def __toggleGrid (m):
        m.show_grid = not m.show_grid
        m.__displayTexture(first_time=False)

    def __drawGrid (m, cnv, width, height, tile_w, tile_h, zoom=1.0):
        if not m.show_grid:
            return
        half_w = int((tile_w // 2) * zoom)
        half_h = int((tile_h // 2) * zoom)

        half_width = tile_w // 2
        half_height = tile_h // 2
        tex_left = m.min_x
        tex_right = m.min_x + m.img_width
        tex_top = m.min_y
        tex_bottom = m.min_y + m.img_height
        
        m.tile_polys = []
        for ty in range(m.num_rows):
            row_offset = 0 if ty % 2 == 0 else half_width
            y_center = m.min_y + ty * half_height + half_height
            for tx in range(m.num_cols):
                x_center = m.min_x + tx * tile_w + row_offset + half_width
                top = (x_center, y_center - half_h)
                right = (x_center + half_w, y_center)
                bottom = (x_center, y_center + half_h)
                left = (x_center - half_w, y_center)
                m.tile_polys.append([top, right, bottom, left])
                cnv.create_line(top[0], top[1], right[0], right[1], fill="light gray", width=1, tags="grid")
                cnv.create_line(right[0], right[1], bottom[0], bottom[1], fill="light gray", width=1, tags="grid")
                cnv.create_line(bottom[0], bottom[1], left[0], left[1], fill="light gray", width=1, tags="grid")
                cnv.create_line(left[0], left[1], top[0], top[1], fill="light gray", width=1, tags="grid")

    def __displayTexture (m, first_time=False):
        m.canv_main.delete("all")
        
        # Because the tiles are staggered, we need to dupicate parts of the texture without breaking the texture tiling
        if first_time:
            half_w = m.tile_width // 2
            left_cols = m.tex_img[:, :half_w, :]
            tex_ext = np.concatenate([m.tex_img, left_cols], axis=1)
            half_rows = m.tile_height // 2
            top_rows = tex_ext[:half_rows, :, :]
            tex_ext = np.concatenate([tex_ext, top_rows], axis=0)
            m.tex_img = tex_ext

        photo = ImageTk.PhotoImage(Image.fromarray(m.tex_img, mode='RGB'))
        m.canv_main.create_image(m.img_x, m.img_y, anchor=tk.NW, image=photo, tags="texture")
        m.canv_main.image = photo

        m.__drawGrid(m.canv_main, m.canv_w, m.canv_h, m.tile_width, m.tile_height, 1.0)
        m.canv_main.tag_raise("grid")

        if m.tile_polys:
            m.crop_btn.config(state=tk.NORMAL)
        else:
            m.crop_btn.config(state=tk.DISABLED)

    def __cropAndTile (m):
        if not m.tile_polys:
            return
        output_img = np.zeros((m.canv_h, m.canv_w, 4), dtype=np.uint8)
        tex_h, tex_w = m.tex_img.shape[:2]

        dat15_start_writer(m.dat15_path_out)
        for poly in m.tile_polys:
            img_poly = []
            for point in poly:
                img_x = int(point[0] - m.img_x)
                img_y = int(point[1] - m.img_y)
                img_poly.append([img_x, img_y])
            mask = np.zeros((tex_h, tex_w), dtype=np.uint8)
            poly_array = np.array([img_poly], dtype=np.int32)
            cv2.fillPoly(mask, poly_array, 255)
            masked_rgb = m.tex_img.copy()
            for c in range(3):
                masked_rgb[:, :, c] = np.where(mask > 0, masked_rgb[:, :, c], 0)
            
            poly_canvas = np.array([[int(p[0]), int(p[1])] for p in poly])
            canvas_min_x = max(0, int(poly_canvas[:, 0].min()))
            canvas_max_x = min(m.canv_w, int(poly_canvas[:, 0].max()) + 1)
            canvas_min_y = max(0, int(poly_canvas[:, 1].min()))
            canvas_max_y = min(m.canv_h, int(poly_canvas[:, 1].max()) + 1)
            tex_min_x = max(0, canvas_min_x - m.img_x)
            tex_max_x = min(tex_w, canvas_max_x - m.img_x)
            tex_min_y = max(0, canvas_min_y - m.img_y)
            tex_max_y = min(tex_h, canvas_max_y - m.img_y)
            
            tex_region_rgb = masked_rgb[tex_min_y:tex_max_y, tex_min_x:tex_max_x]
            mask_region = mask[tex_min_y:tex_max_y, tex_min_x:tex_max_x]
            out_min_x = tex_min_x + m.img_x
            out_max_x = tex_max_x + m.img_x
            out_min_y = tex_min_y + m.img_y
            out_max_y = tex_max_y + m.img_y

            if tex_region_rgb.shape[0] > 0 and tex_region_rgb.shape[1] > 0:
                out_region = output_img[out_min_y:out_max_y, out_min_x:out_max_x]
                mask_3d = mask_region[:, :, np.newaxis] > 0
                out_region[:, :, :3] = np.where(mask_3d, tex_region_rgb, out_region[:, :, :3])
                out_region[:, :, 3] = np.where(mask_region > 0, 255, out_region[:, :, 3])
                output_img[out_min_y:out_max_y, out_min_x:out_max_x] = out_region
                
                tile_rgb = np.zeros((out_region.shape[0], out_region.shape[1], 3), dtype=np.uint8)
                tile_rgb[:, :, 0] = 255  # Magenta R
                tile_rgb[:, :, 1] = 0   # Magenta G
                tile_rgb[:, :, 2] = 255 # Magenta B
                mask_3d_rgb = mask_region[:, :, np.newaxis] > 0
                dat15_write_item(np.where(mask_3d_rgb, out_region[:, :, :3], tile_rgb), tile_rgb.nbytes)

        dat15_finish()
        output_pil = Image.fromarray(output_img, mode='RGBA')
        output_pil.save(m.tex_path_out)
        print("*** Saved cropped and tiled texture to: ", m.tex_path_out)

    def __init__ (m, root, tile_width, tile_height, tex_path):
        m.tex_path_out = tex_path.replace(".png", "_cropped_and_tiled.png")
        m.dat15_path_out = tex_path.replace(".png", ".dat15")
        m.root = root
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.show_grid = True
        img = Image.open(tex_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        m.tex_img = np.array(img)
        m.img_width, m.img_height = img.size

        m.tile_polys = []
        m.canv_w = m.img_width + 3 * tile_width
        m.canv_h = m.img_height + 3 * tile_height
        m.img_x = tile_width
        m.img_y = tile_height
        m.num_cols = m.img_width // tile_width
        m.num_rows = m.img_height // (tile_height // 2)
        m.min_x = m.img_x
        m.min_y = m.img_y
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.grid_btn = tk.Button(m.top_frame, text="Toggle Grid", command=m.__toggleGrid, width=15)
        m.grid_btn.pack(side=tk.LEFT, padx=5)
        m.crop_btn = tk.Button(m.top_frame, text="Crop and tile", command=m.__cropAndTile, width=15, state=tk.DISABLED)
        m.crop_btn.pack(side=tk.LEFT, padx=5)
        m.canv_frame = tk.Frame(root)
        m.canv_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_main = tk.Canvas(m.canv_frame, width=m.canv_w, height=m.canv_h, bg="gray")
        m.canv_main.pack(side=tk.LEFT, padx=5)
        
        m.__displayTexture(first_time=True)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    tex_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.png"
    root = tk.Tk()
    root.title("Tile Cropper")
    tile_width = 100
    tile_height = 50
    editor = TileCropper (root, tile_width, tile_height, tex_path)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
