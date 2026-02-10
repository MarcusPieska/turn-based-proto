#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import ctypes
from PIL import Image
import cv2

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "dat15_io.so")
if not os.path.exists(lib_path):
    lib_path = os.path.join(os.path.dirname(__file__), "..", "tile_gen_proto", "dat15_io.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile dat15_io.cpp first." % (lib_path))

lib = ctypes.CDLL(lib_path)

lib.dat15_start_reader.restype = None
lib.dat15_start_reader.argtypes = [ctypes.c_char_p]

lib.dat15_get_item.restype = ctypes.c_void_p
lib.dat15_get_item.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_int)]

def dat15_start_reader(path):
    lib.dat15_start_reader(path.encode('utf-8'))

def dat15_get_item(row, col):
    size_out = ctypes.c_int()
    ptr = lib.dat15_get_item(row, col, ctypes.byref(size_out))
    if ptr and size_out.value > 0:
        return ctypes.string_at(ptr, size_out.value)
    return None

#================================================================================================================================#
#=> - Class: Dat15Tester
#================================================================================================================================#

class Dat15Tester:

    def __cropAndTile (m):
        if not m.dat15_path:
            print("*** Error: No dat15 file path specified")
            return
        
        dat15_start_reader(m.dat15_path)
        img = Image.open(m.tex_path)
        if img.mode != 'RGB':
            img = img.convert('RGB')
        img_width, img_height = img.size
        
        num_cols = img_width // m.tile_width
        num_rows = img_height // (m.tile_height // 2)
        
        half_width = m.tile_width // 2
        half_height = m.tile_height // 2
        
        canv_w = img_width + 3 * m.tile_width
        canv_h = img_height + 3 * m.tile_height
        img_x = m.tile_width
        img_y = m.tile_height
        min_x = img_x
        min_y = img_y
        
        output_img1 = np.zeros((canv_h, canv_w, 3), dtype=np.uint8)
        output_img1[:, :] = [255, 0, 255]
        output_img2 = np.zeros((canv_h*2, canv_w, 3), dtype=np.uint8)
        output_img2[:, :] = [255, 0, 255]
        
        for ty in range(num_rows):
            row_offset = 0 if ty % 2 == 0 else half_width
            y_center = min_y + ty * half_height + half_height
            for tx in range(num_cols):
                x_center = min_x + tx * m.tile_width + row_offset + half_width
                top = (x_center, y_center - half_height)
                right = (x_center + half_width, y_center)
                bottom = (x_center, y_center + half_height)
                left = (x_center - half_width, y_center)
                
                item_data = dat15_get_item(tx, ty)
                if item_data:
                    item_arr = np.frombuffer(item_data, dtype=np.uint8)
                    expected_size = len(item_data)
                    tile_rgb = item_arr.reshape((m.tile_height+1, m.tile_width+1, 3))
                    poly_canvas = np.array([[int(p[0]), int(p[1])] for p in [top, right, bottom, left]])
                    canvas_min_x = max(0, int(poly_canvas[:, 0].min()))
                    canvas_max_x = min(canv_w, int(poly_canvas[:, 0].max()))
                    canvas_min_y = max(0, int(poly_canvas[:, 1].min()))
                    canvas_max_y = min(canv_h, int(poly_canvas[:, 1].max()))
                    
                    out_min_x = canvas_min_x
                    out_max_x = canvas_max_x
                    out_min_y = canvas_min_y
                    out_max_y = canvas_max_y
                    
                    tile_min_x = max(0, canvas_min_x - (x_center - half_width))
                    tile_min_y = max(0, canvas_min_y - (y_center - half_height))
                    tile_max_x = min(m.tile_width, canvas_max_x - (x_center - half_width))
                    tile_max_y = min(m.tile_height, canvas_max_y - (y_center - half_height))
                    
                    out_region = output_img1[out_min_y:out_max_y, out_min_x:out_max_x]
                    tile_region = tile_rgb[tile_min_y:tile_max_y, tile_min_x:tile_max_x]
                    
                    mask = np.zeros((tile_max_y - tile_min_y, tile_max_x - tile_min_x), dtype=np.uint8)
                    img_poly = []
                    for point in [top, right, bottom, left]:
                        img_x = int(point[0] - out_min_x)
                        img_y = int(point[1] - out_min_y)
                        img_poly.append([img_x, img_y])
                    poly_array = np.array([img_poly], dtype=np.int32)
                    cv2.fillPoly(mask, poly_array, 255)
                    
                    mask_3d = mask[:, :, np.newaxis] > 0
                    out_region[:, :, :] = np.where(mask_3d, tile_region, out_region)
                    output_img1[out_min_y:out_max_y, out_min_x:out_max_x] = out_region
                    
                    x2 = tx * m.tile_width
                    y2 = ty * m.tile_height
                    output_img2[y2:y2 + m.tile_height+1, x2:x2 + m.tile_width+1] = tile_rgb
        
        output_pil = Image.fromarray(output_img1, mode='RGB')
        output_pil.save(m.dat15_path.replace(".dat15", "_reconstructed1.png"))
        output_pil.show()
        output_pil = Image.fromarray(output_img2, mode='RGB')
        output_pil.save(m.dat15_path.replace(".dat15", "_reconstructed2.png"))
        output_pil.show()

    def __init__ (m, dat15_path, tex_path, tile_width, tile_height):
        m.dat15_path = dat15_path
        m.tex_path = tex_path
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.__cropAndTile()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    dat15_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.dat15"
    tex_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.png"
    tile_width = 100
    tile_height = 50
    tester = Dat15Tester(dat15_path, tex_path, tile_width, tile_height)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
