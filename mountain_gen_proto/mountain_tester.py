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
from mountain_gen import MountainGenerator
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: MountainTesterGUI
#================================================================================================================================#

class MountainTesterGUI:

    def __init__ (m, root, width=100, height=100, zoom=1.0):
        m.root = root
        m.generator = MountainGenerator (width=width, height=height)
        m.sprite_width = width
        m.sprite_height = height
        m.zoom = zoom
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.generate_outline_btn = tk.Button(m.top_frame, text="Generate Outline", command=m.__generateOutline, width=15)
        m.generate_outline_btn.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(root, width=int(m.sprite_width * m.zoom), height=int(m.sprite_height * m.zoom), bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

    def __generateOutline (m):
        image_matrix, alpha = m.generator.generateOutline(dark_side_color=100, light_side_color=150)
        green_bg_matrix = np.zeros((m.sprite_height, m.sprite_width, 3), dtype=np.uint8)
        green_bg_matrix[:, :, 1] = 80
        
        mountain_rgb = np.stack([image_matrix, image_matrix, image_matrix], axis=2)
        alpha_normalized = alpha[..., np.newaxis].astype(np.float32) / 255.0
        combined_matrix = (mountain_rgb * alpha_normalized + green_bg_matrix * (1.0 - alpha_normalized)).astype(np.uint8)


        m.__displayImage(combined_matrix)

    def __displayImage (m, image_matrix):
        m.canvas.delete("all")
        if len(image_matrix.shape) == 3:
            img = Image.fromarray(image_matrix, mode='RGB')
        else:
            img = Image.fromarray(image_matrix, mode='L')
        img = img.resize((int(m.sprite_width * m.zoom), int(m.sprite_height * m.zoom)), Image.NEAREST)
        photo = ImageTk.PhotoImage(img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo

#================================================================================================================================#
#=> - Class: MultiMountainTesterGUI
#================================================================================================================================#

class MultiMountainTesterGUI:

    def __init__ (m, root, canvas_width=800, canvas_height=600, tile_width=100, tile_height=60, zoom=1.0):
        m.root = root
        m.canvas_width = canvas_width
        m.canvas_height = canvas_height
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.zoom = zoom
        m.generator = MountainGenerator(width=tile_width, height=tile_height)
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.generate_btn = tk.Button(m.top_frame, text="Generate Mountains", command=m.__generateMountains, width=20)
        m.generate_btn.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(root, width=int(canvas_width * zoom), height=int(canvas_height * zoom), bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.coordinates = []

    def setCoordinates (m, coordinates):
        m.coordinates = coordinates

    def __generateMountains (m):
        green_bg_matrix = np.zeros((m.canvas_height, m.canvas_width, 3), dtype=np.uint8)
        green_bg_matrix[:, :, 1] = 80
        
        sorted_coords = sorted(m.coordinates, key=lambda c: c[1])
        
        for x, y in sorted_coords:
            image_matrix, alpha = m.generator.generateOutline(dark_side_color=100, light_side_color=150)
            mountain_rgb = np.stack([image_matrix, image_matrix, image_matrix], axis=2)
            alpha_normalized = alpha[..., np.newaxis].astype(np.float32) / 255.0
            
            y_end = min(y + m.tile_height, m.canvas_height)
            x_end = min(x + m.tile_width, m.canvas_width)
            y_start = max(0, y)
            x_start = max(0, x)
            
            tile_h = y_end - y_start
            tile_w = x_end - x_start
            
            if tile_h <= 0 or tile_w <= 0:
                continue
            
            tile_y_start = max(0, -y)
            tile_x_start = max(0, -x)
            tile_y_end = tile_y_start + tile_h
            tile_x_end = tile_x_start + tile_w
            
            tile_rgb = mountain_rgb[tile_y_start:tile_y_end, tile_x_start:tile_x_end]
            tile_alpha = alpha_normalized[tile_y_start:tile_y_end, tile_x_start:tile_x_end]
            bg_patch = green_bg_matrix[y_start:y_end, x_start:x_end]
            
            blended = (tile_rgb * tile_alpha + bg_patch * (1.0 - tile_alpha)).astype(np.uint8)
            green_bg_matrix[y_start:y_end, x_start:x_end] = blended
        
        m.__displayImage(green_bg_matrix)

    def __displayImage (m, image_matrix):
        m.canvas.delete("all")
        if len(image_matrix.shape) == 3:
            img = Image.fromarray(image_matrix, mode='RGB')
        else:
            img = Image.fromarray(image_matrix, mode='L')
        img = img.resize((int(m.canvas_width * m.zoom), int(m.canvas_height * m.zoom)), Image.NEAREST)
        photo = ImageTk.PhotoImage(img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if 0:
        root = tk.Tk()
        root.title("Mountain Tester")
        gui = MountainTesterGUI(root, width=100, height=60, zoom=1.0)
        root.mainloop()
    if 1:
        root = tk.Tk()
        root.title("Multi Mountain Tester")
        coordinates = []
        coordinates.append((300, 10))
        coordinates.append((350, 30))
        coordinates.append((310, 50))
        coordinates.append((370, 70))
        coordinates.append((430, 90))
        coordinates.append((300, 110))
        coordinates.append((360, 130))
        coordinates.append((420, 150))
        coordinates.append((480, 170))
        gui = MultiMountainTesterGUI(root, canvas_width=1600, canvas_height=800, tile_width=100, tile_height=60, zoom=1.0)
        gui.setCoordinates(coordinates)
        root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
