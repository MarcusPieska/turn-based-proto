#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
import random
import math
from line_gen import LineGenerator
from PIL import Image, ImageTk
import numpy as np

#================================================================================================================================#
#=> - Class: OutlineGenGUI
#================================================================================================================================#

class OutlineGenGUI:

    def __init__ (m, root, width=200, height=100, zoom=4.0):
        m.root = root
        m.angles = []
        m.depths = []
        m.points = []
        m.generator = LineGenerator()
        m.canvas_width = 200
        m.canvas_height = 100
        m.zoom = 8.0
        m.min_angle_step = 1.0
        m.max_angle_step = 30.0
        m.min_depth = 0.3
        m.max_depth = 0.9
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.new_angles_btn = tk.Button(m.top_frame, text="New angles", command=m.__newAngles, width=12)
        m.new_angles_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_depths_btn = tk.Button(m.top_frame, text="New depths", command=m.__newDepths, width=12)
        m.new_depths_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_lines_btn = tk.Button(m.top_frame, text="New lines", command=m.__newLines, width=12)
        m.new_lines_btn.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(root, width=m.canvas_width * m.zoom, height=m.canvas_height * m.zoom, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.__newAngles()

    def __newAngles (m):
        m.angles = []
        current_angle = 0.0
        while current_angle < 360.0:
            angle_step = random.uniform(m.min_angle_step, m.max_angle_step)
            current_angle += angle_step
            if current_angle >= 360.0:
                break
            m.angles.append(current_angle)
        m.depths = []
        m.points = []
        m.__drawCanvas()

    def __newDepths (m):
        if not m.angles:
            return
        m.depths = []
        center_x = m.canvas_width // 2
        center_y = m.canvas_height // 2
        max_radius = min(center_x, center_y) * 0.9
        prev_depth, prev_change_was_sharp = 0.0, False
        for angle in m.angles:
            if not prev_change_was_sharp:
                depth = random.uniform(m.min_depth, m.max_depth)
            m.depths.append(depth)
            prev_change_was_sharp = abs(depth - prev_depth) > 0.1
            prev_depth = depth
        m.points = []
        m.__drawCanvas()

    def __newLines (m):
        if not m.angles or not m.depths:
            return
        if len(m.angles) != len(m.depths):
            return
        
        center_x = m.canvas_width // 2
        center_y = m.canvas_height // 2
        max_radius = min(center_x, center_y) * 0.9
        
        m.points = []
        for i in range(len(m.angles)):
            angle = m.angles[i]
            depth = m.depths[i]
            radius = max_radius * depth
            angle_rad = math.radians(angle)
            x = center_x + radius * math.cos(angle_rad)
            y = center_y - radius * math.sin(angle_rad)
            m.points.append((x, y))
        
        image_matrix = np.zeros((int(m.canvas_height * m.zoom), int(m.canvas_width * m.zoom)), dtype=np.uint8)
        img_w = int(m.canvas_width * m.zoom)
        img_h = int(m.canvas_height * m.zoom)
        
        for i in range(len(m.points)):
            p1 = m.points[i]
            p2 = m.points[(i + 1) % len(m.points)]
            x1, y1 = p1[0] * m.zoom, p1[1] * m.zoom
            x2, y2 = p2[0] * m.zoom, p2[1] * m.zoom
            dx = x2 - x1
            dy = y2 - y1
            line_len = math.sqrt(dx * dx + dy * dy) / m.zoom
            angle = math.degrees(math.atan2(-dy, dx))
            pxs = m.generator.generateLinePixels(line_length=line_len, angle=angle, width=img_w, height=img_h, zoom=m.zoom)
            offset_x = int(x1 - img_w // 2)
            offset_y = int(y1 - img_h // 2)
            
            for px, py in pxs:
                img_x = px + offset_x
                img_y = py + offset_y
                if 0 <= img_x < img_w and 0 <= img_y < img_h:
                    image_matrix[img_y, img_x] = 255
        
        m.__fillPolygon(image_matrix)
        m.__drawImageMatrix(image_matrix)

    def __drawCanvas (m):
        m.canvas.delete("all")
        center_x = m.canvas_width * m.zoom // 2
        center_y = m.canvas_height * m.zoom // 2
        max_radius = min(m.canvas_width, m.canvas_height) * m.zoom * 0.45
        
        for i, angle in enumerate(m.angles):
            angle_rad = math.radians(angle)
            end_x = center_x + max_radius * math.cos(angle_rad)
            end_y = center_y - max_radius * math.sin(angle_rad)
            color = "#%02x%02x%02x" % (200, 200, 200)
            m.canvas.create_line(center_x, center_y, end_x, end_y, fill=color, width=1)
            
            if i < len(m.depths):
                depth = m.depths[i]
                depth_radius = max_radius * depth
                depth_x = center_x + depth_radius * math.cos(angle_rad)
                depth_y = center_y - depth_radius * math.sin(angle_rad)
                m.canvas.create_oval(depth_x - 2, depth_y - 2, depth_x + 2, depth_y + 2, fill="blue", outline="blue")

    def __fillPolygon (m, image_matrix):
        if len(m.points) < 3:
            return
        
        img_h, img_w = image_matrix.shape
        center_x = int(m.canvas_width * m.zoom // 2)
        center_y = int(m.canvas_height * m.zoom // 2)
        
        if image_matrix[center_y, center_x] != 0:
            return
        
        stack = [(center_x, center_y)]
        visited = set()
        
        while stack:
            x, y = stack.pop()
            if (x, y) in visited:
                continue
            if x < 0 or x >= img_w or y < 0 or y >= img_h:
                continue
            if image_matrix[y, x] != 0:
                continue
            
            visited.add((x, y))
            image_matrix[y, x] = 128
            
            stack.append((x + 1, y))
            stack.append((x - 1, y))
            stack.append((x, y + 1))
            stack.append((x, y - 1))
        
    def __drawImageMatrix (m, image_matrix):
        m.canvas.delete("all")
        img = Image.fromarray(image_matrix, mode='L')
        photo = ImageTk.PhotoImage(img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo

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
