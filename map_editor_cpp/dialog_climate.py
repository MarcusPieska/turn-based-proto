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
import copy

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

lib.distance_mask.restype = None
lib.distance_mask.argtypes = [
    ctypes.POINTER(ctypes.c_int),
    size,
    ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte)
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

def distance_mask_cpp(dist_in, lower_limit, upper_limit):
    dist_in = np.ascontiguousarray(dist_in, dtype=np.int32)
    h, w = dist_in.shape
    mask_out = np.zeros((h, w), dtype=np.uint8)
    sz = size(w, h)
    dist_ptr = dist_in.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    mask_ptr = mask_out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.distance_mask(dist_ptr, sz, lower_limit, upper_limit, mask_ptr)
    return mask_out

#================================================================================================================================#
#=> - Class: DialogClimate -
#================================================================================================================================#

class DialogClimate:

    def __on_slider_changed (m, slider_idx, value):
        if slider_idx == m.num_sliders - 1:
            m.slider_values[slider_idx] = 100
            m.sliders[slider_idx].set(100)
            return
        val = int(float(value))
        m.slider_values[slider_idx] = val
        if slider_idx > 0 and val < m.slider_values[slider_idx - 1]:
            m.slider_values[slider_idx] = m.slider_values[slider_idx - 1]
            m.sliders[slider_idx].set(m.slider_values[slider_idx - 1])
        elif slider_idx < m.num_sliders - 1:
            max_val = m.slider_values[slider_idx + 1] if slider_idx + 1 < m.num_sliders - 1 else 100
            if val > max_val:
                m.slider_values[slider_idx] = max_val
                m.sliders[slider_idx].set(max_val)
        m.slider_values[m.num_sliders - 1] = 100
        m.__update_pixel_counts()
        m.__update_display()
    
    def __update_pixel_counts (m):
        pass

    def __generate_overlay_image (m):
        if m.dist_overlay is None:
            return None
        valid_mask = m.dist_overlay >= 0
        if not np.any(valid_mask):
            return None
        max_dist = np.max(m.dist_overlay[valid_mask])
        if max_dist <= 0:
            return None
        normalized = np.zeros_like(m.dist_overlay, dtype=np.uint8)
        normalized[valid_mask] = (m.dist_overlay[valid_mask].astype(np.float32) / max_dist * 255).astype(np.uint8)
        overlay_img = np.stack([normalized, normalized, normalized], axis=2)
        if m.image.ndim == 3:
            bg_mask = np.all(m.image == m.color_bg, axis=2)
            overlay_img[bg_mask] = m.color_bg
        else:
            bg_mask = (m.image == m.color_bg)
            overlay_img[bg_mask] = m.color_bg
        return Image.fromarray(overlay_img, 'RGB')

    def __apply_climate (m):
        if m.dist_overlay is None or len(m.sorted_pixels) == 0:
            return
        result = m.image.copy()
        total_pixels = len(m.sorted_pixels)
        percentages = [s.get() for s in m.sliders]
        colors = copy.deepcopy(m.climate_colors)
        for pixel_idx, (y, x) in enumerate(m.sorted_pixels):
            current_percentage = (pixel_idx + 1) * 100.0 / total_pixels
            if current_percentage > percentages[0] and len(colors) > 1:
                colors.pop(0)
                percentages.pop(0)
            if result.ndim == 3:
                result[y, x, 0] = colors[0][0]
                result[y, x, 1] = colors[0][1]
                result[y, x, 2] = colors[0][2]
            else:
                result[y, x] = colors[0][0]
        return Image.fromarray(result, 'RGB')

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
        if m.result_image is not None:
            m.result_image = m.__apply_climate()
            m.__display_on_canvas(m.result_image)
        else:
            m.__display_on_canvas(m.__generate_overlay_image())

    def __on_apply (m):
        m.result_image = m.image.copy()
        m.__update_display()

    def __on_reset (m):
        m.result_image = None
        m.__update_display()

    def __on_ok (m):
        m.dialog.destroy()

    def __on_cancel (m):
        m.result_image = None
        m.dialog.destroy()

    def __setup_ui (m):
        m.frm_main = tk.Frame(m.dialog, padx=10, pady=10)
        m.frm_main.pack()
        
        m.canvas = tk.Canvas(m.frm_main, width=800, height=600, bg="white")
        m.canvas.pack(pady=5)
        
        m.frm_slider = tk.Frame(m.frm_main)
        m.frm_slider.pack(pady=5)
        
        m.sliders = []
        step = 100 // m.num_sliders
        m.slider_values = [int(step * (i + 1)) for i in range(m.num_sliders - 1)] + [100]
        for i, value in enumerate(m.slider_values):
            slider = tk.Scale(m.frm_slider, from_=0, to=100, orient=tk.HORIZONTAL, length=300)
            slider.set(value)
            slider.configure(command=lambda v, idx=i: m.__on_slider_changed(idx, v))
            slider.pack(side=tk.TOP, padx=5)
            m.sliders.append(slider)
        m.sliders[-1].configure(state=tk.DISABLED)
        
        m.frm_btn = tk.Frame(m.frm_main)
        m.frm_btn.pack(pady=5)
        m.btn_apply = tk.Button(m.frm_btn, text="Apply", width=10, command=m.__on_apply)
        m.btn_apply.pack(side=tk.LEFT, padx=5)
        m.btn_reset = tk.Button(m.frm_btn, text="Reset", width=10, command=m.__on_reset)
        m.btn_reset.pack(side=tk.LEFT, padx=5)
        m.btn_ok = tk.Button(m.frm_btn, text="OK", width=10, command=m.__on_ok)
        m.btn_ok.pack(side=tk.LEFT, padx=5)
        m.btn_cancel = tk.Button(m.frm_btn, text="Cancel", width=10, command=m.__on_cancel)
        m.btn_cancel.pack(side=tk.LEFT, padx=5)

    def __init__ (m, parent, image, color_bg, block_color, climate_colors, ignore_colors):
        m.parent = parent
        m.result_image = None
        if isinstance(image, Image.Image):
            m.image = np.array(image, dtype=np.uint8)
        else:
            m.image = np.ascontiguousarray(image, dtype=np.uint8)
        m.img_h, m.img_w = m.image.shape[:2]
        m.color_bg = np.array(color_bg, dtype=np.uint8)
        m.block_color = np.array(block_color, dtype=np.uint8)
        m.climate_colors = [np.array(color, dtype=np.uint8) for color in climate_colors]
        m.ignore_colors = [np.array(color, dtype=np.uint8) for color in ignore_colors]
        m.num_sliders = len(m.climate_colors)
        m.dist_overlay = distance_overlay_block_cpp(m.image, m.color_bg, m.block_color)

        h, n = m.img_h, 2
        mid_h = h // 2
        modified_overlay = m.dist_overlay.copy().astype(np.float32)
        additive = 0
        for row in range(mid_h):
            additive += n
            modified_overlay[row, :] += additive
        for row in range(mid_h, h):
            additive = max(0, additive - n)
            modified_overlay[row, :] += additive
        
        ignore_mask = np.zeros((h, m.img_w), dtype=bool)
        if m.image.ndim == 3:
            for ignore_color in m.ignore_colors:
                color_match = np.all(m.image == ignore_color, axis=2)
                ignore_mask = ignore_mask | color_match
        else:
            for ignore_color in m.ignore_colors:
                color_match = (m.image == ignore_color[0])
                ignore_mask = ignore_mask | color_match
        
        valid_mask = (modified_overlay >= 0) & (~ignore_mask)
        valid_coords = np.where(valid_mask)
        valid_values = modified_overlay[valid_mask]
        sort_indices = np.argsort(valid_values)
        m.sorted_pixels = list(zip(valid_coords[0][sort_indices], valid_coords[1][sort_indices]))
        m.dist_overlay = modified_overlay.astype(np.int32)
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Climate")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.dialog.protocol("WM_DELETE_WINDOW", m.__on_cancel)
        m.dialog.geometry("850x850")
        m.__setup_ui()
        m.__update_display()
        m.dialog.wait_window()

    def get_image (m):
        return m.result_image

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    pass

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
