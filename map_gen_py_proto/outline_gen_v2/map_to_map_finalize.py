#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
from PIL import Image

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def normalize_color (color_val):
    if isinstance(color_val, (list, tuple)):
        color_val = np.array(color_val, dtype=np.uint8)
    if isinstance(color_val, np.ndarray):
        return tuple(int(c) for c in (color_val[:3] if color_val.size >= 3 else [color_val[0]] * 3))
    return (int(color_val), int(color_val), int(color_val))

def is_approved_color (pixel_rgb, approved_colors):
    return pixel_rgb in approved_colors

def get_neighbors (y, x, h, w):
    neighbors = []
    if y > 0:
        neighbors.append((y - 1, x))
    if y < h - 1:
        neighbors.append((y + 1, x))
    if x > 0:
        neighbors.append((y, x - 1))
    if x < w - 1:
        neighbors.append((y, x + 1))
    return neighbors

def get_most_common_approved_color (y, x, img_array, approved_colors, h, w):
    neighbors = get_neighbors(y, x, h, w)
    approved_neighbor_colors = []
    for ny, nx in neighbors:
        neighbor_rgb = tuple(int(c) for c in img_array[ny, nx, :3])
        if is_approved_color(neighbor_rgb, approved_colors):
            approved_neighbor_colors.append(neighbor_rgb)
    if len(approved_neighbor_colors) == 0:
        return None
    color_counts = {}
    for color in approved_neighbor_colors:
        color_counts[color] = color_counts.get(color, 0) + 1
    return max(color_counts.items(), key=lambda x: x[1])[0]

def finalize_image (image_path, approved_colors):
    img_pil = Image.open(image_path)
    img_array = np.array(img_pil)
    if img_array.ndim == 2:
        h, w = img_array.shape
        img_array = img_array[:, :, np.newaxis]
    h, w, ch = img_array.shape
    if ch == 1:
        img_array = np.repeat(img_array, 3, axis=2)
    elif ch == 4:
        img_array = img_array[:, :, :3]
    approved_colors_set = set(normalize_color(c) for c in approved_colors)
    iteration = 0
    while True:
        iteration += 1
        pixels_to_recolor = []
        for y in range(h):
            for x in range(w):
                pixel_rgb = tuple(int(c) for c in img_array[y, x, :3])
                if not is_approved_color(pixel_rgb, approved_colors_set):
                    neighbors = get_neighbors(y, x, h, w)
                    has_approved_neighbor = False
                    for ny, nx in neighbors:
                        neighbor_rgb = tuple(int(c) for c in img_array[ny, nx, :3])
                        if is_approved_color(neighbor_rgb, approved_colors_set):
                            has_approved_neighbor = True
                            break
                    if has_approved_neighbor:
                        pixels_to_recolor.append((y, x))
        print(f"Iteration {iteration}: collected {len(pixels_to_recolor)} pixels")
        if len(pixels_to_recolor) == 0:
            break
        for y, x in pixels_to_recolor:
            new_color = get_most_common_approved_color(y, x, img_array, approved_colors_set, h, w)
            if new_color is not None:
                img_array[y, x, :] = new_color
    finalized_path = image_path.replace(".png", "_finalized.png")
    Image.fromarray(img_array).save(finalized_path)
    print(f"** Saved finalized image to: {finalized_path}")

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    colors = []
    colors.append([121, 189, 36])
    colors.append([222, 199, 89])
    colors.append([244, 203, 141])
    colors.append([248, 251, 252])
    colors.append([82, 63, 14])
    colors.append([32, 26, 120])
    colors.append([65, 156, 244])
    finalize_image("../../../DEL_world_map3_reduced.png", colors)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
