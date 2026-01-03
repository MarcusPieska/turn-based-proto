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

#================================================================================================================================#
#=> - Class: MountainGenerator
#================================================================================================================================#

class MountainGenerator:

    def __get_triangle_inside_pt (m, pt1, pt2, pt3):
        return ((pt1[0] + pt2[0] + pt3[0])/3, (pt1[1] + pt2[1] + pt3[1])/3)

    def __fill_area (m, img, seed_point, fill_color, blank_color):
        stack = [seed_point]
        visited = set()
        img_h, img_w = img.shape
        while stack:
            x, y = stack.pop()
            x, y = int(x), int(y)
            if (x, y) in visited or x < 0 or x >= img_w or y < 0 or y >= img_h or img[y, x] != blank_color:
                continue
            visited.add((x, y))
            img[y, x] = fill_color
            stack.extend([(x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)])

    def __init__ (m, width=100, height=100):
        m.width = width
        m.height = height
        m.mountain_peak_pt = (width // 2, int(height * 0.05))
        m.base_length = int(math.sqrt((width//1)**2 + (height//1)**2))
        m.end_pts = []

    def generateOutline (m, dark_side_color, light_side_color):
        image_matrix = np.zeros((m.height, m.width), dtype=np.uint8)

        lengths, angles, colors = [1.0, 1.1, 1.0], [30, 110, 150], [dark_side_color, light_side_color, light_side_color]
        for i in range(3):
            lengths[i] = int(lengths[i] * random.uniform(0.65, 0.85) * m.base_length)
            angles[i] += random.uniform(-10, 10)
        m.end_pts = []

        angle_rad = math.radians(angles[0])
        end_x = int(m.mountain_peak_pt[0] + lengths[0] * math.cos(angle_rad))
        end_y = int(m.mountain_peak_pt[1] + lengths[0] * math.sin(angle_rad))
        m.end_pts.append((end_x, end_y))
        m.__drawLine(image_matrix, m.mountain_peak_pt, (end_x, end_y), colors[0])
 
        angle_rad = math.radians(angles[1])
        end_x_mid = int(m.mountain_peak_pt[0] + lengths[1] * math.cos(angle_rad))
        end_y_mid = int(m.mountain_peak_pt[1] + lengths[1] * math.sin(angle_rad))
        m.end_pts.append((end_x_mid, end_y_mid))

        angle_rad = math.radians(angles[2])
        end_x = int(m.mountain_peak_pt[0] + lengths[2] * math.cos(angle_rad))
        end_y = int(m.mountain_peak_pt[1] + lengths[2] * math.sin(angle_rad))
        m.end_pts.append((end_x, end_y))
        m.__drawLine(image_matrix, m.mountain_peak_pt, (end_x, end_y), colors[2])
        
        m.__drawLine(image_matrix, m.end_pts[0], m.end_pts[1], dark_side_color)
        m.__drawLine(image_matrix, m.end_pts[1], m.end_pts[2], light_side_color)
        inside_pt = m.__get_triangle_inside_pt(m.end_pts[0], m.end_pts[1], m.mountain_peak_pt)
        m.__fill_area(image_matrix, inside_pt, dark_side_color, 0)

        m.__drawLine(image_matrix, m.mountain_peak_pt, (end_x_mid, end_y_mid), colors[1])
        inside_pt = m.__get_triangle_inside_pt(m.end_pts[1], m.end_pts[2], m.mountain_peak_pt)
        m.__fill_area(image_matrix, inside_pt, light_side_color, dark_side_color)

        alpha1 = np.zeros((m.height, m.width), dtype=np.uint8)
        for y in range(m.height):
            for x in range(m.width):
                if image_matrix[y, x] != 0:
                    alpha1[y, x] = 255
        
        alpha2 = np.zeros((m.height, m.width), dtype=np.uint8)
        peak_x, peak_y = m.mountain_peak_pt
        max_dist = m.base_length // 1.7
        for y in range(m.height):
            for x in range(m.width):
                dist = math.sqrt((x - peak_x) ** 2 + (y - peak_y) ** 2)
                if dist >= max_dist:
                    alpha2[y, x] = 0
                else:
                    alpha2[y, x] = int(255 - (255 * (dist / max_dist)))
        
        alpha = np.minimum(alpha1, alpha2)
        
        return image_matrix, alpha

    def __drawLine (m, img, pt1, pt2, color):
        dx = abs(pt2[0] - pt1[0])
        dy = abs(pt2[1] - pt1[1])
        sx = 1 if pt1[0] < pt2[0] else -1
        sy = 1 if pt1[1] < pt2[1] else -1
        err = dx - dy
        x, y = pt1[0], pt1[1]
        while True:
            if 0 <= x < img.shape[1] and 0 <= y < img.shape[0]:
                img[y, x] = color
            if x == pt2[0] and y == pt2[1]:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x += sx
            if e2 < dx:
                err += dx
                y += sy

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
