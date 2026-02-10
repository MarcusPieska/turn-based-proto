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
#=> - Globals -
#================================================================================================================================#

ALPHA_SOURCE_COLOR = (128, 128, 128)
RAW_TILE_COLOR = (255, 255, 255)
FULL_ALPHA_COLOR = (0, 0, 0)

#================================================================================================================================#
#=> - Helper Functions -
#================================================================================================================================#

def makeStraightNorthEast (raw_tile):
    source_coords = []
    width_half = raw_tile.width // 2
    for i in range (width_half, raw_tile.width):
        if raw_tile.getpixel((i, 0)) == RAW_TILE_COLOR:
            continue
        for j in range(0, raw_tile.height):
            if raw_tile.getpixel((i, j))[:3] == RAW_TILE_COLOR:
                raw_tile.putpixel((i, j - 1), ALPHA_SOURCE_COLOR)
                source_coords.append((i, j - 1))
                break
    
    img_array = np.array(raw_tile)
    width, height = raw_tile.size
    
    for y in range(height):
        for x in range(width):
            min_distance = float('inf')
            for sx, sy in source_coords:
                distance = math.sqrt((x - sx) ** 2 + (y - sy) ** 2)
                if distance < min_distance:
                    min_distance = distance
            grey_value = int(min(255, 0 + min_distance * 10))
            rand_val = random.randint(0, 255)
            grey_value = max(0, min(255, grey_value + random.randint(min(0, int(distance*2)-50), 50)))
            if rand_val // 2 > grey_value:
                img_array[y, x][:3] = FULL_ALPHA_COLOR
            elif grey_value < img_array[y, x][0]:
                img_array[y, x][:3] = (grey_value, grey_value, grey_value)

    for x, y in source_coords:
        img_array[y, x][:3] = (0, 0, 0)
    
    return Image.fromarray(img_array)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    raw_tile_path = "../../DEL_temp_tile_alpha_raw.png"
    raw_tile = Image.open(raw_tile_path)
    alpha_tile = makeStraightNorthEast(raw_tile)
    alpha_tile.save("../../DEL_temp_tile_alpha_north_east.png")

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
