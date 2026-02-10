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
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: TestTriangleMaker
#================================================================================================================================#

class TestTriangleMaker:

    def __init__ (m, size=200, texture=None):
        m.size = size
        m.texture = texture
        m.height = int(size * math.sqrt(3) / 2)
        m.width = size
        m.triangle = None

    def generateTriangle (m):
        v1 = (0, m.height)
        v2 = (m.width, m.height)
        v3 = (m.width // 2, 0)
        m.triangle = (v1, v2, v3)
        if m.texture is not None:
            image_matrix = np.array(Image.open(m.texture).convert('L'))
            return image_matrix, m.triangle
        image_matrix = np.zeros((m.height, m.width), dtype=np.uint8)
        stripe_width = max(3, m.width // 20)
        for y in range(m.height):
            for x in range(m.width):
                if m.__pointInTriangle(x, y, v1, v2, v3):
                    stripe_index = x // stripe_width
                    image_matrix[y, x] = 128 if stripe_index % 2 == 0 else 255
        
        return image_matrix, m.triangle

    def __pointInTriangle (m, x, y, v1, v2, v3):
        def sign(p1, p2, p3):
            return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
        
        d1 = sign((x, y), v1, v2)
        d2 = sign((x, y), v2, v3)
        d3 = sign((x, y), v3, v1)
        
        has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
        has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
        
        return not (has_neg and has_pos)

#================================================================================================================================#
#=> - Class: TriangleMorpher
#================================================================================================================================#

class TriangleMorpher:

    def __init__ (m, root, size=200, texture=None):
        m.root = root
        m.size = size
        m.texture = texture
        m.triangle_maker = TestTriangleMaker(size=size, texture=texture)
        m.original_image = None
        m.original_triangle = None
        m.morphed_triangle = None
        m.min_size_factor = 0.5
        m.max_size_factor = 2.0
        m.min_rotation = -75
        m.max_rotation = 75
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.new_angles_btn = tk.Button(m.top_frame, text="New angles", command=m.__newAngles, width=15)
        m.new_angles_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_size_btn = tk.Button(m.top_frame, text="New size", command=m.__newSize, width=15)
        m.new_size_btn.pack(side=tk.LEFT, padx=5)
        
        m.canvas_frame = tk.Frame(root)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        canvas_size = 500
        m.canvas_original = tk.Canvas(m.canvas_frame, width=canvas_size, height=canvas_size, bg="white")
        m.canvas_original.pack(side=tk.LEFT, padx=5)
        
        m.canvas_outline = tk.Canvas(m.canvas_frame, width=canvas_size, height=canvas_size, bg="white")
        m.canvas_outline.pack(side=tk.LEFT, padx=5)
        
        m.canvas_morphed = tk.Canvas(m.canvas_frame, width=canvas_size, height=canvas_size, bg="white")
        m.canvas_morphed.pack(side=tk.LEFT, padx=5)
        
        m.__generateOriginal()
        m.__newAngles()

    def __generateOriginal (m):
        m.original_image, m.original_triangle = m.triangle_maker.generateTriangle()
        m.__displayOriginal()

    def __newAngles (m):
        if m.original_triangle is None:
            return
        v1, v2, v3 = m.original_triangle
        center_x = (v1[0] + v2[0] + v3[0]) / 3
        center_y = (v1[1] + v2[1] + v3[1]) / 3
        
        base_dist1 = math.sqrt((v1[0] - center_x) ** 2 + (v1[1] - center_y) ** 2)
        base_dist2 = math.sqrt((v2[0] - center_x) ** 2 + (v2[1] - center_y) ** 2)
        base_dist3 = math.sqrt((v3[0] - center_x) ** 2 + (v3[1] - center_y) ** 2)
        
        base_angle1 = math.degrees(math.atan2(v1[1] - center_y, v1[0] - center_x))
        base_angle2 = math.degrees(math.atan2(v2[1] - center_y, v2[0] - center_x))
        base_angle3 = math.degrees(math.atan2(v3[1] - center_y, v3[0] - center_x))
        
        angle1 = base_angle1 + random.uniform(m.min_rotation, m.max_rotation)
        angle2 = base_angle2 + random.uniform(m.min_rotation, m.max_rotation)
        angle3 = base_angle3 + random.uniform(m.min_rotation, m.max_rotation)
        dist1 = base_dist1 * random.uniform(m.min_size_factor, m.max_size_factor)
        dist2 = base_dist2 * random.uniform(m.min_size_factor, m.max_size_factor)
        dist3 = base_dist3 * random.uniform(m.min_size_factor, m.max_size_factor)
        
        new_v1 = (center_x + dist1 * math.cos(math.radians(angle1)), center_y + dist1 * math.sin(math.radians(angle1)))
        new_v2 = (center_x + dist2 * math.cos(math.radians(angle2)), center_y + dist2 * math.sin(math.radians(angle2)))
        new_v3 = (center_x + dist3 * math.cos(math.radians(angle3)), center_y + dist3 * math.sin(math.radians(angle3)))
        
        new_center_x = (new_v1[0] + new_v2[0] + new_v3[0]) / 3
        new_center_y = (new_v1[1] + new_v2[1] + new_v3[1]) / 3
        
        canvas_center_x, canvas_center_y = 150, 150
        offset_x = canvas_center_x - new_center_x
        offset_y = canvas_center_y - new_center_y
        
        m.morphed_triangle = ((new_v1[0] + offset_x, new_v1[1] + offset_y), 
                              (new_v2[0] + offset_x, new_v2[1] + offset_y), 
                              (new_v3[0] + offset_x, new_v3[1] + offset_y))
        m.__displayOutline()
        m.__displayMorphed()

    def __newSize (m):
        m.__generateOriginal()
        m.__newAngles()

    def __displayOriginal (m):
        m.canvas_original.delete("all")
        if len(m.original_image.shape) == 3:
            if m.original_image.shape[2] == 4:
                img = Image.fromarray(m.original_image[:, :, :3], mode='RGB')
            else:
                img = Image.fromarray(m.original_image, mode='RGB')
        else:
            img = Image.fromarray(m.original_image, mode='L')
        photo = ImageTk.PhotoImage(img)
        m.canvas_original.create_image(150, 150, anchor=tk.CENTER, image=photo)
        m.canvas_original.image = photo

    def __displayOutline (m):
        m.canvas_outline.delete("all")
        if m.morphed_triangle is None:
            return
        v1, v2, v3 = m.morphed_triangle
        m.canvas_outline.create_line(v1[0], v1[1], v2[0], v2[1], fill="black", width=2)
        m.canvas_outline.create_line(v2[0], v2[1], v3[0], v3[1], fill="black", width=2)
        m.canvas_outline.create_line(v3[0], v3[1], v1[0], v1[1], fill="black", width=2)

    def __displayMorphed (m):
        m.canvas_morphed.delete("all")
        if m.morphed_triangle is None or m.original_image is None:
            return
        
        morphed_image = m.__morphTriangle(m.original_image, m.original_triangle, m.morphed_triangle, 300, 300)
        img = Image.fromarray(morphed_image, mode='L')
        photo = ImageTk.PhotoImage(img)
        m.canvas_morphed.create_image(150, 150, anchor=tk.CENTER, image=photo)
        m.canvas_morphed.image = photo

    def __morphTriangle (m, source_img, source_tri, dest_tri, width, height):
        dest_image = np.zeros((height, width), dtype=np.uint8)
        v1_s, v2_s, v3_s = source_tri
        v1_d, v2_d, v3_d = dest_tri
        
        min_x = int(max(0, min(v[0] for v in dest_tri)))
        max_x = int(min(width - 1, max(v[0] for v in dest_tri)))
        min_y = int(max(0, min(v[1] for v in dest_tri)))
        max_y = int(min(height - 1, max(v[1] for v in dest_tri)))
        
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                if m.__pointInTriangle(x, y, v1_d, v2_d, v3_d):
                    bary = m.__barycentric(x, y, v1_d, v2_d, v3_d)
                    if bary[0] >= 0 and bary[1] >= 0 and bary[2] >= 0:
                        src_x = int(bary[0] * v1_s[0] + bary[1] * v2_s[0] + bary[2] * v3_s[0])
                        src_y = int(bary[0] * v1_s[1] + bary[1] * v2_s[1] + bary[2] * v3_s[1])
                        
                        if 0 <= src_x < source_img.shape[1] and 0 <= src_y < source_img.shape[0]:
                            dest_image[y, x] = source_img[src_y, src_x]
        
        return dest_image

    def __barycentric (m, x, y, v1, v2, v3):
        v0x, v0y = v2[0] - v1[0], v2[1] - v1[1]
        v1x, v1y = v3[0] - v1[0], v3[1] - v1[1]
        v2x, v2y = x - v1[0], y - v1[1]
        
        dot00 = v0x * v0x + v0y * v0y
        dot01 = v0x * v1x + v0y * v1y
        dot02 = v0x * v2x + v0y * v2y
        dot11 = v1x * v1x + v1y * v1y
        dot12 = v1x * v2x + v1y * v2y
        
        inv_denom = 1 / (dot00 * dot11 - dot01 * dot01)
        u = (dot11 * dot02 - dot01 * dot12) * inv_denom
        v = (dot00 * dot12 - dot01 * dot02) * inv_denom
        
        return (1 - u - v, u, v)

    def __pointInTriangle (m, x, y, v1, v2, v3):
        def sign(p1, p2, p3):
            return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
        
        d1 = sign((x, y), v1, v2)
        d2 = sign((x, y), v2, v3)
        d3 = sign((x, y), v3, v1)
        
        has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
        has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
        
        return not (has_neg and has_pos)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    tex1 = "../../DEL_temp_mtn_tex_0_139_90_43.png"
    tex2 = "../../DEL_temp_mtn_tex_0_140_110_85.png"
    tex3 = "../../DEL_temp_mtn_tex_0_100_100_100.png"
    tex4 = "../../DEL_temp_mtn_tex_0_101_75_60.png"
    tex5 = "../../DEL_temp_mtn_tex_0_120_90_70.png"

    root = tk.Tk()
    root.title ("Triangle Morpher")
    gui = TriangleMorpher (root, texture=tex1)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
