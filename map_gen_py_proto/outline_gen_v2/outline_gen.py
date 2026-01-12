#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import random
import math
from line_gen import LineGenerator
import numpy as np

#================================================================================================================================#
#=> - Class: OutlineGenerator
#================================================================================================================================#

class OutlineGenerator:

    def __init__ (m, canvas_width=200, canvas_height=100, zoom=8.0):
        m.angles, m.depths, m.points, m.line_pxs = [], [], [], []
        m.line_gen = LineGenerator()
        m.canvas_width = canvas_width
        m.canvas_height = canvas_height
        m.zoom = zoom
        m.min_angle_step = 1.0
        m.max_angle_step = 30.0
        m.min_depth = 0.3
        m.max_depth = 0.9
        m.rotation = 0.0

        m.color_bg = np.array([32, 26, 120], dtype=np.uint8)
        m.color_fill = np.array([121, 189, 36], dtype=np.uint8)
        m.color_outline = np.array([255, 255, 255], dtype=np.uint8)
        m.color_white = np.array([255, 255, 255], dtype=np.uint8)

    def setRandomSeed (m, seed):
        m.line_gen.setRandomSeed (int(seed))

    def generateAngles (m):
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

    def generateDepths (m):
        if not m.angles:
            return
        m.depths = []
        prev_depth, prev_change_was_sharp = 0.0, False
        for angle in m.angles:
            if not prev_change_was_sharp:
                depth = random.uniform(m.min_depth, m.max_depth)
            m.depths.append(depth)
            prev_change_was_sharp = abs(depth - prev_depth) > 0.1
            prev_depth = depth
        m.points = []

    def calculatePoints (m):
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
            rotated_angle = (angle + m.rotation) % 360.0
            depth = m.depths[i]
            radius = max_radius * depth
            angle_rad = math.radians(rotated_angle)
            x = center_x + radius * math.cos(angle_rad)
            y = center_y - radius * math.sin(angle_rad)
            m.points.append((x, y))

    def generateLines (m):
        if not m.points:
            m.calculatePoints ()
        if not m.points:
            return
        img_w = int(m.canvas_width * m.zoom)
        img_h = int(m.canvas_height * m.zoom)
        image_matrix = np.full((img_h, img_w, 3), m.color_bg, dtype=np.uint8)
        m.line_pxs = []
        for i in range(len(m.points)):
            p1 = m.points[i]
            p2 = m.points[(i + 1) % len(m.points)]
            x1, y1 = p1[0] * m.zoom, p1[1] * m.zoom
            x2, y2 = p2[0] * m.zoom, p2[1] * m.zoom
            dx = x2 - x1
            dy = y2 - y1
            length = math.sqrt(dx * dx + dy * dy) / m.zoom
            angle = math.degrees(math.atan2(-dy, dx))
            pxs = m.line_gen.generateLinePixels(line_length=length, angle=angle, width=img_w, height=img_h, zoom=m.zoom)
            offset_x = int(x1 - img_w // 2)
            offset_y = int(y1 - img_h // 2)
            for px, py in pxs:
                img_x = px + offset_x
                img_y = py + offset_y
                if 0 <= img_x < img_w and 0 <= img_y < img_h:
                    image_matrix[img_y, img_x] = m.color_white
                    m.line_pxs.append((img_x, img_y))
        
        m.fillPolygon(image_matrix)
        m.removeOutline(image_matrix)
        return image_matrix

    def removeOutline (m, image_matrix):
        white_mask = np.all(image_matrix == m.color_white, axis=2)
        image_matrix[white_mask] = m.color_bg

    def fillPolygon (m, image_matrix):
        if len (m.points) < 3:
            return
        img_h, img_w, _ = image_matrix.shape
        center_x = int (m.canvas_width * m.zoom // 2)
        center_y = int (m.canvas_height * m.zoom // 2)
        if not np.array_equal(image_matrix[center_y, center_x], m.color_bg):
            return
        stack = [(center_x, center_y)]
        visited = set ()
        while stack:
            x, y = stack.pop ()
            if (x, y) in visited:
                continue
            if x < 0 or x >= img_w or y < 0 or y >= img_h:
                continue
            if not np.array_equal(image_matrix[y, x], m.color_bg):
                continue
            visited.add ((x, y))
            image_matrix[y, x] = m.color_fill
            stack.append ((x + 1, y))
            stack.append ((x - 1, y))
            stack.append ((x, y + 1))
            stack.append ((x, y - 1))

    def getPoints (m):
        return m.points

    def getAllOutlinePixels (m):
        return m.line_pxs

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
