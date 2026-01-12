#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
from PIL import Image, ImageTk
import tkinter as tk
from tkinter import ttk
import ctypes

#================================================================================================================================#
#=> - Setup C library -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "map_to_map.so")
if not os.path.exists(lib_path):
    print("Error: %s not found. Please compile map_to_map.cpp first." % lib_path)
    sys.exit(1)

lib = ctypes.CDLL(lib_path)

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

class color(ctypes.Structure):
    _fields_ = [("h", ctypes.c_ubyte), ("s", ctypes.c_ubyte), ("b", ctypes.c_ubyte)]

lib.reduce_colors.restype = None
lib.reduce_colors.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte),
    color,
    ctypes.POINTER(ctypes.c_ubyte)
]

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def rgb_to_hsb (r, g, b):
    r_norm = float(r) / 255.0
    g_norm = float(g) / 255.0
    b_norm = float(b) / 255.0
    max_val = max(r_norm, g_norm, b_norm)
    min_val = min(r_norm, g_norm, b_norm)
    delta = max_val - min_val
    brightness = max_val * 255.0
    if delta < 0.0001:
        return (0.0, 0.0, brightness)
    saturation = (delta / max_val) * 255.0 if max_val > 0.0001 else 0.0
    if max_val == r_norm:
        hue = 60.0 * (((g_norm - b_norm) / delta) % 6.0)
    elif max_val == g_norm:
        hue = 60.0 * (2.0 + ((b_norm - r_norm) / delta))
    else:
        hue = 60.0 * (4.0 + ((r_norm - g_norm) / delta))
    if hue < 0.0:
        hue += 360.0
    hue = hue * 255.0 / 360.0
    return (hue, saturation, brightness)

def normalize_color (color_val):
    if isinstance(color_val, (list, tuple)):
        color_val = np.array(color_val, dtype=np.uint8)
    if isinstance(color_val, np.ndarray):
        return color_val[:3] if color_val.size >= 3 else np.array([color_val[0]] * 3, dtype=np.uint8)
    return np.array([color_val] * 3, dtype=np.uint8)

def reduce_colors_numpy (img, target_color, replacement_color, hue_range, brightness_range, saturation_range, pixel_map):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w, ch = *img.shape, 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if ch < 3:
        return img
    img_contiguous = np.ascontiguousarray(img, dtype=np.uint8)
    img_needs_copy_back = img_contiguous is not img
    if pixel_map.shape != (h, w):
        raise ValueError(f"pixel_map shape {pixel_map.shape} does not match image shape ({h}, {w})")
    pixel_map_contiguous = np.ascontiguousarray(pixel_map, dtype=np.uint8)
    pixel_map_needs_copy_back = pixel_map_contiguous is not pixel_map
    target_rgb = np.ascontiguousarray(normalize_color(target_color), dtype=np.uint8)
    replacement_rgb = np.ascontiguousarray(normalize_color(replacement_color), dtype=np.uint8)
    img_ptr = img_contiguous.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    sz = size(w, h)
    old_color_ptr = target_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    new_color_ptr = replacement_rgb.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    range_struct = color(int(hue_range), int(saturation_range), int(brightness_range))
    pixel_map_ptr = pixel_map_contiguous.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.reduce_colors(img_ptr, sz, ch, old_color_ptr, new_color_ptr, range_struct, pixel_map_ptr)
    if img_needs_copy_back:
        img[:] = img_contiguous[:]
    if pixel_map_needs_copy_back:
        pixel_map[:] = pixel_map_contiguous[:]
    return img[:, :, 0] if ch == 1 else img

#================================================================================================================================#
#=> - MapToMap class -
#================================================================================================================================#

class MapToMap:
    
    def __load_image (m):
        m.img_pil = Image.open(m.image_path)
        m.img_array = np.array(m.img_pil)
        if m.img_array.ndim == 2:
            h, w = m.img_array.shape
            m.img_array = m.img_array[:, :, np.newaxis]
        h, w, ch = m.img_array.shape
        if ch == 1:
            m.img_array = np.repeat(m.img_array, 3, axis=2)
        elif ch == 4:
            m.img_array = m.img_array[:, :, :3]
        m.img_pil_copy = m.img_pil.copy()
        m.img_array_copy = m.img_array.copy()
        m.used_pixels = np.zeros((h, w), dtype=np.uint8)
    
    def __setup_gui (m):
        m.root = tk.Tk()
        m.root.title("MapToMap")
        m.__create_top_frame()
        m.__create_hue_frame()
        m.__create_bottom_frame()
        m.__display_image()
    
    def __create_top_frame (m):
        top_frame = tk.Frame(m.root)
        top_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=10)
        m.color_boxes = []
        for i, color in enumerate(m.colors):
            color_box = tk.Frame(top_frame, width=100, height=100, relief=tk.RAISED, borderwidth=2)
            color_box.pack(side=tk.LEFT, padx=5)
            color_box.pack_propagate(False)
            if isinstance(color, (list, tuple, np.ndarray)):
                rgb = tuple(int(c) for c in (color[:3] if len(color) >= 3 else [color[0] if len(color) > 0 else color] * 3))
            else:
                rgb = (int(color), int(color), int(color))
            color_label = tk.Label(color_box, bg="#%02x%02x%02x" % rgb, width=100, height=100, cursor="hand2")
            color_label.pack(fill=tk.BOTH, expand=True)
            color_label.bind("<Button-1>", lambda e, idx=i, col=rgb: m.__on_color_box_click(idx, col))
            m.color_boxes.append((color_box, color_label, rgb))
        spacer = tk.Frame(top_frame, width=20)
        spacer.pack(side=tk.LEFT)
        m.clicked_color_box = tk.Frame(top_frame, width=100, height=100, relief=tk.RAISED, borderwidth=2)
        m.clicked_color_box.pack(side=tk.RIGHT, padx=5)
        m.clicked_color_box.pack_propagate(False)
        m.clicked_color_label = tk.Label(m.clicked_color_box, bg="#000000", width=100, height=100)
        m.clicked_color_label.pack(fill=tk.BOTH, expand=True)
        m.clicked_color_rgb = None
    
    def __create_slider (m, parent, label_txt, start, end, init_val, length, cb, value_label_attr, slider_attr, off=False):
        tk.Label(parent, text=label_txt).pack(side=tk.TOP, padx=5)
        s = ttk.Scale(parent, from_=start, to=end, orient=tk.HORIZONTAL, length=length, state=tk.DISABLED if off else tk.NORMAL)
        s.set(init_val)
        s.pack(side=tk.TOP, padx=5)
        if not off:
            s.bind("<ButtonRelease-1>", cb)
            s.bind("<B1-Motion>", cb)
        setattr(m, slider_attr, s)
        setattr(m, value_label_attr, tk.Label(parent, text=str(init_val)))
        getattr(m, value_label_attr).pack(side=tk.TOP, padx=5)
    
    def __create_hue_frame (m):
        hue_frame = tk.Frame(m.root)
        hue_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=10)
        frame1 = tk.Frame(hue_frame)
        frame1.pack(side=tk.LEFT, padx=5)
        m.__create_slider(frame1, "Hue:", 0, 40, m.hue, 400, m.__on_hue_change, "hue_value_label", "hue_slider")
        m.__create_slider(frame1, "Brightness:", 0, 40, m.brightness, 400, m.__on_brightness_change, "brightness_value_label", "brightness_slider")
        m.__create_slider(frame1, "Saturation:", 0, 40, m.saturation, 400, m.__on_saturation_change, "saturation_value_label", "saturation_slider")
        frame2 = tk.Frame(hue_frame)
        frame2.pack(side=tk.LEFT, padx=5)
        m.__create_slider(frame2, "Clicked Hue:", 1, 255, 1, 400, None, "clicked_hue_value_label", "clicked_hue_slider", True)
        m.__create_slider(frame2, "Clicked Saturation:", 1, 255, 1, 400, None, "clicked_saturation_value_label", "clicked_saturation_slider", True)
        m.__create_slider(frame2, "Clicked Brightness:", 1, 255, 1, 400, None, "clicked_brightness_value_label", "clicked_brightness_slider", True)
        frame3 = tk.Frame(hue_frame)
        frame3.pack(side=tk.LEFT, padx=10)
        m.undo_button = tk.Button(frame3, text="Undo", command=m.__on_undo, state=tk.DISABLED)
        m.undo_button.pack(side=tk.TOP, padx=5)
        m.save_button = tk.Button(frame3, text="Save", command=m.__on_save)
        m.save_button.pack(side=tk.TOP, padx=5)
    
    def __on_hue_change (m, event=None):
        m.hue = m.hue_slider.get()
        m.hue_value_label.config(text=f"{int(m.hue)}")
        m.__on_hue_update(m.hue)
    
    def __on_brightness_change (m, event=None):
        m.brightness = m.brightness_slider.get()
        m.brightness_value_label.config(text=f"{int(m.brightness)}")
        m.__on_brightness_update(m.brightness)
    
    def __on_saturation_change (m, event=None):
        m.saturation = m.saturation_slider.get()
        m.saturation_value_label.config(text=f"{int(m.saturation)}")
        m.__on_saturation_update(m.saturation)
    
    def __on_hue_update (m, hue):
        pass
    
    def __on_brightness_update (m, brightness):
        pass
    
    def __on_saturation_update (m, saturation):
        pass
    
    def __create_bottom_frame (m):
        bottom_frame = tk.Frame(m.root)
        bottom_frame.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True, padx=10, pady=10)
        canvas_frame = tk.Frame(bottom_frame)
        canvas_frame.pack(fill=tk.BOTH, expand=True)
        img_w, img_h = m.img_pil.size
        max_w, max_h = 1600, 800
        canvas_w = min(img_w, max_w)
        canvas_h = min(img_h, max_h)
        m.canvas = tk.Canvas(canvas_frame, bg="gray", width=canvas_w, height=canvas_h)
        h_scrollbar = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=m.canvas.xview)
        v_scrollbar = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=m.canvas.yview)
        m.canvas.configure(xscrollcommand=h_scrollbar.set, yscrollcommand=v_scrollbar.set)
        if img_w > max_w:
            h_scrollbar.pack(side=tk.BOTTOM, fill=tk.X)
        if img_h > max_h:
            v_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        m.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.canvas.bind("<Button-1>", m.__on_canvas_click)
        m.canvas.bind("<Button-3>", m.__on_canvas_right_click)
    
    def __display_image (m):
        if m.display_mode == 2:
            h, w = m.used_pixels.shape
            pixel_map_img = np.zeros((h, w, 3), dtype=np.uint8)
            pixel_map_img[:, :, :] = m.used_pixels[:, :, np.newaxis] * 255
            current_img = Image.fromarray(pixel_map_img)
        else:
            current_img = m.img_pil_copy if m.display_mode == 1 else m.img_pil
        img_w, img_h = current_img.size
        m.img_tk = ImageTk.PhotoImage(current_img)
        if m.canvas_image_id is not None:
            m.canvas.delete(m.canvas_image_id)
        m.canvas_image_id = m.canvas.create_image(0, 0, anchor=tk.NW, image=m.img_tk)
        m.canvas.configure(scrollregion=(0, 0, img_w, img_h))
    
    def __on_canvas_right_click (m, event):
        m.display_mode = (m.display_mode + 1) % 3
        m.__display_image()
    
    def __on_canvas_click (m, event):
        canvas_x = int(m.canvas.canvasx(event.x))
        canvas_y = int(m.canvas.canvasy(event.y))
        img_w, img_h = m.img_pil.size
        if canvas_x < 0 or canvas_y < 0 or canvas_x >= img_w or canvas_y >= img_h:
            return
        pixel_x = canvas_x
        pixel_y = canvas_y
        if m.used_pixels[pixel_y, pixel_x] != 0:
            print(f"Pixel ({pixel_x}, {pixel_y}) is already used and cannot be selected")
            return
        if m.display_mode == 2:
            return
        current_array = m.img_array_copy if m.display_mode == 1 else m.img_array
        color = current_array[pixel_y, pixel_x, :] if current_array.ndim == 3 else current_array[pixel_y, pixel_x]
        rgb = tuple(int(c) for c in (color[:3] if isinstance(color, np.ndarray) else [color] * 3))
        m.__update_clicked_color(rgb)
        print(f"Coordinates: ({pixel_x}, {pixel_y})")
        m.__on_pixel_click(pixel_x, pixel_y, rgb)
    
    def __update_clicked_color (m, rgb):
        m.clicked_color_label.config(bg="#%02x%02x%02x" % rgb)
        m.clicked_color_rgb = rgb
        h, s, b = rgb_to_hsb(rgb[0], rgb[1], rgb[2])
        h_val = max(1, min(255, int(h)))
        s_val = max(1, min(255, int(s)))
        b_val = max(1, min(255, int(b)))
        m.clicked_hue_slider.config(state=tk.NORMAL)
        m.clicked_hue_slider.set(h_val)
        m.clicked_hue_slider.config(state=tk.DISABLED)
        m.clicked_saturation_slider.config(state=tk.NORMAL)
        m.clicked_saturation_slider.set(s_val)
        m.clicked_saturation_slider.config(state=tk.DISABLED)
        m.clicked_brightness_slider.config(state=tk.NORMAL)
        m.clicked_brightness_slider.set(b_val)
        m.clicked_brightness_slider.config(state=tk.DISABLED)
        m.clicked_hue_value_label.config(text=f"{int(h)}")
        m.clicked_saturation_value_label.config(text=f"{int(s)}")
        m.clicked_brightness_value_label.config(text=f"{int(b)}")
        m.root.update_idletasks()
    
    def __on_color_box_click (m, box_idx, replacement_rgb):
        if m.clicked_color_rgb is None or m.display_mode == 2:
            return
        current_array = m.img_array_copy if m.display_mode == 1 else m.img_array
        h, w = current_array.shape[:2]
        pixel_map = np.zeros((h, w), dtype=np.uint8)
        saved_state = current_array.copy()
        saved_used_pixels = m.used_pixels.copy()
        reduce_colors_numpy(current_array, m.clicked_color_rgb, replacement_rgb, int(m.hue), int(m.brightness), int(m.saturation), pixel_map)
        m.used_pixels = np.maximum(m.used_pixels, pixel_map)
        if m.display_mode == 1:
            m.img_pil_copy = Image.fromarray(current_array)
        else:
            m.img_pil = Image.fromarray(current_array)
        m.__push_undo_state(saved_state, saved_used_pixels)
        m.__display_image()
    
    def __push_undo_state (m, img_array, used_pixels_map):
        if len(m.undo_queue) >= 10:
            m.undo_queue.pop(0)
        m.undo_queue.append((m.display_mode, img_array, used_pixels_map))
        m.undo_button.config(state=tk.NORMAL)
    
    def __on_undo (m):
        if len(m.undo_queue) == 0:
            return
        was_display_mode, saved_array, saved_used_pixels = m.undo_queue.pop()
        if was_display_mode == 0:
            m.img_array, m.img_pil = saved_array, Image.fromarray(saved_array)
        else:
            m.img_array_copy, m.img_pil_copy = saved_array, Image.fromarray(saved_array)
        m.used_pixels = saved_used_pixels
        m.undo_button.config(state=tk.DISABLED if len(m.undo_queue) == 0 else tk.NORMAL)
        m.__display_image()
    
    def __on_save (m):
        reduced_img_path = m.image_path.replace (".png", "_reduced.png")
        m.img_pil.save (reduced_img_path)
        print ("** Saved reduced image to: %s" % reduced_img_path)
    
    def __on_pixel_click (m, x, y, color):
        pass
    
    def __init__ (m, image_path, colors):
        if not isinstance(colors, list) or len(colors) < 6 or len(colors) > 12:
            raise ValueError("colors must be a list of 6-12 color values")
        m.image_path = image_path
        m.colors = colors
        m.hue = 0.0
        m.brightness = 0.0
        m.saturation = 0.0
        m.display_mode = 0
        m.clicked_color_rgb = None
        m.undo_queue = []
        m.img_array = None
        m.img_pil = None
        m.img_array_copy = None
        m.img_pil_copy = None
        m.used_pixels = None
        m.img_tk = None
        m.canvas_image_id = None
        m.__load_image()
        m.__setup_gui()
    
    def set_click_callback (m, callback):
        m.__on_pixel_click = callback
    
    def set_hue_callback (m, callback):
        m.__on_hue_update = callback
    
    def run (m):
        m.root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    colors = []
    colors.append([121, 189, 36])
    colors.append([222, 199, 89])
    colors.append([244, 203, 141])
    colors.append([248, 251, 252])
    colors.append([82, 63, 14])
    colors.append([32, 26, 120])
    colors.append([65, 156, 244])
    map_to_map = MapToMap ("../../../DEL_world_map3.png", colors)
    map_to_map.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
