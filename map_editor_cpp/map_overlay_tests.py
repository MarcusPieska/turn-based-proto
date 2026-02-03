#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import ctypes
import numpy as np
from PIL import Image, ImageTk
import tkinter as tk

#================================================================================================================================#
#=> - C types and structures -
#================================================================================================================================#

class Size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

class Pt(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]

#================================================================================================================================#
#=> - Test class -
#================================================================================================================================#

class DepthOverlayTester:

    def __init__(m, image_path, target_color):
        m.lib = ctypes.CDLL("./map_overlays.so")
        
        m.lib.depth_overlay.argtypes = [
            ctypes.POINTER(ctypes.c_uint8),  
            Size, 
            ctypes.c_int, 
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_int)
        ]
        m.lib.depth_overlay.restype = None
        
        m.lib.min_depth_map.argtypes = [
            ctypes.POINTER(ctypes.c_int),
            Size,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_uint8),
            ctypes.POINTER(ctypes.c_int)
        ]
        m.lib.min_depth_map.restype = None
        
        m.original_image = Image.open(image_path)
        m.image_array = np.array(m.original_image)
        m.width = m.image_array.shape[1]
        m.height = m.image_array.shape[0]
        m.channels = m.image_array.shape[2] if len(m.image_array.shape) == 3 else 1
        
        if isinstance(target_color, (list, tuple)):
            m.target_color = np.array(target_color, dtype=np.uint8)
        else:
            m.target_color = np.array([target_color] * m.channels, dtype=np.uint8)
        
        m.state = 0
        m.dist_out = None
        m.current_image = m.original_image
        
        m.root = tk.Tk()
        m.root.title("Depth Overlay Tester - Click to cycle through images")
        
        canvas_frame = tk.Frame(m.root)
        canvas_frame.pack(fill=tk.BOTH, expand=True)
        
        m.canvas = tk.Canvas(canvas_frame, width=min(m.width, 800), height=min(m.height, 600))
        m.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.canvas.bind("<Button-1>", m.on_click)
        
        m.update_display()
        
    def compute_depth_overlay(m):
        img_data = m.image_array.flatten().astype(np.uint8)
        img_ptr = img_data.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
        
        target_ptr = m.target_color.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
        
        dist_size = m.width * m.height
        dist_out = np.zeros(dist_size, dtype=np.int32)
        dist_ptr = dist_out.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
        
        img_size = Size(m.width, m.height)
        
        m.lib.depth_overlay(img_ptr, img_size, m.channels, target_ptr, dist_ptr)
        
        m.dist_out = dist_out.reshape(m.height, m.width)
        return m.dist_out
    
    def compute_min_depth_map(m, min_val):
        if m.dist_out is None:
            m.compute_depth_overlay()
        
        dist_in = m.dist_out.flatten().astype(np.int32)
        dist_ptr = dist_in.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
        
        mask_size = m.width * m.height
        mask_out = np.zeros(mask_size, dtype=np.uint8)
        mask_ptr = mask_out.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8))
        
        count_out = ctypes.c_int(0)
        count_ptr = ctypes.pointer(count_out)
        
        img_size = Size(m.width, m.height)
        
        m.lib.min_depth_map(dist_ptr, img_size, min_val, mask_ptr, count_ptr)
        
        mask = mask_out.reshape(m.height, m.width)
        return mask, count_out.value
    
    def visualize_depth_overlay(m, dist_array):
        max_val = dist_array.max()
        if max_val > 0:
            normalized = (dist_array * 255 / max_val).astype(np.uint8)
        else:
            normalized = dist_array.astype(np.uint8)
        
        if m.channels == 1:
            img = Image.fromarray(normalized, mode='L')
        else:
            img = Image.fromarray(np.stack([normalized] * 3, axis=2), mode='RGB')
        
        return img
    
    def visualize_mask(m, mask):
        if m.channels == 1:
            img = Image.fromarray(mask, mode='L')
        else:
            img = Image.fromarray(np.stack([mask] * 3, axis=2), mode='RGB')
        
        return img
    
    def on_click(m, event):
        if m.state == 0:
            m.state = 1
            dist = m.compute_depth_overlay()
            m.current_image = m.visualize_depth_overlay(dist)
        elif m.state == 1:
            m.state = 2
            mask, count = m.compute_min_depth_map(1)
            if count > 0:
                m.current_image = m.visualize_mask(mask)
            else:
                m.state = 0
                m.current_image = m.original_image
        else:
            mask, count = m.compute_min_depth_map(m.state)
            if count > 0:
                m.current_image = m.visualize_mask(mask)
                m.state += 1
            else:
                m.state = 0
                m.current_image = m.original_image
        
        m.update_display()
    
    def update_display(m):
        m.root.update_idletasks()
        canvas_width = m.canvas.winfo_width()
        canvas_height = m.canvas.winfo_height()
        
        if canvas_width <= 1 or canvas_height <= 1:
            canvas_width = min(m.width, 800)
            canvas_height = min(m.height, 600)
        
        display_image = m.current_image.copy()
        img_width, img_height = display_image.size
        
        scale = min(canvas_width / img_width, canvas_height / img_height, 1.0)
        if scale < 1.0:
            new_width = int(img_width * scale)
            new_height = int(img_height * scale)
            display_image = display_image.resize((new_width, new_height), Image.Resampling.LANCZOS)
        
        photo = ImageTk.PhotoImage(display_image)
        m.canvas.delete("all")
        m.canvas.create_image(canvas_width // 2, canvas_height // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo
    
    def run(m):
        m.root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    image_path = "/home/w/Projects/rts-proto-map/first-test/cont001.png"
    target_color = (100, 50, 25)
    tester = DepthOverlayTester(image_path, target_color)
    tester.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
