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
import ctypes
import glob
import random
import tkinter as tk

#================================================================================================================================#
#=> - C++ Library Setup -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "dat31_io.so")
if not os.path.exists(lib_path):
    lib_path = os.path.join(os.path.dirname(__file__), "..", "tile_gen_proto", "dat31_io.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile dat31_io.cpp first." % (lib_path))

lib = ctypes.CDLL(lib_path)

lib.dat31_start_writer.restype = None
lib.dat31_start_writer.argtypes = [ctypes.c_char_p]

lib.dat31_write_item.restype = None
lib.dat31_write_item.argtypes = [ctypes.c_void_p, ctypes.c_int]

lib.dat31_finish.restype = None
lib.dat31_finish.argtypes = []

lib.dat31_start_reader.restype = None
lib.dat31_start_reader.argtypes = [ctypes.c_char_p]

lib.dat31_get_item.restype = ctypes.c_void_p
lib.dat31_get_item.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_int)]

def dat31_start_writer(path):
    lib.dat31_start_writer(path.encode('utf-8'))

def dat31_write_item(data, size):
    if isinstance(data, np.ndarray):
        data_ptr = data.ctypes.data_as(ctypes.c_void_p)
    else:
        arr = (ctypes.c_uint8 * size).from_buffer_copy(data)
        data_ptr = ctypes.cast(arr, ctypes.c_void_p)
    lib.dat31_write_item(data_ptr, size)

def dat31_finish():
    lib.dat31_finish()

def dat31_start_reader(path):
    lib.dat31_start_reader(path.encode('utf-8'))

def dat31_get_item(index):
    size_out = ctypes.c_int()
    ptr = lib.dat31_get_item(index, ctypes.byref(size_out))
    if ptr and size_out.value > 0:
        data = ctypes.string_at(ptr, size_out.value)
        return data, size_out.value
    return None, 0

#================================================================================================================================#
#=> - Function for building and testing mountain decal set
#================================================================================================================================#

def process_texture_set(tex_set_path, output_path):
    png_pattern = os.path.join(tex_set_path, "*.png")
    png_files = sorted(glob.glob(png_pattern))
    
    if not png_files:
        print("*** Error: No PNG files found in %s" % tex_set_path)
        exit(1)
    print("*** Found %d PNG files" % len(png_files))
    random.shuffle(png_files)
    dat31_start_writer(output_path)
    for idx, png_path in enumerate(png_files):
        img = Image.open(png_path)
        if img.mode != 'RGBA':
            img = img.convert('RGBA')
        img_array = np.array(img, dtype=np.uint8)
        height, width = img_array.shape[:2]
        
        rgb_array = img_array[:, :, :3].copy()
        alpha = img_array[:, :, 3]
        transparent_mask = alpha < 128
        rgb_array[transparent_mask, 0] = 255
        rgb_array[transparent_mask, 1] = 0
        rgb_array[transparent_mask, 2] = 255
        
        expected_nbytes = width * height * 3
        dat31_write_item(rgb_array, expected_nbytes)
        print("*** Image shape: %s -> RGB shape: %s" % (str(img_array.shape), str(rgb_array.shape)))
        print("*** Image size: %d bytes" % (rgb_array.size * rgb_array.itemsize))
        print("*** Calculated expected size: %d bytes" % expected_nbytes)
    dat31_finish()
    print("*** Saved texture set to: %s" % output_path)

def test_dat31_viewer(dat31_path):
    dat31_start_reader(dat31_path)
    
    root = tk.Tk()
    root.title("dat31 Viewer - Click to advance (0/10)")
    
    current_idx = [0]
    click_count = [0]
    max_clicks = 20
    
    def load_and_display_image():
        if click_count[0] >= max_clicks:
            print("*** Reached %d clicks, closing..." % max_clicks)
            root.quit()
            root.destroy()
            return
        
        index = current_idx[0]
        size_out = ctypes.c_int()
        ptr = lib.dat31_get_item(index, ctypes.byref(size_out))
        
        if not ptr or size_out.value == 0:
            print("*** Error: Failed to read item %d" % current_idx[0])
            current_idx[0] += 1
            load_and_display_image()
            return
        
        print("*** C function returned: ptr=%s, size_out=%d" % (hex(ptr), size_out.value))
        
        img_data = ctypes.string_at(ptr, size_out.value)
        actual_data_size = len(img_data)
        print("*** Read data size: %d bytes" % actual_data_size)
        
        num_pixels = actual_data_size // 3
        width = int(np.sqrt(num_pixels * 1.0))
        height = num_pixels // width
        while width * height * 3 < actual_data_size:
            width += 1
        while width * height * 3 > actual_data_size:
            width -= 1
        print("*** Calculated dimensions: %d x %d" % (width, height))
        
        img_array = np.frombuffer(img_data, dtype=np.uint8)
        if len(img_array) >= width * height * 3:
            img_array = img_array[:width * height * 3].reshape((height, width, 3))
        else:
            print("*** Warning: Data too small for dimensions, using available data")
            actual_pixels = len(img_array) // 3
            height = int(np.sqrt(actual_pixels))
            width = actual_pixels // height
            img_array = img_array[:height * width * 3].reshape((height, width, 3))
        
        pil_img = Image.fromarray(img_array, mode='RGB')
        
        max_display_w, max_display_h = 800, 600
        if pil_img.width > max_display_w or pil_img.height > max_display_h:
            ratio = min(max_display_w / pil_img.width, max_display_h / pil_img.height)
            new_w = int(pil_img.width * ratio)
            new_h = int(pil_img.height * ratio)
            pil_img = pil_img.resize((new_w, new_h), Image.Resampling.LANCZOS)
        
        canvas.delete("all")
        photo = ImageTk.PhotoImage(pil_img)
        canvas.update_idletasks()
        canvas_w = canvas.winfo_width() if canvas.winfo_width() > 1 else 800
        canvas_h = canvas.winfo_height() if canvas.winfo_height() > 1 else 600
        canvas.create_image(canvas_w // 2, canvas_h // 2, 
                           anchor=tk.CENTER, image=photo)
        canvas.image = photo
        
        root.title("dat31 Viewer - Click to advance (%d/10)" % (click_count[0] + 1))
        print("*** Displaying image %d (click %d/10) (%d x %d)" % (current_idx[0] + 1, click_count[0] + 1, width, height))
    
    def on_canvas_click(event):
        click_count[0] += 1
        current_idx[0] += 1
        load_and_display_image()
    
    canvas = tk.Canvas(root, width=800, height=600, bg="gray")
    canvas.pack(fill=tk.BOTH, expand=True)
    canvas.bind("<Button-1>", on_canvas_click)
    
    load_and_display_image()
    
    root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    tex_set_path = "/home/w/Projects/img-content/texture-mtn2"
    output_path = "/home/w/Projects/img-content/decal_mtn2.dat31"
    process_texture_set(tex_set_path, output_path)
    test_dat31_viewer(output_path)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
