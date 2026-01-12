#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from outline_gen import OutlineGenerator
from PIL import Image, ImageTk
import math

#================================================================================================================================#
#=> - Class: OutlineGenGUI
#================================================================================================================================#

class OutlineGenGUI:

    def __onRotationChange (m, value):
        m.out_gen.rotation = float (value)
        if m.out_gen.points:
            m.out_gen.calculatePoints()
            m.__newLines (reset=True)
        else:
            m.__drawCanvas ()

    def __newAngles (m):
        m.out_gen.generateAngles()
        m.__drawCanvas()

    def __newDepths (m):
        m.out_gen.generateDepths()
        m.__drawCanvas()

    def __newLines (m, reset=False):
        if not reset:
            m.line_gen_seed += 1
        m.out_gen.line_gen.setRandomSeed (m.line_gen_seed)
        image_matrix = m.out_gen.generateLines()
        if image_matrix is not None:
            m.__drawImageMatrix(image_matrix)

    def __drawCanvas (m):
        m.canvas.delete ("all")
        center_x = m.out_gen.canvas_width * m.out_gen.zoom // 2
        center_y = m.out_gen.canvas_height * m.out_gen.zoom // 2
        max_radius = min (m.out_gen.canvas_width, m.out_gen.canvas_height) * m.out_gen.zoom * 0.45
        
        for i, angle in enumerate (m.out_gen.angles):
            rotated_angle = (angle + m.out_gen.rotation) % 360.0
            angle_rad = math.radians (rotated_angle)
            end_x = center_x + max_radius * math.cos (angle_rad)
            end_y = center_y - max_radius * math.sin (angle_rad)
            color = "#%02x%02x%02x" % (200, 200, 200)
            m.canvas.create_line (center_x, center_y, end_x, end_y, fill=color, width=1)
            
            if i < len(m.out_gen.depths):
                depth = m.out_gen.depths[i]
                depth_radius = max_radius * depth
                depth_x = center_x + depth_radius * math.cos (angle_rad)
                depth_y = center_y - depth_radius * math.sin (angle_rad)
                m.canvas.create_oval (depth_x - 2, depth_y - 2, depth_x + 2, depth_y + 2, fill="blue", outline="blue")
        
    def __drawImageMatrix (m, image_matrix):
        m.canvas.delete ("all")
        img = Image.fromarray (image_matrix, mode='RGB')
        photo = ImageTk.PhotoImage (img)
        m.canvas.create_image (0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo

    def __init__ (m, root, width=200, height=100, zoom=4.0):
        m.root = root
        m.out_gen = OutlineGenerator(canvas_width=200, canvas_height=100, zoom=8.0)
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.new_angles_btn = tk.Button(m.top_frame, text="New angles", command=m.__newAngles, width=12)
        m.new_angles_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_depths_btn = tk.Button(m.top_frame, text="New depths", command=m.__newDepths, width=12)
        m.new_depths_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_lines_btn = tk.Button(m.top_frame, text="New lines", command=m.__newLines, width=12)
        m.new_lines_btn.pack(side=tk.LEFT, padx=5)
        
        m.rotation_label = tk.Label(m.top_frame, text="Rotation:")
        m.rotation_label.pack(side=tk.LEFT, padx=(10, 5))
        
        m.rotation_var = tk.DoubleVar(value=0.0)
        m.rotation_slider = tk.Scale(m.top_frame, from_=0, to=360, orient=tk.HORIZONTAL, length=200, resolution=1.0)
        m.rotation_slider.configure(variable=m.rotation_var)
        m.rotation_slider.configure(command=m.__onRotationChange)
        m.rotation_slider.set(m.out_gen.rotation)
        m.rotation_slider.pack(side=tk.LEFT, padx=5)
        
        height = m.out_gen.canvas_height * m.out_gen.zoom
        width = m.out_gen.canvas_width * m.out_gen.zoom
        m.canvas = tk.Canvas(root, width=width, height=height, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.line_gen_seed = 0
        m.__newAngles()

    def get_points (m):
        return m.out_gen.get_points ()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title ("Outline Generator")
    width, height, zoom_factor = 1800, 800, 1.0
    gui = OutlineGenGUI (root, width=width, height=height, zoom=zoom_factor)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
