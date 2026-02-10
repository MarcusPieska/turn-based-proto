#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: TiledImageViewer
#================================================================================================================================#

class TiledImageViewer:

    def __loadImage (m):
        if os.path.exists(m.image_path):
            img = Image.open(m.image_path)
            img_rgb = img.convert('RGB')
            m.image = img_rgb
            m.image_width, m.image_height = m.image.size
        else:
            m.image = None
            m.image_width, m.image_height = 0, 0

    def __updateDisplay (m):
        if m.image is None:
            return
        m.canvas.delete("all")
        zoomed_width = int(m.image_width * m.zoom_factor)
        zoomed_height = int(m.image_height * m.zoom_factor)
        zoomed_img = m.image.resize((zoomed_width, zoomed_height), Image.NEAREST)
        photo = ImageTk.PhotoImage(zoomed_img)
        m.photo_ref = photo
        tile_rows = 5
        tile_cols = 5
        for row in range(tile_rows):
            for col in range(tile_cols):
                x = col * zoomed_width
                y = row * zoomed_height
                m.canvas.create_image(x, y, anchor=tk.NW, image=photo, tags="tile")
        canvas_width = tile_cols * zoomed_width
        canvas_height = tile_rows * zoomed_height
        m.canvas.config(scrollregion=(0, 0, canvas_width, canvas_height))

    def __zoomIn (m):
        m.zoom_factor *= 1.2
        m.__updateDisplay()

    def __zoomOut (m):
        m.zoom_factor /= 1.2
        m.__updateDisplay()

    def __onKeyPress (m, event):
        if event.keysym == 'plus' or event.keysym == 'equal':
            m.__zoomIn()
        elif event.keysym == 'minus' or event.keysym == 'underscore':
            m.__zoomOut()

    def __init__ (m, root, image_path):
        m.root = root
        m.image_path = image_path
        m.zoom_factor = 1.0
        m.photo_ref = None
        m.image = None
        m.image_width = 0
        m.image_height = 0
        m.__loadImage()
        if m.image is None:
            return
        canvas_multiplier = 5
        canvas_base_width = m.image_width * canvas_multiplier
        canvas_base_height = m.image_height * canvas_multiplier
        m.main_frame = tk.Frame(root)
        m.main_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.control_frame = tk.Frame(m.main_frame)
        m.control_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.zoom_in_btn = tk.Button(m.control_frame, text="Zoom In (+)", command=m.__zoomIn, width=15)
        m.zoom_in_btn.pack(side=tk.LEFT, padx=5)
        m.zoom_out_btn = tk.Button(m.control_frame, text="Zoom Out (-)", command=m.__zoomOut, width=15)
        m.zoom_out_btn.pack(side=tk.LEFT, padx=5)
        m.canvas_frame = tk.Frame(m.main_frame)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        m.canvas = tk.Canvas(m.canvas_frame, width=canvas_base_width, height=canvas_base_height, bg="gray")
        m.v_scrollbar = ttk.Scrollbar(m.canvas_frame, orient=tk.VERTICAL, command=m.canvas.yview)
        m.h_scrollbar = ttk.Scrollbar(m.canvas_frame, orient=tk.HORIZONTAL, command=m.canvas.xview)
        m.canvas.config(yscrollcommand=m.v_scrollbar.set, xscrollcommand=m.h_scrollbar.set)
        m.canvas.grid(row=0, column=0, sticky="nsew")
        m.v_scrollbar.grid(row=0, column=1, sticky="ns")
        m.h_scrollbar.grid(row=1, column=0, sticky="ew")
        m.canvas_frame.grid_rowconfigure(0, weight=1)
        m.canvas_frame.grid_columnconfigure(0, weight=1)
        m.root.bind('<KeyPress>', m.__onKeyPress)
        m.root.focus_set()
        m.__updateDisplay()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ("*** Error: Usage: %s <image_path>" %(sys.argv[0]))
        sys.exit(1)
    image_path = sys.argv[1]
    
    root = tk.Tk()
    root.title("Tiled Image Viewer")
    viewer = TiledImageViewer(root, image_path)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
