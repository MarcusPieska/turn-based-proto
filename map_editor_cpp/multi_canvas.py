#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from PIL import Image, ImageTk
import numpy as np
import ctypes

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "img_mng.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile img_mng.cpp first." %(lib_path))

lib = ctypes.CDLL(lib_path)

class pt(ctypes.Structure): _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]
class size(ctypes.Structure): _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.rotate_img.restype = None
lib.rotate_img.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    pt, 
    ctypes.c_double
]

lib.get_outline.restype = None
lib.get_outline.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte)
]

lib.apply_outline.restype = None
lib.apply_outline.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte)
]

def rotate_image_cpp(img, def_col, cx, cy, rot_deg):
    img = np.ascontiguousarray(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(def_col, (list, tuple)):
        def_col = np.array(def_col, dtype=np.uint8)
    if def_col.size != ch:
        def_col = np.array([def_col[0]], dtype=np.uint8) if ch == 1 else np.array(def_col[:ch], dtype=np.uint8)
    out = np.zeros_like(img)
    sz = size(w, h)
    cntr = pt(float(cx), float(cy))
    in_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    out_ptr = out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    col_ptr = def_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.rotate_img(in_ptr, out_ptr, col_ptr, sz, ch, cntr, rot_deg)
    if ch == 1:
        out = out[:, :, 0]
    return out

def get_outline_cpp(img, bg_col):
    img = np.ascontiguousarray(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(bg_col, (list, tuple)):
        bg_col = np.array(bg_col, dtype=np.uint8)
    if bg_col.size != ch:
        bg_col = np.array([bg_col[0]], dtype=np.uint8) if ch == 1 else np.array(bg_col[:ch], dtype=np.uint8)
    outline_out = np.zeros((h, w, 3), dtype=np.uint8)
    sz = size(w, h)
    img_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    outline_ptr = outline_out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    bg_ptr = bg_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.get_outline(img_ptr, outline_ptr, sz, ch, bg_ptr)
    return outline_out

def apply_outline_cpp(img_main, img_outline, color_outline):
    img_main = np.ascontiguousarray(img_main, dtype=np.uint8)
    img_outline = np.ascontiguousarray(img_outline, dtype=np.uint8)
    if img_main.ndim == 2:
        h, w = img_main.shape
        ch = 1
        img_main = img_main[:, :, np.newaxis]
    else:
        h, w, ch = img_main.shape
    if isinstance(color_outline, (list, tuple)):
        color_outline = np.array(color_outline, dtype=np.uint8)
    if color_outline.size != ch:
        color_outline = np.array([color_outline[0]], dtype=np.uint8) if ch == 1 else np.array(color_outline[:ch], dtype=np.uint8)
    sz = size(w, h)
    main_ptr = img_main.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    outline_ptr = img_outline.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    color_ptr = color_outline.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.apply_outline(main_ptr, outline_ptr, sz, ch, color_ptr)
    if ch == 1:
        img_main = img_main[:, :, 0]
    return img_main

#================================================================================================================================#
#=> - CycleSelectCanvas class -
#================================================================================================================================#

class CycleSelectCanvas:

    def __get_checked_indices (m):
        return [i for i in range(len(m.checkboxes)) if m.checkboxes[i].get()]

    def __find_next_checked (m, start_idx, direction):
        checked = m.__get_checked_indices()
        if not checked:
            return -1
        if start_idx in checked:
            idx = checked.index(start_idx)
            if direction > 0:
                if idx < len(checked) - 1:
                    return checked[idx + 1]
            else:
                if idx > 0:
                    return checked[idx - 1]
        if direction > 0:
            for idx in checked:
                if idx > start_idx:
                    return idx
            return checked[0] if checked else -1
        else:
            for idx in reversed(checked):
                if idx < start_idx:
                    return idx
            return checked[-1] if checked else -1

    def __on_prev (m):
        checked = m.__get_checked_indices()
        if not checked:
            return
        new_idx = m.__find_next_checked(m.current_index, -1)
        if new_idx >= 0:
            m.current_index = new_idx
            m.__display_current()

    def __on_next (m):
        checked = m.__get_checked_indices()
        if not checked:
            return
        new_idx = m.__find_next_checked(m.current_index, 1)
        if new_idx >= 0:
            m.current_index = new_idx
            m.__display_current()

    def __display_image (m, img):
        if img is None:
            return
        img_w, img_h = img.size
        zoom_x, zoom_y = m.canvas_width // img_w, m.canvas_height // img_h
        zoom = max(min(zoom_x, zoom_y), 1)
        new_w, new_h = img_w * zoom, img_h * zoom
        img_zoomed = img.resize((new_w, new_h), Image.Resampling.NEAREST)
        photo = ImageTk.PhotoImage(img_zoomed)
        m.canvas.delete("all")
        m.canvas.create_image(m.canvas_width // 2, m.canvas_height // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo
        m.canvas_zoom = zoom
        m.canvas_offset_x, m.canvas_offset_y = (m.canvas_width - new_w) // 2, (m.canvas_height - new_h) // 2

    def __display_current (m):
        if not m.images or m.current_index < 0 or m.current_index >= len(m.images):
            return
        img = m.images[m.current_index]
        if img is None:
            return
        m.__display_image(img)
        m.rotation_slider.set(0)
        m.__update_button_states()

    def __update_button_states (m):
        checked = m.__get_checked_indices()
        if not checked:
            m.btn_prev.config(state=tk.DISABLED)
            m.btn_next.config(state=tk.DISABLED)
        else:
            has_prev = m.__find_next_checked(m.current_index, -1) != m.current_index
            has_next = m.__find_next_checked(m.current_index, 1) != m.current_index
            m.btn_prev.config(state=tk.NORMAL if has_prev else tk.DISABLED)
            m.btn_next.config(state=tk.NORMAL if has_next else tk.DISABLED)

    def __on_checkbox_changed (m, idx):
        if not m.checkboxes[idx].get():
            checked = m.__get_checked_indices()
            if m.current_index == idx:
                if checked:
                    new_idx = checked[0]
                    m.current_index = new_idx
                    m.__display_current()
                else:
                    m.canvas.delete("all")
                    m.canvas.image = None
        m.__update_button_states()

    def __on_merge (m):
        checked = m.__get_checked_indices()
        if len(checked) < 2 or m.current_index < 0 or m.current_index >= len(m.images):
            return
        if m.current_index not in checked:
            return
        base_img = m.images[m.current_index].copy()
        base_array = np.array(base_img)
        color_def_array = np.array(m.color_def)
        for idx in checked:
            if idx == m.current_index:
                continue
            if idx >= len(m.images):
                continue
            src_img = m.images[idx]
            src_array = np.array(src_img)
            mask = ~np.all(src_array == color_def_array, axis=2)
            base_array[mask] = src_array[mask]
        merged_img = Image.fromarray(base_array, 'RGB')
        m.images[m.current_index] = merged_img
        to_remove = [idx for idx in checked if idx != m.current_index and idx < len(m.images)]
        checkbox_widgets = list(m.checkbox_frame.winfo_children())
        for idx in sorted(to_remove, reverse=True):
            m.images.pop(idx)
            if idx < len(checkbox_widgets):
                checkbox_widgets[idx].destroy()
            m.checkboxes.pop(idx)
        for i in range(len(m.checkboxes)):
            remaining_widgets = m.checkbox_frame.winfo_children()
            if i < len(remaining_widgets):
                cb = remaining_widgets[i]
                cb.config(command=lambda idx=i: m.__on_checkbox_changed(idx))
        if m.current_index >= len(m.images):
            m.current_index = len(m.images) - 1 if m.images else -1
        m.rotation_slider.set(0)
        m.__display_current()

    def __on_outline (m):
        checked = m.__get_checked_indices()
        if len(checked) < 2 or m.current_index < 0 or m.current_index >= len(m.images):
            return
        if m.current_index not in checked:
            return
        outlined_img = m.images[m.current_index].copy()
        outlined_array = np.array(outlined_img)
        color_white = np.array([255, 255, 255], dtype=np.uint8)
        for idx in checked:
            if idx == m.current_index:
                continue
            src_img = m.images[idx]
            outline = get_outline_cpp(src_img, m.color_def)
            outlined_array = apply_outline_cpp(outlined_array, outline, color_white)
        temp_img = Image.fromarray(outlined_array, 'RGB')
        m.__display_image(temp_img)

    def __on_rotation_changed (m, value):
        if m.current_index < 0 or m.current_index >= len(m.images):
            return
        img = m.images[m.current_index]
        if img is None:
            return
        rotation = float(value)
        img_array = np.array(img)
        img_h, img_w = img_array.shape[:2]
        center_x, center_y = img_w / 2.0, img_h / 2.0
        rotated_array = rotate_image_cpp(img_array, m.color_def, center_x, center_y, rotation)
        rotated_img = Image.fromarray(rotated_array, 'RGB')
        m.images[m.current_index] = rotated_img
        m.__display_current()

    def __on_canvas_press (m, event):
        m.drag_start_x, m.drag_start_y = event.x, event.y
        m.dragging = True

    def __on_canvas_drag (m, event):
        if not m.dragging or m.current_index < 0 or m.current_index >= len(m.images):
            return
        img = m.images[m.current_index]
        if img is None:
            return
        img_w, img_h = img.size
        canvas_x, canvas_y = event.x, event.y
        img_x = int((canvas_x - m.canvas_offset_x) / m.canvas_zoom)
        img_y = int((canvas_y - m.canvas_offset_y) / m.canvas_zoom)
        start_img_x = int((m.drag_start_x - m.canvas_offset_x) / m.canvas_zoom)
        start_img_y = int((m.drag_start_y - m.canvas_offset_y) / m.canvas_zoom)
        dx, dy = img_x - start_img_x, img_y - start_img_y
        if dx == 0 and dy == 0:
            return
        new_img = Image.new("RGB", (img_w, img_h), m.color_def)
        src_x0, src_y0 = max(0, -dx), max(0, -dy)
        src_x1, src_y1 = min(img_w, img_w - dx), min(img_h, img_h - dy)
        dst_x0, dst_y0 = max(0, dx), max(0, dy)
        dst_x1, dst_y1 = min(img_w, img_w + dx), min(img_h, img_h + dy)
        if src_x1 > src_x0 and src_y1 > src_y0 and dst_x1 > dst_x0 and dst_y1 > dst_y0:
            crop_box = (src_x0, src_y0, src_x1, src_y1)
            cropped = img.crop(crop_box)
            new_img.paste(cropped, (dst_x0, dst_y0))
        m.images[m.current_index] = new_img
        m.__display_current()
        m.drag_start_x, m.drag_start_y = event.x, event.y

    def __on_canvas_release (m, event):
        if m.dragging:
            m.rotation_slider.set(0)
        m.dragging = False

    def __init__ (m, parent, width, height, color_def):
        m.canvas_width, m.canvas_height = width, height
        m.color_def = color_def
        m.images, m.checkboxes = [], []
        m.current_index = -1
        m.dragging = False
        m.drag_start_x, m.drag_start_y = 0, 0
        m.canvas_offset_x, m.canvas_offset_y = 0, 0
        m.canvas_zoom = 1
        container = tk.Frame(parent)
        m.canvas = tk.Canvas(container, width=width, height=height, bg="white")
        m.canvas.pack(pady=5)
        m.canvas.bind("<Button-1>", m.__on_canvas_press)
        m.canvas.bind("<B1-Motion>", m.__on_canvas_drag)
        m.canvas.bind("<ButtonRelease-1>", m.__on_canvas_release)
        m.btn_frame = tk.Frame(container)
        m.btn_frame.pack(pady=5, fill=tk.X)
        m.btn_prev = tk.Button(m.btn_frame, text="<", width=5, command=m.__on_prev)
        m.btn_prev.pack(side=tk.LEFT, padx=5)
        m.btn_next = tk.Button(m.btn_frame, text=">", width=5, command=m.__on_next)
        m.btn_next.pack(side=tk.LEFT, padx=5)
        m.checkbox_frame = tk.Frame(m.btn_frame)
        m.checkbox_frame.pack(side=tk.RIGHT, padx=5)
        m.btn_outline = tk.Button(m.btn_frame, text="Outline", width=8, command=m.__on_outline)
        m.btn_outline.pack(side=tk.RIGHT, padx=5)
        m.btn_merge = tk.Button(m.btn_frame, text="Merge", width=8, command=m.__on_merge)
        m.btn_merge.pack(side=tk.RIGHT, padx=5)
        m.rotation_frame = tk.Frame(m.btn_frame)
        m.rotation_frame.pack(side=tk.RIGHT, padx=5)
        tk.Label(m.rotation_frame, text="Rotation:").pack(side=tk.LEFT, padx=2)
        m.rotation_slider = tk.Scale(m.rotation_frame, from_=0, to=360, orient=tk.HORIZONTAL, length=150)
        m.rotation_slider.configure(command=m.__on_rotation_changed)
        m.rotation_slider.set(0)
        m.rotation_slider.pack(side=tk.LEFT, padx=2)
        m.container = container

    def add_image (m, img, path=None):
        m.images.append(img.copy())
        var = tk.BooleanVar(value=True)
        m.checkboxes.append(var)
        cb = tk.Checkbutton(m.checkbox_frame, variable=var, command=lambda: m.__on_checkbox_changed(len(m.checkboxes)))
        cb.pack(side=tk.LEFT, padx=2)
        m.current_index = max(m.current_index, len(m.images) - 1)
        m.__display_current()

    def set_current_index (m, idx):
        if 0 <= idx < len(m.images):
            m.current_index = idx
            m.__display_current()

    def get_current_index (m):
        return m.current_index

    def get_current_image (m):
        if m.current_index >= 0 and m.current_index < len(m.images):
            return m.images[m.current_index]
        return None

    def update_current_image (m, img):
        if m.current_index >= 0 and m.current_index < len(m.images):
            m.images[m.current_index] = img
            m.rotation_slider.set(0)
            m.__display_current()

    def get_all_images (m):
        return m.images

    def get_widget (m):
        return m.container

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
