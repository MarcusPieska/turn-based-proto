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
import numpy as np

#================================================================================================================================#
#=> - Class: LineGenerator
#================================================================================================================================#

class LineGenerator:

    def __init__ (m):
        pass

    def setRandomSeed (m, seed):
        random.seed(int(seed))

    def generateLine (m, line_length, angle, bend_factor=None, peak_position=None, width=800, height=600, zoom=4.0):
        if bend_factor is None:
            bend_factor = random.uniform(0.0, 1.0)
        if peak_position is None:
            peak_position = random.uniform(0.2, 0.8)
        width = int(width)
        height = int(height)
        center_x = width // 2
        center_y = height // 2
        
        num_points = max(50, int(line_length * zoom))
        max_bend = line_length * 0.3 * bend_factor
        
        p0_x = 0
        p0_y = 0
        p1_x = line_length * zoom * peak_position
        p1_y = max_bend * zoom
        p2_x = line_length * zoom
        p2_y = 0
        
        points = []
        for i in range(num_points + 1):
            t = i / num_points
            x_local = (1 - t) * (1 - t) * p0_x + 2 * (1 - t) * t * p1_x + t * t * p2_x
            y_local = (1 - t) * (1 - t) * p0_y + 2 * (1 - t) * t * p1_y + t * t * p2_y
            points.append((x_local, y_local))
        
        angle_rad = math.radians(angle)
        cos_a = math.cos(angle_rad)
        sin_a = math.sin(angle_rad)
        
        transformed_points = []
        for x, y in points:
            x_rot = x * cos_a - y * sin_a
            y_rot = x * sin_a + y * cos_a
            px = int(center_x + x_rot)
            py = int(center_y - y_rot)
            if 0 <= px < width and 0 <= py < height:
                transformed_points.append((px, py))
        
        image_matrix = np.zeros((height, width), dtype=np.uint8)
        if len(transformed_points) > 1:
            for i in range(len(transformed_points) - 1):
                x1, y1 = transformed_points[i]
                x2, y2 = transformed_points[i + 1]
                m.__drawLineSegment(image_matrix, x1, y1, x2, y2)
        
        return image_matrix

    def generateLinePixels (m, line_length, angle, bend_factor=None, peak_position=None, width=800, height=600, zoom=4.0):
        if bend_factor is None:
            bend_factor = random.uniform(0.0, 1.0)
        if peak_position is None:
            peak_position = random.uniform(0.2, 0.8)
        width = int(width)
        height = int(height)
        center_x = width // 2
        center_y = height // 2
        
        num_points = max(50, int(line_length * zoom))
        max_bend = line_length * 0.3 * bend_factor
        
        p0_x = 0
        p0_y = 0
        p1_x = line_length * zoom * peak_position
        p1_y = max_bend * zoom
        p2_x = line_length * zoom
        p2_y = 0
        
        points = []
        for i in range(num_points + 1):
            t = i / num_points
            x_local = (1 - t) * (1 - t) * p0_x + 2 * (1 - t) * t * p1_x + t * t * p2_x
            y_local = (1 - t) * (1 - t) * p0_y + 2 * (1 - t) * t * p1_y + t * t * p2_y
            points.append((x_local, y_local))
        
        angle_rad = math.radians(angle)
        cos_a = math.cos(angle_rad)
        sin_a = math.sin(angle_rad)
        
        transformed_points = []
        for x, y in points:
            x_rot = x * cos_a - y * sin_a
            y_rot = x * sin_a + y * cos_a
            px = int(center_x + x_rot)
            py = int(center_y - y_rot)
            if 0 <= px < width and 0 <= py < height:
                transformed_points.append((px, py))
        
        pixel_list = []
        if len(transformed_points) > 1:
            for i in range(len(transformed_points) - 1):
                x1, y1 = transformed_points[i]
                x2, y2 = transformed_points[i + 1]
                segment_pixels = m.__getLineSegmentPixels(x1, y1, x2, y2, width, height)
                pixel_list.extend(segment_pixels)
        
        return pixel_list

    def __getLineSegmentPixels (m, x1, y1, x2, y2, width, height):
        pixels = []
        dx = abs(x2 - x1)
        dy = abs(y2 - y1)
        sx = 1 if x1 < x2 else -1
        sy = 1 if y1 < y2 else -1
        err = dx - dy
        x, y = x1, y1
        while True:
            if 0 <= x < width and 0 <= y < height:
                pixels.append((x, y))
            if x == x2 and y == y2:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x += sx
            if e2 < dx:
                err += dx
                y += sy
        return pixels

    def __drawLineSegment (m, img, x1, y1, x2, y2):
        dx = abs(x2 - x1)
        dy = abs(y2 - y1)
        sx = 1 if x1 < x2 else -1
        sy = 1 if y1 < y2 else -1
        err = dx - dy
        x, y = x1, y1
        while True:
            if 0 <= x < img.shape[1] and 0 <= y < img.shape[0]:
                img[y, x] = 255
            if x == x2 and y == y2:
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
