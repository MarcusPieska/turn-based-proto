#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
import numpy as np
from PIL import Image, ImageTk
import ctypes

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "map_overlays.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile map_overlays.cpp first." %(lib_path))

lib = ctypes.CDLL(lib_path)

class pt(ctypes.Structure): _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]
class size(ctypes.Structure): _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.distance_overlay_block.restype = None
lib.distance_overlay_block.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_int)
]

def distance_overlay_block_cpp(img, bg_col, block_col):
    img = np.ascontiguousarray(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(bg_col, (list, tuple)):
        bg_col = np.array(bg_col, dtype=np.uint8)
    if isinstance(block_col, (list, tuple)):
        block_col = np.array(block_col, dtype=np.uint8)
    if bg_col.size != ch:
        bg_col = np.array([bg_col[0]], dtype=np.uint8) if ch == 1 else np.array(bg_col[:ch], dtype=np.uint8)
    if block_col.size != ch:
        block_col = np.array([block_col[0]], dtype=np.uint8) if ch == 1 else np.array(block_col[:ch], dtype=np.uint8)
    dist_out = np.full((h, w), -1, dtype=np.int32)
    sz = size(w, h)
    img_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    bg_ptr = bg_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    block_ptr = block_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    dist_ptr = dist_out.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    lib.distance_overlay_block(img_ptr, sz, ch, bg_ptr, block_ptr, dist_ptr)
    return dist_out

#================================================================================================================================#
#=> - Class: DialogGetHeight -
#================================================================================================================================#

class DialogGetHeight:

    MIN_VAL = 0
    MAX_VAL = 100

    def __generate_overlay_image (m, dist_overlay):
        if dist_overlay is None:
            return None
        valid_mask = dist_overlay >= 0
        if not np.any(valid_mask):
            return None
        normalized = np.zeros_like(dist_overlay, dtype=np.uint8)
        normalized[valid_mask] = (dist_overlay[valid_mask] * 2.55).astype(np.uint8)
        overlay_img = np.stack([normalized, normalized, normalized], axis=2)
        if m.image.ndim == 3:
            bg_mask = np.all(m.image == m.color_bg, axis=2)
            overlay_img[bg_mask] = m.color_bg
        else:
            bg_mask = (m.image == m.color_bg)
            overlay_img[bg_mask] = m.color_bg
        return Image.fromarray(overlay_img, 'RGB')

    def __display_on_canvas (m, img):
        if img is None:
            return
        img_w, img_h = img.size
        canvas_w, canvas_h = 800, 600
        zoom_x, zoom_y = canvas_w // img_w, canvas_h // img_h
        zoom = min(zoom_x, zoom_y)
        if zoom < 1:
            zoom = 1
        new_w, new_h = img_w * zoom, img_h * zoom
        img_zoomed = img.resize((new_w, new_h), Image.Resampling.NEAREST)
        photo = ImageTk.PhotoImage(img_zoomed)
        m.canvas.delete("all")
        m.canvas.create_image(canvas_w // 2, canvas_h // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo

    def __update_display (m):
        if m.current_overlay_idx == 0:
            overlay_img = m.__generate_overlay_image(m.dist_overlay_1)
        elif m.current_overlay_idx == 1:
            overlay_img = m.__generate_overlay_image(m.dist_overlay_2)
        else:
            overlay_img = m.__generate_overlay_image(m.dist_overlay_3)
        m.__display_on_canvas(overlay_img)

    def __on_toggle (m):
        m.current_overlay_idx = (m.current_overlay_idx + 1) % 3
        m.__update_display()

    def __on_ok (m):
        m.dialog.destroy()

    def __on_cancel (m):
        m.dialog.destroy()

    def __on_save (m):
        save_path = m.image_path[:-4] + '_hm.png'
        m.__generate_overlay_image(m.dist_overlay_3).save(save_path)

    def __setup_ui (m):
        m.frm_main = tk.Frame(m.dialog, padx=10, pady=10)
        m.frm_main.pack()
        
        m.frm_btn = tk.Frame(m.frm_main)
        m.frm_btn.pack(pady=5)
        m.btn_toggle = tk.Button(m.frm_btn, text="Toggle Overlay", width=15, command=m.__on_toggle)
        m.btn_toggle.pack(side=tk.LEFT, padx=5)
        m.btn_save = tk.Button(m.frm_btn, text="Save", width=10, command=m.__on_save)
        m.btn_save.pack(side=tk.LEFT, padx=5)
        m.btn_ok = tk.Button(m.frm_btn, text="OK", width=10, command=m.__on_ok)
        m.btn_ok.pack(side=tk.LEFT, padx=5)
        m.btn_cancel = tk.Button(m.frm_btn, text="Cancel", width=10, command=m.__on_cancel)
        m.btn_cancel.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(m.frm_main, width=800, height=600, bg="white")
        m.canvas.pack(pady=5)

    def __normalize_dist_overlay (m, dist_overlay):
        valid_mask = dist_overlay >= 0
        if np.any(valid_mask):
            max_dist = np.max(dist_overlay[valid_mask])
            if max_dist > 0:
                dist_overlay = dist_overlay.astype(np.float32)
                dist_overlay[valid_mask] = (dist_overlay[valid_mask] / max_dist) * 100.0
                dist_overlay[valid_mask] = np.clip(dist_overlay[valid_mask], 0.0, 100.0)
                dist_overlay = dist_overlay.astype(np.int16)
        return dist_overlay

    def __blind_interpolate_dist_overlay (m, dist_overlay1, dist_overlay2):
        h, w = dist_overlay1.shape
        dist_overlay_interpolated = np.full((h, w), -1, dtype=np.int16)
        for y in range(h):
            for x in range(w):
                dv1 = dist_overlay1[y, x]
                dv2 = dist_overlay2[y, x]
                if dv1 < 0 and dv2 < 0:
                    dist_overlay_interpolated[y, x] = -1
                    continue
                if dv1 < 0 and dv2 >= 0:
                    dist_overlay_interpolated[y, x] = 100
                    continue
                if dv1 == m.MIN_VAL and dv2 == m.MAX_VAL:
                    value = int((m.MIN_VAL + m.MAX_VAL) / 2)
                else:
                    value = int((dv2) * (dv1 / (dv1 + m.MAX_VAL - dv2)))
                dist_overlay_interpolated[y, x] = np.clip(value, m.MIN_VAL, m.MAX_VAL)
        return dist_overlay_interpolated

    def __init__ (m, parent, image_path, color_bg, block_color):
        m.parent = parent
        m.image_path = image_path
        m.image = np.array(Image.open(image_path), dtype=np.uint8)
        m.img_h, m.img_w = m.image.shape[:2]
        m.color_bg = np.array(color_bg, dtype=np.uint8)
        m.block_color = np.array(block_color, dtype=np.uint8)
        m.dist_overlay_1 = distance_overlay_block_cpp(m.image, m.color_bg, m.block_color)
        m.dist_overlay_2 = distance_overlay_block_cpp(m.image, m.block_color, m.color_bg)
        
        m.dist_overlay_1 = m.__normalize_dist_overlay(m.dist_overlay_1)
        m.dist_overlay_2 = m.__normalize_dist_overlay(m.dist_overlay_2)
        valid_mask_2 = m.dist_overlay_2 >= 0
        if np.any(valid_mask_2):
            m.dist_overlay_2 = m.dist_overlay_2.astype(np.float32)
            m.dist_overlay_2[valid_mask_2] = 100.0 - m.dist_overlay_2[valid_mask_2]
            m.dist_overlay_2[valid_mask_2] = np.clip(m.dist_overlay_2[valid_mask_2], 0.0 - 1.0, 100.0)
            m.dist_overlay_2 = m.dist_overlay_2.astype(np.int16)
        
        m.dist_overlay_3 = m.__blind_interpolate_dist_overlay(m.dist_overlay_1, m.dist_overlay_2)
        
        m.current_overlay_idx = 0
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Get Height")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.dialog.protocol("WM_DELETE_WINDOW", m.__on_cancel)
        m.dialog.geometry("%dx%d" %(m.img_w * 2, m.img_h * 2))

        m.__setup_ui()
        m.__update_display()
        m.dialog.wait_window()

#================================================================================================================================#
#=> - Math prototyping -
#================================================================================================================================#

def try_blind_interpolation (list_size):
    dist_overlay_1 = list(range(list_size))
    dist_overlay_2 = list(range(100, 100 - list_size, -1))[::-1]
    dist_overlay_interpolated = []
    for dv1, dv2 in zip(dist_overlay_1, dist_overlay_2):
        if dv1 == 0 and dv2 == 100:
            dist_overlay_interpolated.append(50)
        else:
            dist_overlay_interpolated.append(int((dv2) * (dv1 / (dv1 + 100 - dv2)))) 
    print(dist_overlay_interpolated)

def try_blind_interpolations ():
    try_blind_interpolation (10);
    try_blind_interpolation (9);
    try_blind_interpolation (8);
    try_blind_interpolation (7);
    try_blind_interpolation (6);
    try_blind_interpolation (5);
    try_blind_interpolation (4);
    try_blind_interpolation (3);
    try_blind_interpolation (2);
    try_blind_interpolation (1);

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    img_path = "/home/w/Projects/rts-proto-map/first-test/cont001.png"
    color_bg = (32, 26, 120)
    block_color = (100, 50, 25)
    dialog = DialogGetHeight(root, img_path, color_bg, block_color)
    root.mainloop()

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
