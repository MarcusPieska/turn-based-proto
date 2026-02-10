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

from triangle_morph import TriangleMorpher

#================================================================================================================================#
#=> - Class: MountainTesterGUI
#================================================================================================================================#

class MountainTesterGUI:

    def __brightenImage (m, image_matrix, amount):
        image_matrix = image_matrix.astype(np.float32)
        image_matrix = image_matrix + amount
        image_matrix = np.clip(image_matrix, 0, 255)
        image_matrix = image_matrix.astype(np.uint8)
        return image_matrix

    def __init__ (m, root, width=100, height=100, zoom=1.0, texture=None):
        m.root = root
        m.generator = MountainGenerator (width=width, height=height)
        m.morpher = TriangleMorpher ()
        m.sprite_width = width
        m.sprite_height = height
        m.zoom = zoom
        m.texture = np.array(Image.open(texture))
        
        m.top_frame = tk.Frame (root)
        m.top_frame.pack (side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.generate_outline_btn = tk.Button (m.top_frame, text="Generate Outline", command=m.__generateOutline, width=15)
        m.generate_outline_btn.pack (side=tk.LEFT, padx=5)
        
        width_c = int(m.sprite_width * m.zoom)
        height_c = int(m.sprite_height * m.zoom)
        m_canvas_frame = tk.Frame (root)
        m_canvas_frame.pack (side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas = tk.Canvas (m_canvas_frame, width=width_c, height=height_c, bg="white")
        m.canvas.pack (side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas_morph1 = tk.Canvas (m_canvas_frame, width=width_c, height=height_c, bg="white")
        m.canvas_morph1.pack (side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas_morph2 = tk.Canvas (m_canvas_frame, width=width_c, height=height_c, bg="white")
        m.canvas_morph2.pack (side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)

    def __generateOutline (m):
        image_matrix, alpha, pts = m.generator.generateOutline (dark_side_color=100, light_side_color=150)
        temp = image_matrix.copy()
        tx1, tx2 = None, None
        if m.texture is not None:
            v1 = m.texture.shape[1] // 2, 0
            v2 = 0, m.texture.shape[0]
            v3 = m.texture.shape[1], m.texture.shape[0] 
            tx1 = m.morpher.morphTriangle (m.texture, (v1, v2, v3), pts[0], pts[1], pts[2], m.sprite_width, m.sprite_height)
            tx2 = m.morpher.morphTriangle (m.texture, (v1, v2, v3), pts[0], pts[2], pts[3], m.sprite_width, m.sprite_height)
            tx1 = m.__brightenImage(tx1, -50)
            mask2 = (tx2 == 0)
            tx2 = m.__brightenImage(tx2, 50)
            tx2[mask2] = 0
            image_matrix = tx1 + tx2
            temp_rgb = np.stack([temp, temp, temp], axis=2)
            image_matrix[np.all(image_matrix == 0, axis=2)] = temp_rgb[np.all(image_matrix == 0, axis=2)]

            m.canvas_morph1.delete("all")
            m.canvas_morph2.delete("all")
            photo1 = ImageTk.PhotoImage(Image.fromarray(tx1))
            photo2 = ImageTk.PhotoImage(Image.fromarray(tx2))
            m.canvas_morph1.create_image(0, 0, anchor=tk.NW, image=photo1)
            m.canvas_morph2.create_image(0, 0, anchor=tk.NW, image=photo2)
            m.canvas_morph1.image = photo1
            m.canvas_morph2.image = photo2
        
        green_bg_matrix = np.zeros((m.sprite_height, m.sprite_width, 3), dtype=np.uint8)
        green_bg_matrix[:, :, 1] = 80
        alpha_normalized = alpha[..., np.newaxis].astype(np.float32) / 255.0
        comb_matrix = (image_matrix * alpha_normalized + green_bg_matrix * (1.0 - alpha_normalized)).astype(np.uint8)
        m.__displayImage (comb_matrix)

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

    def __brightenImage (m, image_matrix, amount):
        image_matrix = image_matrix.astype(np.float32)
        image_matrix = image_matrix + amount
        image_matrix = np.clip(image_matrix, 0, 255)
        image_matrix = image_matrix.astype(np.uint8)
        return image_matrix

    def __generateMountains (m):
        green_bg_matrix = np.zeros((m.canvas_height, m.canvas_width, 3), dtype=np.uint8)
        green_bg_matrix[:, :, 1] = 80
        tx1, tx2 = None, None
        for x, y in sorted(m.coordinates, key=lambda c: c[1]):
            image_matrix, alpha, pts = m.generator.generateOutline(dark_side_color=100, light_side_color=150)
            temp = image_matrix.copy()
            if m.texture is not None:
                v1 = m.texture.shape[1] // 2, 0
                v2 = 0, m.texture.shape[0]
                v3 = m.texture.shape[1], m.texture.shape[0] 
                tx1 = m.morpher.morphTriangle (m.texture, (v1, v2, v3), pts[0], pts[1], pts[2], m.tile_width, m.tile_height)
                tx2 = m.morpher.morphTriangle (m.texture, (v1, v2, v3), pts[0], pts[2], pts[3], m.tile_width, m.tile_height)
                tx1 = m.__brightenImage(tx1, -50)
                mask2 = (tx2 == 0)
                tx2 = m.__brightenImage(tx2, 50)
                tx2[mask2] = 0
                image_matrix = tx1 + tx2
                temp_rgb = np.stack([temp, temp, temp], axis=2)
                image_matrix[np.all(image_matrix == 0, axis=2)] = temp_rgb[np.all(image_matrix == 0, axis=2)]
                mountain_rgb = image_matrix
            else:
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

    def __init__ (m, root, canvas_width=800, canvas_height=600, tile_width=100, tile_height=60, zoom=1.0, texture=None):
        m.root = root
        m.canvas_width = canvas_width
        m.canvas_height = canvas_height
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.zoom = zoom
        m.texture = None
        if texture is not None:
            m.texture = np.array (Image.open(texture))
        m.generator = MountainGenerator (width=tile_width, height=tile_height)
        m.morpher = TriangleMorpher ()
        
        m.top_frame = tk.Frame (root)
        m.top_frame.pack (side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.generate_btn = tk.Button (m.top_frame, text="Generate Mountains", command=m.__generateMountains, width=20)
        m.generate_btn.pack (side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas (root, width=int(canvas_width * zoom), height=int(canvas_height * zoom), bg="white")
        m.canvas.pack (side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.coordinates = []

    def setCoordinates (m, coordinates):
        m.coordinates = coordinates

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    tex1 = "../../DEL_temp_mtn_tex_0_139_90_43.png"
    tex2 = "../../DEL_temp_mtn_tex_0_140_110_85.png"
    tex3 = "../../DEL_temp_mtn_tex_0_100_100_100.png"
    tex4 = "../../DEL_temp_mtn_tex_0_101_75_60.png"
    tex5 = "../../DEL_temp_mtn_tex_0_120_90_70.png"

    all_textures = [tex1, tex2, tex3, tex4, tex5]
    
    if 0:
        root = tk.Tk ()
        root.title ("Mountain Tester")
        gui = MountainTesterGUI (root, width=100, height=60, zoom=1.0, texture=tex2)
        root.mainloop ()
    elif 1:
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
        for t in all_textures:
            root = tk.Tk()
            root.title("Multi Mountain Tester")
            gui = MultiMountainTesterGUI (root, canvas_width=1600, canvas_height=800, tile_width=100, tile_height=60, texture=t)
            gui.setCoordinates (coordinates)
            root.mainloop ()
            break

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
