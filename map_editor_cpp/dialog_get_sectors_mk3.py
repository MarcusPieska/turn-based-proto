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

lib_path = os.path.join(os.path.dirname(__file__), "get_sectors.so")
if not os.path.exists(lib_path):
    raise FileNotFoundError("*** Error: %s not found. Please compile get_sectors.cpp first." % (lib_path))

lib = ctypes.CDLL(lib_path)

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.gen_sect.restype = None
lib.gen_sect.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte),
    size,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_ubyte),
    ctypes.POINTER(ctypes.c_ubyte)
]

#================================================================================================================================#
#=> - Class: DialogGetSectors -
#================================================================================================================================#

class DialogGetSectors:

    def __display_image (m, img):
        if img is None: return
        photo = ImageTk.PhotoImage(img)
        m.canvas.delete("all")
        m.canvas.create_image(img.size[0] // 2, img.size[1] // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo

    def __on_regenerate (m):
        if m.image is None:
            return
        lat_sp = int(m.scale_lattice.get())
        img_array = np.array(m.image)
        img_h, img_w = img_array.shape[:2]
        channels = img_array.shape[2] if img_array.ndim == 3 else 1
        img_flat = img_array.flatten()
        color_ocean_arr = np.array(m.color_ocean, dtype=np.uint8)
        color_mtn_arr = np.array(m.color_mtn, dtype=np.uint8)
        zoom = 2
        zoomed_w, zoomed_h = img_w * zoom, img_h * zoom
        img_sectors_out = np.zeros((zoomed_h, zoomed_w, channels), dtype=np.uint8)
        img_rivers_out = np.zeros((zoomed_h, zoomed_w, channels), dtype=np.uint8)
        sz = size(img_w, img_h)
        img_ptr = img_flat.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
        ocean_ptr = color_ocean_arr.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
        mtn_ptr = color_mtn_arr.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
        sectors_ptr = img_sectors_out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
        rivers_ptr = img_rivers_out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
        lib.gen_sect(img_ptr, sz, channels, ocean_ptr, mtn_ptr, lat_sp, sectors_ptr, rivers_ptr)
        m.img_sectors = Image.fromarray(img_sectors_out, 'RGB')
        m.img_rivers = Image.fromarray(img_rivers_out, 'RGB')
        m.show_river_system_view = False
        m.__display_image(m.img_sectors)

    def __on_toggle (m):
        if m.img_sectors is None or m.img_rivers is None: return
        m.show_river_system_view = not m.show_river_system_view
        m.__display_image(m.img_rivers if m.show_river_system_view else m.img_sectors)

    def __setup_ui (m):
        m.frm_main = tk.Frame(m.dialog, padx=10, pady=10)
        m.frm_main.pack()
        btn_frame = tk.Frame(m.frm_main)
        btn_frame.pack(pady=5)
        m.btn_regenerate = tk.Button(btn_frame, text="Regenerate", width=12, command=m.__on_regenerate)
        m.btn_regenerate.pack(side=tk.LEFT, padx=5)
        m.btn_toggle = tk.Button(btn_frame, text="Toggle", width=12, command=m.__on_toggle)
        m.btn_toggle.pack(side=tk.LEFT, padx=5)
        lbl_lattice = tk.Label(btn_frame, text="Lattice:")
        lbl_lattice.pack(side=tk.LEFT, padx=5)
        m.scale_lattice = tk.Scale(btn_frame, from_=5, to=50, orient=tk.HORIZONTAL, length=150)
        m.scale_lattice.set(20)
        m.scale_lattice.pack(side=tk.LEFT, padx=5)
        m.canvas = tk.Canvas(m.frm_main, width=800, height=600, bg="white")
        m.canvas.pack(pady=5)

    def __init__ (m, parent, img_path, color_ocean, color_mtn, color_grass):
        m.parent = parent
        m.img_path = img_path
        m.color_ocean = color_ocean
        m.color_mtn = color_mtn
        m.color_grass = color_grass
        m.image = Image.open(img_path) if os.path.exists(img_path) else None
        m.img_sectors = None
        m.img_rivers = None
        m.show_river_system_view = False
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Get Sectors")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.__setup_ui()
        if m.image is not None:
            img_w, img_h = m.image.size
            zoom = 2
            new_w, new_h = img_w * zoom, img_h * zoom
            m.canvas.config(width=new_w, height=new_h)
            m.__on_regenerate()
        m.dialog.wait_window()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    color_ocean = (32, 26, 120)
    color_mtn = (100, 50, 25)
    color_grass = (121, 189, 36)
    img_path = "/home/w/Projects/rts-proto-map/first-test/cont001.png"
    root = tk.Tk()
    app = DialogGetSectors(root, img_path, color_ocean, color_mtn, color_grass)
    root.mainloop()

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
