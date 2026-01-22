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
import random
import time
import tkinter as tk
from tkinter import messagebox
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

dat15_lib_path = os.path.join(os.path.dirname(__file__), "dat15_io.so")
if not os.path.exists(dat15_lib_path):
    dat15_lib_path = os.path.join(os.path.dirname(__file__), "..", "tile_gen_proto", "dat15_io.so")
if not os.path.exists(dat15_lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile dat15_io.cpp first." % (dat15_lib_path))

dat15_lib = ctypes.CDLL(dat15_lib_path)

dat15_lib.dat15_start_reader.restype = None
dat15_lib.dat15_start_reader.argtypes = [ctypes.c_char_p]

dat15_lib.dat15_get_item_count.restype = ctypes.c_int
dat15_lib.dat15_get_item_count.argtypes = []

dat15_lib.dat15_get_item.restype = ctypes.c_void_p
dat15_lib.dat15_get_item.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]

def dat15_start_reader(path):
    dat15_lib.dat15_start_reader(path.encode('utf-8'))

def dat15_get_item_count():
    return dat15_lib.dat15_get_item_count()

def dat15_get_item(idx):
    size_out = ctypes.c_int()
    ptr = dat15_lib.dat15_get_item(idx, ctypes.byref(size_out))
    if ptr and size_out.value > 0:
        return ctypes.string_at(ptr, size_out.value)
    return None

morph_lib_path = os.path.join(os.path.dirname(__file__), "col_morph.so")
if not os.path.exists(morph_lib_path):
    morph_lib_path = os.path.join(os.path.dirname(__file__), "..", "tile_gen_proto", "col_morph.so")
if not os.path.exists(morph_lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile col_morph.cpp first." % (morph_lib_path))

morph_lib = ctypes.CDLL(morph_lib_path)

class pt(ctypes.Structure):
    _fields_ = [("x", ctypes.c_int), ("y", ctypes.c_int)]

class deltas(ctypes.Structure):
    _fields_ = [("top_dy", ctypes.c_int), ("right_dy", ctypes.c_int), ("bottom_dy", ctypes.c_int), ("left_dy", ctypes.c_int)]

morph_lib.morph_tile.restype = None
morph_lib.morph_tile.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_int, ctypes.c_int, ctypes.c_int, pt, pt, pt, pt, deltas, ctypes.POINTER(ctypes.c_uint8)]

def morph_tile(src_data, src_w, src_h, channels, top_pt, right_pt, bottom_pt, left_pt, d, dst_data):
    if isinstance(src_data, np.ndarray):
        src_ptr = src_data.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
    else:
        src_arr = (ctypes.c_uint8 * (src_w * src_h * channels)).from_buffer_copy(src_data)
        src_ptr = ctypes.cast(src_arr, ctypes.POINTER(ctypes.c_uint8))
    
    if isinstance(dst_data, np.ndarray):
        dst_ptr = dst_data.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
    else:
        dst_arr = (ctypes.c_uint8 * (src_w * src_h * channels)).from_buffer_copy(dst_data)
        dst_ptr = ctypes.cast(dst_arr, ctypes.POINTER(ctypes.c_uint8))
    
    morph_lib.morph_tile(src_ptr, src_w, src_h, channels, top_pt, right_pt, bottom_pt, left_pt, d, dst_ptr)
    
    if isinstance(dst_data, np.ndarray):
        return dst_data
    else:
        return bytes(dst_arr)

#================================================================================================================================#
#=> - Class: TileMorpher
#================================================================================================================================#

class TileMorpher:

    def __init__ (m, root, dat15_path, tile_idx=0):
        m.root = root
        m.dat15_path = dat15_path
        
        dat15_start_reader(dat15_path)
        item_data = dat15_get_item(tile_idx)
        item_arr = np.frombuffer(item_data, dtype=np.uint8)
        m.tile_w = 101
        m.tile_h = 51
        m.morph_h = m.tile_h * 2
        m.head_space_h = m.tile_h
        m.full_tile_h = m.morph_h
        m.tile_img_base = item_arr.reshape((m.tile_h, m.tile_w, 3))
        m.tile_img = np.zeros((m.full_tile_h, m.tile_w, 3), dtype=np.uint8)
        m.tile_img[:, :, 0] = 255
        m.tile_img[:, :, 2] = 255
        m.tile_img[m.head_space_h:, :, :] = m.tile_img_base
        m.src_morph_img = np.zeros((m.morph_h, m.tile_w, 3), dtype=np.uint8)
        m.src_morph_img[:, :, 0] = 255
        m.src_morph_img[:, :, 2] = 255
        m.src_morph_img[m.tile_h:, :, :] = m.tile_img_base
        
        m.scale = 3.0
        scaled_h = int(m.full_tile_h * m.scale)
        m.canv_w = 600
        m.canv_h = scaled_h
        m.center_x = m.canv_w // 2
        m.center_y = m.canv_h // 2
        
        half_w = m.tile_w // 2
        half_h = m.tile_h // 2
        morph_center_y = m.tile_h + half_h
        
        m.top_pt = pt(m.tile_w // 2, morph_center_y - half_h)
        m.right_pt = pt(m.tile_w // 2 + half_w, morph_center_y)
        m.bottom_pt = pt(m.tile_w // 2, morph_center_y + half_h)
        m.left_pt = pt(m.tile_w // 2 - half_w, morph_center_y)
        
        m.top_dy = 0
        m.right_dy = 0
        m.bottom_dy = 0
        m.left_dy = 0
        
        m.dragging = None
        m.drag_offset_y = 0
        
        m.button_frame = tk.Frame(root)
        m.button_frame.pack(padx=5, pady=5)
        
        m.random_btn = tk.Button(m.button_frame, text="Random", command=m.__onRandom)
        m.random_btn.pack(side=tk.LEFT, padx=5)
        
        m.time_btn = tk.Button(m.button_frame, text="Time", command=m.__time)
        m.time_btn.pack(side=tk.LEFT, padx=5)
        
        m.canv = tk.Canvas(root, width=m.canv_w, height=m.canv_h, bg="gray")
        m.canv.pack(padx=5, pady=5)
        
        m.canv.bind("<Button-1>", m.__onClick)
        m.canv.bind("<B1-Motion>", m.__onDrag)
        m.canv.bind("<ButtonRelease-1>", m.__onRelease)
        
        m.__displayTile()

    def __getCornerCanvasPos (m, corner_name):
        half_w = m.tile_w // 2
        half_h = m.tile_h // 2
        morph_center_y = m.tile_h + half_h
        base_y = m.center_y + (morph_center_y - m.morph_h // 2) * m.scale
        
        if corner_name == "top":
            return (m.center_x, base_y - half_h * m.scale + m.top_dy)
        elif corner_name == "right":
            return (m.center_x + half_w * m.scale, base_y + m.right_dy)
        elif corner_name == "bottom":
            return (m.center_x, base_y + half_h * m.scale + m.bottom_dy)
        elif corner_name == "left":
            return (m.center_x - half_w * m.scale, base_y + m.left_dy)

    def __onClick (m, event):
        click_x, click_y = event.x, event.y
        drag_threshold = 10
        
        for corner_name in ["top", "right", "left"]:
            corner_pos = m.__getCornerCanvasPos(corner_name)
            dist = ((click_x - corner_pos[0]) ** 2 + (click_y - corner_pos[1]) ** 2) ** 0.5
            if dist < drag_threshold:
                m.dragging = corner_name
                m.drag_offset_y = click_y - corner_pos[1]
                break

    def __onDrag (m, event):
        if m.dragging is None:
            return
        
        corner_pos = m.__getCornerCanvasPos(m.dragging)
        new_y = event.y - m.drag_offset_y
        new_y = max(0, min(m.canv_h - 1, new_y))
        
        half_h = m.tile_h // 2
        morph_center_y = m.tile_h + half_h
        base_y = m.center_y + (morph_center_y - m.morph_h // 2) * m.scale
        
        if m.dragging == "top":
            m.top_dy = new_y - (base_y - half_h * m.scale)
        elif m.dragging == "right":
            m.right_dy = new_y - base_y
        elif m.dragging == "left":
            m.left_dy = new_y - base_y
        
        m.__morphAndDisplay(display=True)

    def __onRelease (m, event):
        m.dragging = None

    def __onRandom (m, display=True):
        m.top_dy = random.randint(-40, 40)
        m.right_dy = random.randint(-20, 20)
        m.left_dy = random.randint(-20, 20)
        m.bottom_dy = 0
        m.__morphAndDisplay(display=display)

    def __time (m):
        num_iterations = 1000
        start_time = time.perf_counter()
        for _ in range(num_iterations):
            m.__onRandom(display=False)
        stop_time = time.perf_counter()
        elapsed_time = stop_time - start_time
        time_per_tile = elapsed_time / num_iterations
        print ("*** Total time taken for %d morhping operations: %.4f seconds" % (num_iterations, elapsed_time))

    def __morphAndDisplay (m, display=True):
        d = deltas(int(m.top_dy / m.scale), int(m.right_dy / m.scale), int(m.bottom_dy / m.scale), int(m.left_dy / m.scale))
        morphed_img = np.zeros((m.morph_h, m.tile_w, 3), dtype=np.uint8)
        morph_tile(m.src_morph_img, m.tile_w, m.morph_h, 3, m.top_pt, m.right_pt, m.bottom_pt, m.left_pt, d, morphed_img)
        m.tile_img[:, :, :] = morphed_img

        if display:
            m.__displayTile()

    def __displayTile (m):
        m.canv.delete("all")
        
        scaled_w = int(m.tile_w * m.scale)
        scaled_h = int(m.full_tile_h * m.scale)
        
        img_pil = Image.fromarray(m.tile_img, mode='RGB')
        img_pil = img_pil.resize((scaled_w, scaled_h), Image.NEAREST)
        photo = ImageTk.PhotoImage(img_pil)
        
        m.canv.create_image(m.center_x, m.center_y, anchor=tk.CENTER, image=photo, tags="tile")
        m.canv.image = photo
        
        top_pos = m.__getCornerCanvasPos("top")
        right_pos = m.__getCornerCanvasPos("right")
        bottom_pos = m.__getCornerCanvasPos("bottom")
        left_pos = m.__getCornerCanvasPos("left")
        
        m.canv.create_line(top_pos[0], top_pos[1], right_pos[0], right_pos[1], fill="light gray", width=2, tags="outline")
        m.canv.create_line(right_pos[0], right_pos[1], bottom_pos[0], bottom_pos[1], fill="light gray", width=2, tags="outline")
        m.canv.create_line(bottom_pos[0], bottom_pos[1], left_pos[0], left_pos[1], fill="light gray", width=2, tags="outline")
        m.canv.create_line(left_pos[0], left_pos[1], top_pos[0], top_pos[1], fill="light gray", width=2, tags="outline")
        
        for corner_name in ["top", "right", "left"]:
            corner_pos = m.__getCornerCanvasPos(corner_name)
            m.canv.create_oval(corner_pos[0] - 5, corner_pos[1] - 5, corner_pos[0] + 5, corner_pos[1] + 5, fill="yellow", outline="black", width=2, tags="corner")
        
        m.canv.tag_raise("outline")
        m.canv.tag_raise("corner")

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    dat15_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.dat15"
    root = tk.Tk()
    root.title("Tile Morpher")
    morpher = TileMorpher(root, dat15_path, tile_idx=0)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
