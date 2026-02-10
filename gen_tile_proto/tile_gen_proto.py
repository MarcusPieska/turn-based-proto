#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import math
import random
import tkinter as tk
from PIL import Image, ImageTk, ImageFilter
import cv2

#================================================================================================================================#
#=> - Globals
#================================================================================================================================#

SIZE_BOOST = 0 # Boost the size of the tile by this factor to handle jagged borders and the gaps between tiles

#================================================================================================================================#
#=> - Class: TileEditor
#================================================================================================================================#

class TileEditor:

    def __init__ (m, root, tile_width, tile_height, initial_tile=None, zoom_factor=4.0):
        m.root = root
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.zoom_factor = 4.0
        if initial_tile is not None:
            if len(initial_tile) == 2:
                m.curr_tile, m.curr_alpha = initial_tile
            else:
                m.curr_tile = initial_tile
                m.curr_alpha = np.ones((tile_height, tile_width), dtype=np.uint8) * 255
        else:
            m.curr_tile = np.zeros((tile_height, tile_width, 3), dtype=np.uint8)
            m.curr_alpha = np.zeros((tile_height, tile_width), dtype=np.uint8)
        m.show_grid = True
        m.current_color = (100, 100, 100)
        m.canv_w, m.canv_h = 1200, 800

        max_cols = int(m.canv_w/m.tile_width) - 1
        max_rows = int(m.canv_h/(m.tile_height//2)) - 2
        if max_rows % 2 == 0:
            max_rows -= 1
        total_width = max_cols * m.tile_width
        total_height = (max_rows + 1) * (m.tile_height//2)

        m.min_x = (m.canv_w - total_width) // 2
        m.min_y = (m.canv_h - total_height) // 2
        m.num_rows = max_rows
        m.num_cols = max_cols
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.tile_btn = tk.Button(m.top_frame, text="Tile", command=m.__tileCanvas, width=15)
        m.tile_btn.pack(side=tk.LEFT, padx=5)
        
        m.grid_btn = tk.Button(m.top_frame, text="Toggle Grid", command=m.__toggleGrid, width=15)
        m.grid_btn.pack(side=tk.LEFT, padx=5)
        
        m.color_btn = tk.Button(m.top_frame, text="Select Color", command=m.__selectColor, width=15)
        m.color_btn.pack(side=tk.LEFT, padx=5)
        
        m.canv_frame = tk.Frame(root)
        m.canv_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        canvas_width = (tile_width + SIZE_BOOST) * zoom_factor
        m.canv_editor = tk.Canvas(m.canv_frame, width=canvas_width, height=canvas_width, bg="gray")
        m.canv_editor.pack(side=tk.LEFT, padx=5)
        
        m.canv_tiled = tk.Canvas(m.canv_frame, width=m.canv_w, height=m.canv_h, bg="gray")
        m.canv_tiled.pack(side=tk.LEFT, padx=5)
        
        m.__displayEditorTile()
        m.__tileCanvas()

    def __toggleGrid (m):
        m.show_grid = not m.show_grid
        m.__displayEditorTile()
        m.__tileCanvas()

    def __selectColor (m):
        dialog = ColorPickerDialog(m.root, m.current_color)
        m.root.wait_window(dialog.dialog)
        if dialog.selected_color is not None:
            m.current_color = dialog.selected_color

    def __drawGrid (m, canvas, width, height, tile_w, tile_h, zoom=1.0, staggered=False):
        if not m.show_grid:
            return
        half_w = int((tile_w // 2) * zoom)
        half_h = int((tile_h // 2) * zoom)
        if staggered:
            half_width = tile_w // 2
            half_height = tile_h // 2
            for ty in range(m.num_rows):
                if ty % 2 == 1:
                    continue
                y_center = m.min_y + ty * half_height + half_height
                for tx in range(m.num_cols):
                    x_center = m.min_x + tx * tile_w + half_width
                    top = (x_center, y_center - half_h)
                    right = (x_center + half_w, y_center)
                    bottom = (x_center, y_center + half_h)
                    left = (x_center - half_w, y_center)
                    if 0 <= top[1] < height or 0 <= bottom[1] < height:
                        canvas.create_line(top[0], top[1], right[0], right[1], fill="light gray", width=1, tags="grid")
                        canvas.create_line(right[0], right[1], bottom[0], bottom[1], fill="light gray", width=1, tags="grid")
                        canvas.create_line(bottom[0], bottom[1], left[0], left[1], fill="light gray", width=1, tags="grid")
                        canvas.create_line(left[0], left[1], top[0], top[1], fill="light gray", width=1, tags="grid")
        else:
            center_x = width // 2
            center_y = height // 2
            top = (center_x, center_y - half_h)
            right = (center_x + half_w, center_y)
            bottom = (center_x, center_y + half_h)
            left = (center_x - half_w, center_y)
            canvas.create_line(top[0], top[1], right[0], right[1], fill="light gray", width=1, tags="grid")
            canvas.create_line(right[0], right[1], bottom[0], bottom[1], fill="light gray", width=1, tags="grid")
            canvas.create_line(bottom[0], bottom[1], left[0], left[1], fill="light gray", width=1, tags="grid")
            canvas.create_line(left[0], left[1], top[0], top[1], fill="light gray", width=1, tags="grid")

    def __displayEditorTile (m):
        m.canv_editor.delete("all")
        if m.curr_tile is not None:
            zoomed_width = int(m.tile_width * m.zoom_factor)
            zoomed_height = int(m.tile_height * m.zoom_factor)
            img_rgb = Image.fromarray(m.curr_tile, mode='RGB')
            img_alpha = Image.fromarray(m.curr_alpha, mode='L')
            img_rgb = img_rgb.resize((zoomed_width, zoomed_height), Image.NEAREST)
            img_alpha = img_alpha.resize((zoomed_width, zoomed_height), Image.NEAREST)
            img_rgba = Image.merge('RGBA', (img_rgb.split() + (img_alpha,)))
            photo = ImageTk.PhotoImage(img_rgba)
            m.canv_editor.create_image(200, 200, anchor=tk.CENTER, image=photo, tags="tile")
            m.canv_editor.image = photo
        m.__drawGrid(m.canv_editor, 400, 400, m.tile_width, m.tile_height, m.zoom_factor, staggered=False)
        m.canv_editor.tag_raise("grid")

    def __tileCanvas (m):
        m.canv_tiled.delete("all")
        if m.curr_tile is not None:
            half_width = m.tile_width // 2
            half_height = m.tile_height // 2
            tiled_img = np.ones((m.canv_h, m.canv_w, 3), dtype=np.uint8) * 255
            for ty in range(m.num_rows):
                if ty % 2 == 0:
                    row_offset, num_cols = 0, m.num_cols
                else:
                    row_offset, num_cols = half_width, m.num_cols - 1
                y_start = m.min_y + ty * half_height
                y_end = y_start + m.tile_height
                for tx in range(num_cols):
                    x_start = m.min_x + tx * m.tile_width + row_offset
                    x_end = x_start + m.tile_width + SIZE_BOOST
                    if 0 <= x_start < m.canv_w and 0 <= y_start < m.canv_h and x_end <= m.canv_w and y_end <= m.canv_h:
                        tile_slice_rgb = m.curr_tile
                        tile_slice_alpha = m.curr_alpha
                        alpha_norm = tile_slice_alpha[..., np.newaxis].astype(np.float32) / 255.0
                        bg_slice = tiled_img[y_start:y_end, x_start:x_end]
                        blended = (tile_slice_rgb * alpha_norm + bg_slice * (1.0 - alpha_norm)).astype(np.uint8)
                        tiled_img[y_start:y_end, x_start:x_end] = blended
            img = Image.fromarray(tiled_img, mode='RGB')
            photo = ImageTk.PhotoImage(img)
            m.canv_tiled.create_image(0, 0, anchor=tk.NW, image=photo, tags="tile")
            m.canv_tiled.image = photo
        m.__drawGrid(m.canv_tiled, 1200, 800, m.tile_width, m.tile_height, 1.0, staggered=True)
        m.canv_tiled.tag_raise("grid")

    def setTile (m, tile, alpha=None):
        if isinstance(tile, tuple) and len(tile) == 2:
            m.curr_tile, m.curr_alpha = tile
        else:
            m.curr_tile = tile
            m.curr_alpha = alpha if alpha is not None else np.ones((m.tile_height, m.tile_width), dtype=np.uint8) * 255
        m.__displayEditorTile()
        m.__tileCanvas()

    def getTile (m):
        return m.curr_tile, m.curr_alpha

#================================================================================================================================#
#=> - Class: ColorPickerDialog
#================================================================================================================================#

class ColorPickerDialog:

    def __init__ (m, parent, initial_color=(100, 100, 100)):
        m.selected_color = None
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Select Color")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        
        m.r_var = tk.IntVar(value=initial_color[0])
        m.g_var = tk.IntVar(value=initial_color[1])
        m.b_var = tk.IntVar(value=initial_color[2])
        
        m.r_label = tk.Label(m.dialog, text="Red:")
        m.r_label.grid(row=0, column=0, padx=5, pady=5)
        m.r_scale = tk.Scale(m.dialog, from_=0, to=255, orient=tk.HORIZONTAL, variable=m.r_var, command=m.__updatePreview, length=300)
        m.r_scale.grid(row=0, column=1, padx=5, pady=5)
        
        m.g_label = tk.Label(m.dialog, text="Green:")
        m.g_label.grid(row=1, column=0, padx=5, pady=5)
        m.g_scale = tk.Scale(m.dialog, from_=0, to=255, orient=tk.HORIZONTAL, variable=m.g_var, command=m.__updatePreview, length=300)
        m.g_scale.grid(row=1, column=1, padx=5, pady=5)
        
        m.b_label = tk.Label(m.dialog, text="Blue:")
        m.b_label.grid(row=2, column=0, padx=5, pady=5)
        m.b_scale = tk.Scale(m.dialog, from_=0, to=255, orient=tk.HORIZONTAL, variable=m.b_var, command=m.__updatePreview, length=300)
        m.b_scale.grid(row=2, column=1, padx=5, pady=5)
        
        m.preview_label = tk.Label(m.dialog, text="Preview", width=20, height=5)
        m.preview_label.grid(row=3, column=0, columnspan=2, padx=5, pady=5)
        
        m.button_frame = tk.Frame(m.dialog)
        m.button_frame.grid(row=4, column=0, columnspan=2, padx=5, pady=5)
        
        m.ok_btn = tk.Button(m.button_frame, text="OK", command=m.__ok, width=10)
        m.ok_btn.pack(side=tk.LEFT, padx=5)
        
        m.cancel_btn = tk.Button(m.button_frame, text="Cancel", command=m.__cancel, width=10)
        m.cancel_btn.pack(side=tk.LEFT, padx=5)
        
        m.__updatePreview()

    def __updatePreview (m, *args):
        r = m.r_var.get()
        g = m.g_var.get()
        b = m.b_var.get()
        color_hex = "#%02x%02x%02x" % (r, g, b)
        m.preview_label.config(bg=color_hex)

    def __ok (m):
        m.selected_color = (m.r_var.get(), m.g_var.get(), m.b_var.get())
        m.dialog.destroy()

    def __cancel (m):
        m.selected_color = None
        m.dialog.destroy()

#================================================================================================================================#
#=> - Helper Functions -
#================================================================================================================================#

def createDiamondTile (width, height, color=(100, 100, 100)):
    tile = np.zeros((height, width, 3), dtype=np.uint8)
    alpha = np.zeros((height, width), dtype=np.uint8)
    center_x = width // 2
    center_y = height // 2
    for y in range(height):
        for x in range(width):
            dx = abs(x - center_x)
            dy = abs(y - center_y)
            if dx * height + dy * width < width * height // 2:
                tile[y, x] = color
                alpha[y, x] = 255
    
    if SIZE_BOOST > 0:
        middle_col_idx = width // 2
        middle_col_tile = np.squeeze(tile[:, middle_col_idx:middle_col_idx+1, :], axis=1)
        middle_col_alpha = np.squeeze(alpha[:, middle_col_idx:middle_col_idx+1], axis=1)
        for i in range(SIZE_BOOST):
            tile = np.insert(tile, middle_col_idx + i, middle_col_tile, axis=1)
            alpha = np.insert(alpha, middle_col_idx + i, middle_col_alpha, axis=1)
    
    return tile, alpha

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Tile Editor")
    tile_width = 100
    tile_height = 50
    diamond_tile = createDiamondTile (tile_width, tile_height, color=(100, 100, 100))
    editor = TileEditor (root, tile_width, tile_height, initial_tile=diamond_tile)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
