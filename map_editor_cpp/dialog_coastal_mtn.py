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
import random

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "map_overlays.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile map_overlays.cpp first." %(lib_path))

lib = ctypes.CDLL(lib_path)

class pt(ctypes.Structure): _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]
class size(ctypes.Structure): _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.distance_overlay.restype = None
lib.distance_overlay.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
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

def distance_overlay_cpp(img, bg_col):
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
    dist_out = np.full((h, w), -1, dtype=np.int32)
    sz = size(w, h)
    img_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    bg_ptr = bg_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    dist_ptr = dist_out.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    lib.distance_overlay(img_ptr, sz, ch, bg_ptr, dist_ptr)
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
#=> - Class: DialogCoastalMtn -
#================================================================================================================================#

class DialogCoastalMtn:

    def VALID_COORDS (m, x, y): return 0 <= x < m.img_w and 0 <= y < m.img_h

    def __generate_overlay_image (m):
        if m.dist_overlay is None:
            return None
        max_dist = np.max(m.dist_overlay)
        if max_dist <= 0:
            return None
        normalized = (m.dist_overlay.astype(np.float32) / max_dist * 255).astype(np.uint8)
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
        m.canvas_zoom = zoom
        m.canvas_offset_x = (canvas_w - new_w) // 2
        m.canvas_offset_y = (canvas_h - new_h) // 2

    def __on_canvas_click (m, event):
        canvas_x, canvas_y = event.x, event.y
        img_x = int((canvas_x - m.canvas_offset_x) / m.canvas_zoom)
        img_y = int((canvas_y - m.canvas_offset_y) / m.canvas_zoom)
        if 0 <= img_x < m.img_w and 0 <= img_y < m.img_h:
            m.last_click_x, m.last_click_y = img_x, img_y
            m.label_coord.config(text="Coordinate: (%d, %d)" % (img_x, img_y))
            if m.dist_overlay is not None:
                overlay_val = m.dist_overlay[img_y, img_x]
                m.label_overlay.config(text="Overlay: %d" % overlay_val)
            else:
                m.label_overlay.config(text="Overlay: N/A")

    def __on_pixel_count_changed (m, value):
        m.pixel_count = int(float(value))

    def __on_thickness_changed (m, value):
        m.thickness = int(float(value))

    def __add_mtn_line_noise (m, coords):
        noise_points = []
        for x, y in coords:
            if not m.VALID_COORDS(x, y):
                continue
            has_adjacent_non_mountain = False
            adj_offsets = [(-1, 0), (1, 0), (0, -1), (0, 1)]
            for dx, dy in adj_offsets:
                nx, ny = x + dx, y + dy
                if m.VALID_COORDS(nx, ny):
                    if not np.array_equal(m.result_image[ny, nx], m.color_new):
                        has_adjacent_non_mountain = True
                        break
            if has_adjacent_non_mountain and random.random() < 0.1:
                noise_points.append((x, y))
        if not noise_points:
            return
        start_x, start_y = m.last_click_x, m.last_click_y
        if not m.VALID_COORDS(start_x, start_y) or m.dist_overlay is None:
            return
        max_pixels = 1 if m.dist_overlay[start_y, start_x] < 1 else m.dist_overlay[start_y, start_x] 
        for noise_x, noise_y in noise_points:
            pixel_count = random.randint(1, max_pixels)
            queue = [(noise_x, noise_y)]
            visited = set()
            colored = 0
            while queue and colored < pixel_count:
                x, y = queue.pop(0)
                if (x, y) in visited or not m.VALID_COORDS(x, y) or np.array_equal(m.result_image[y, x], m.color_new):
                    continue
                m.result_image[y, x] = m.color_new
                coords.append((x, y))
                visited.add((x, y))
                colored += 1
                for dx, dy in adj_offsets:
                    nx, ny = x + dx, y + dy
                    if (nx, ny) not in visited and 0 <= nx < m.img_w and 0 <= ny < m.img_h:
                        if not np.array_equal(m.result_image[ny, nx], m.color_new):
                            queue.append((nx, ny))
                adj_offsets = [adj_offsets[-1]] + adj_offsets[:-1]

    def __cull_mtn_line_corners (m, coords):
        if m.result_image is None:
            return
        corner_points = []
        for x, y in coords:
            if not m.VALID_COORDS(x, y):
                continue
            if not np.array_equal(m.result_image[y, x], m.color_new):
                continue
            non_new_count = 0
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                nx, ny = x + dx, y + dy
                if m.VALID_COORDS(nx, ny):
                    if not np.array_equal(m.result_image[ny, nx], m.color_new):
                        non_new_count += 1
            if non_new_count == 2:
                corner_points.append((x, y))
        for x, y in corner_points:
            if m.VALID_COORDS(x, y):
                m.result_image[y, x] = m.color_old

    def __on_generate (m):
        if m.last_click_x < 0 or m.last_click_y < 0:
            return
        if m.result_image is None:
            m.result_image = m.image.copy()
        start_x, start_y = m.last_click_x, m.last_click_y
        if not m.VALID_COORDS(start_x, start_y):
            return
        if not np.array_equal(m.result_image[start_y, start_x], m.color_old) or m.dist_overlay is None:
            return
        start_overlay = m.dist_overlay[start_y, start_x]
        if start_overlay < 0:
            return
        queue = [(start_x, start_y)]
        colored_count = 0
        visited = set()
        coords = []
        while queue and colored_count < m.pixel_count:
            x, y = queue.pop(0)
            if (x, y) in visited or not m.VALID_COORDS(x, y):
                continue
            if not np.array_equal(m.result_image[y, x], m.color_old):
                continue
            current_overlay = m.dist_overlay[y, x]
            if current_overlay < 0 or abs(current_overlay - start_overlay) > m.thickness:
                continue
            m.result_image[y, x] = m.color_new
            visited.add((x, y))
            colored_count += 1
            for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                nx, ny = x + dx, y + dy
                if (nx, ny) not in visited:
                    queue.append((nx, ny))
                    coords.append((nx, ny))
        m.__add_mtn_line_noise(coords)
        m.__cull_mtn_line_corners(coords)
        m.__update_display()

    def __on_reset (m):
        m.result_image = None
        m.last_click_x, m.last_click_y = -1, -1
        m.label_coord.config(text="Coordinate: (-, -)")
        m.label_overlay.config(text="Overlay: -")
        m.__update_display()

    def __update_display (m):
        if m.result_image is not None:
            result_img = Image.fromarray(m.result_image, 'RGB')
            m.__display_on_canvas(result_img)
        else:
            overlay_img = m.__generate_overlay_image()
            if overlay_img is not None:
                m.__display_on_canvas(overlay_img)

    def __on_ok (m):
        m.dialog.destroy()

    def __on_cancel (m):
        m.result_image = None
        m.dialog.destroy()

    def __setup_ui (m):
        main_frame = tk.Frame(m.dialog, padx=10, pady=10)
        main_frame.pack()
        m.canvas = tk.Canvas(main_frame, width=800, height=600, bg="white")
        m.canvas.pack(pady=5)
        m.canvas.bind("<Button-1>", m.__on_canvas_click)
        controls_frame = tk.Frame(main_frame)
        controls_frame.pack(pady=5)
        pixel_count_frame = tk.Frame(controls_frame)
        pixel_count_frame.pack(side=tk.LEFT, padx=10)
        tk.Label(pixel_count_frame, text="Pixel count:").pack(side=tk.LEFT, padx=5)
        m.pixel_count_slider = tk.Scale(pixel_count_frame, from_=10, to=1500, orient=tk.HORIZONTAL, length=150)
        m.pixel_count_slider.configure(command=m.__on_pixel_count_changed)
        m.pixel_count_slider.set(m.pixel_count)
        m.pixel_count_slider.pack(side=tk.LEFT, padx=5)
        thickness_frame = tk.Frame(controls_frame)
        thickness_frame.pack(side=tk.LEFT, padx=10)
        tk.Label(thickness_frame, text="Thickness:").pack(side=tk.LEFT, padx=5)
        m.thickness_slider = tk.Scale(thickness_frame, from_=1, to=10, orient=tk.HORIZONTAL, length=150)
        m.thickness_slider.configure(command=m.__on_thickness_changed)
        m.thickness_slider.set(m.thickness)
        m.thickness_slider.pack(side=tk.LEFT, padx=5)
        m.label_coord = tk.Label(controls_frame, text="Coordinate: (-, -)")
        m.label_coord.pack(side=tk.LEFT, padx=10)
        m.label_overlay = tk.Label(controls_frame, text="Overlay: -")
        m.label_overlay.pack(side=tk.LEFT, padx=10)
        btn_frame = tk.Frame(main_frame)
        btn_frame.pack(pady=5)
        btn_generate = tk.Button(btn_frame, text="Generate", width=10, command=m.__on_generate)
        btn_generate.pack(side=tk.LEFT, padx=5)
        btn_reset = tk.Button(btn_frame, text="Reset", width=10, command=m.__on_reset)
        btn_reset.pack(side=tk.LEFT, padx=5)
        btn_ok = tk.Button(btn_frame, text="OK", width=10, command=m.__on_ok)
        btn_ok.pack(side=tk.LEFT, padx=5)
        btn_cancel = tk.Button(btn_frame, text="Cancel", width=10, command=m.__on_cancel)
        btn_cancel.pack(side=tk.LEFT, padx=5)

    def __init__ (m, parent, image, color_bg, color_old, color_new):
        m.parent = parent
        m.result_image = None
        m.last_click_x, m.last_click_y = -1, -1
        m.pixel_count = 50
        m.thickness = 2
        if isinstance(image, Image.Image):
            m.image = np.array(image, dtype=np.uint8)
        else:
            m.image = np.ascontiguousarray(image, dtype=np.uint8)
        m.img_h, m.img_w = m.image.shape[:2]
        m.color_bg = np.array(color_bg, dtype=np.uint8)
        m.color_old = np.array(color_old, dtype=np.uint8)
        m.color_new = np.array(color_new, dtype=np.uint8)
        m.dist_overlay = distance_overlay_cpp(m.image, m.color_bg)
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Coastal Mountain")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.dialog.protocol("WM_DELETE_WINDOW", m.__on_cancel)
        m.dialog.geometry("850x750")
        m.__setup_ui()
        m.__update_display()
        m.dialog.wait_window()

    def get_image (m):
        if m.result_image is None:
            return None
        return Image.fromarray(m.result_image, 'RGB')

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    from dialog_new_cont import DialogNewCont
    root = tk.Tk()
    root.title("Coastal Mountain Test")
    root.geometry("800x600")
    cont_dialog = DialogNewCont(root, 800, 600, 1, (32, 26, 120), (121, 189, 36))
    cont_image = cont_dialog.get_image()
    if cont_image is not None:
        coastal_dialog = DialogCoastalMtn(root, cont_image, (32, 26, 120), (121, 189, 36), (100, 50, 25))
        coastal_dialog.get_image()
    root.mainloop()

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
