#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
import random
import math
from line_gen import LineGenerator
import numpy as np
from PIL import Image, ImageTk
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

lib.flood_fill.restype = None
lib.flood_fill.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte),
    pt
]

def rotate_image_cpp(img, def_col, cx, cy, rot_deg):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
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

def flood_fill_cpp(img, lim_col, fill_col, start_x, start_y):
    img = np.ascontiguousarray(img, dtype=np.uint8)
    original_shape = img.shape
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(lim_col, (list, tuple)):
        lim_col = np.array(lim_col, dtype=np.uint8)
    if isinstance(fill_col, (list, tuple)):
        fill_col = np.array(fill_col, dtype=np.uint8)
    if lim_col.size != ch:
        lim_col = np.array([lim_col[0]], dtype=np.uint8) if ch == 1 else np.array(lim_col[:ch], dtype=np.uint8)
    if fill_col.size != ch:
        fill_col = np.array([fill_col[0]], dtype=np.uint8) if ch == 1 else np.array(fill_col[:ch], dtype=np.uint8)
    sz = size(w, h)
    start_pt = pt(float(start_x), float(start_y))
    img_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lim_ptr = lim_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    fill_ptr = fill_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.flood_fill(img_ptr, sz, ch, lim_ptr, fill_ptr, start_pt)
    if len(original_shape) == 2:
        img = img[:, :, 0]
    return img

#================================================================================================================================#
#=> - Class: DialogNewCont -
#================================================================================================================================#

class DialogNewCont:

    def __generate_angles (m):
        m.angles, m.depths, m.points = [], [], []
        current_angle = 0.0
        while current_angle < 360.0:
            angle_step = random.uniform(m.min_angle_step, m.max_angle_step)
            current_angle += angle_step
            if current_angle >= 360.0:
                break
            m.angles.append(current_angle)

    def __generate_depths (m):
        if not m.angles:
            return
        m.depths, m.points = [], []
        prev_depth, prev_change_was_sharp = 0.0, False
        for angle in m.angles:
            if not prev_change_was_sharp:
                depth = random.uniform(m.min_depth, m.max_depth)
            m.depths.append(depth)
            prev_change_was_sharp = abs(depth - prev_depth) > 0.1
            prev_depth = depth

    def __calculate_points (m):
        if not m.angles or not m.depths or len(m.angles) != len(m.depths):
            return
        center_x, center_y = m.canvas_width // 2, m.canvas_height // 2
        max_radius = min(center_x, center_y) * 0.9
        m.points = []
        for i in range(len(m.angles)):
            radius = max_radius * m.depths[i]
            angle_rad = math.radians(m.angles[i])
            m.points.append((center_x + radius * math.cos(angle_rad), center_y - radius * math.sin(angle_rad)))

    def __remove_outline (m, image_matrix):
        white_mask = np.all(image_matrix == m.color_white, axis=2)
        image_matrix[white_mask] = m.color_bg

    def __fill_polygon (m, image_matrix):
        if len(m.points) < 3:
            return
        img_h, img_w, _ = image_matrix.shape
        center_x, center_y = int(m.canvas_width * m.zoom // 2), int(m.canvas_height * m.zoom // 2)
        if not np.array_equal(image_matrix[center_y, center_x], m.color_bg):
            return
        image_matrix[:] = flood_fill_cpp(image_matrix, m.color_white, m.color_fill, center_x, center_y)

    def __generate_lines (m):
        if not m.points:
            m.__calculate_points()
        if not m.points:
            return None
        img_w, img_h = int(m.canvas_width * m.zoom), int(m.canvas_height * m.zoom)
        image_matrix = np.full((img_h, img_w, 3), m.color_bg, dtype=np.uint8)
        line_pxs = []
        for i in range(len(m.points)):
            p1 = m.points[i]
            p2 = m.points[(i + 1) % len(m.points)]
            x1, y1 = p1[0] * m.zoom, p1[1] * m.zoom
            x2, y2 = p2[0] * m.zoom, p2[1] * m.zoom
            dx, dy = x2 - x1, y2 - y1
            length = math.sqrt(dx * dx + dy * dy) / m.zoom
            angle = math.degrees(math.atan2(-dy, dx))
            pxs = m.line_gen.generateLinePixels(line_length=length, angle=angle, width=img_w, height=img_h, zoom=m.zoom)
            for px, py in pxs:
                img_x, img_y = px + int(x1 - img_w // 2), py + int(y1 - img_h // 2)
                if 0 <= img_x < img_w and 0 <= img_y < img_h:
                    image_matrix[img_y, img_x] = m.color_white
        m.__fill_polygon(image_matrix)
        m.__remove_outline(image_matrix)
        return image_matrix

    def __generate_continent (m):
        m.__generate_angles()
        m.__generate_depths()
        image_matrix = m.__generate_lines()
        if image_matrix is None:
            return None
        img = Image.fromarray(image_matrix, 'RGB')
        return img

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

    def __on_generate (m):
        base_rotation = m.rotation
        m.rotation = 0.0
        m.base_image = m.__generate_continent()
        m.rotation = base_rotation
        if m.base_image is not None:
            m.__apply_rotation_to_current()

    def __on_rotation_changed (m, value):
        m.rotation = float(value)
        if m.base_image is not None:
            m.__apply_rotation_to_current()

    def __apply_rotation_to_current (m):
        if m.base_image is None:
            return
        if m.rotation == 0.0:
            m.result_image = m.base_image.copy()
        else:
            img_array = np.array(m.base_image)
            img_h, img_w = img_array.shape[:2]
            center_x, center_y = img_w / 2.0, img_h / 2.0
            rotated_array = rotate_image_cpp(img_array, m.color_bg, center_x, center_y, m.rotation)
            m.result_image = Image.fromarray(rotated_array, 'RGB')
        m.__display_on_canvas(m.result_image)

    def __on_scale_changed (m, value):
        m.scale = float(value)
        m.min_depth, m.max_depth = 0.3 * m.scale, 0.9 * m.scale

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
        sliders_frame = tk.Frame(main_frame)
        sliders_frame.pack(pady=5)
        rotation_frame = tk.Frame(sliders_frame)
        rotation_frame.pack(side=tk.LEFT, padx=10)
        tk.Label(rotation_frame, text="Rotation:").pack(side=tk.LEFT, padx=5)
        m.rotation_slider = tk.Scale(rotation_frame, from_=0, to=360, orient=tk.HORIZONTAL, length=200)
        m.rotation_slider.configure(command=m.__on_rotation_changed)
        m.rotation_slider.set(int(m.rotation))
        m.rotation_slider.pack(side=tk.LEFT, padx=5)
        scale_frame = tk.Frame(sliders_frame)
        scale_frame.pack(side=tk.LEFT, padx=10)
        tk.Label(scale_frame, text="Scale:").pack(side=tk.LEFT, padx=5)
        m.scale_slider = tk.Scale(scale_frame, from_=0.1, to=1.0, resolution=0.1, orient=tk.HORIZONTAL, length=200)
        m.scale_slider.configure(command=m.__on_scale_changed)
        m.scale_slider.set(m.scale)
        m.scale_slider.pack(side=tk.LEFT, padx=5)
        
        btn_frame = tk.Frame(main_frame)
        btn_frame.pack(pady=5)
        btn_generate = tk.Button(btn_frame, text="Generate", width=10, command=m.__on_generate)
        btn_generate.pack(side=tk.LEFT, padx=5)
        btn_ok = tk.Button(btn_frame, text="OK", width=10, command=m.__on_ok)
        btn_ok.pack(side=tk.LEFT, padx=5)
        btn_cancel = tk.Button(btn_frame, text="Cancel", width=10, command=m.__on_cancel)
        btn_cancel.pack(side=tk.LEFT, padx=5)

    def __init__ (m, parent, max_w, max_h, zoom, color_bg, color_fill, scale=1.0):
        m.parent = parent
        m.max_w, m.max_h = max_w, max_h
        m.result_image = None
        m.base_image = None
        m.angles, m.depths, m.points = [], [], []
        m.line_gen = LineGenerator()
        m.canvas_width, m.canvas_height = max_w * zoom, max_h * zoom
        m.zoom = zoom
        m.min_angle_step = 1.0
        m.max_angle_step = 30.0
        m.scale = scale
        m.min_depth = 0.3*scale
        m.max_depth = 0.9*scale
        m.rotation = 0.0
        m.color_bg = np.array(color_bg, dtype=np.uint8)
        m.color_fill = np.array(color_fill, dtype=np.uint8)
        m.color_white = np.array([255, 255, 255], dtype=np.uint8)
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("New Continent")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.dialog.protocol("WM_DELETE_WINDOW", m.__on_cancel)
        m.dialog.geometry("850x750")
        m.__setup_ui()
        m.dialog.wait_window()

    def get_image (m):
        return m.result_image

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("New Continent")
    root.geometry("800x600")
    m = DialogNewCont(root, 800, 600, 1, (32, 26, 120), (121, 189, 36))
    m.get_image()
    root.mainloop()

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#