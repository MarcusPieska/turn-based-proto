#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import random
import tkinter as tk
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: PatchTextureGenerator
#================================================================================================================================#

class PatchTextureGenerator:

    def __parsePatchFile (m, patch_path):
        patch_data = []
        if os.path.exists(patch_path):
            with open(patch_path, "r") as ptr:
                for line in ptr:
                    line = line.strip()
                    if line:
                        parts = line.split(":")
                        if len(parts) == 4:
                            try:
                                px_count = int(parts[0])
                                r, g, b = int(parts[1]), int(parts[2]), int(parts[3])
                                patch_data.append((px_count, (r, g, b)))
                            except ValueError:
                                pass
        else:
            print ("*** Warning: Patch file not found: %s" %(patch_path))
        return patch_data

    def __generatePatchShape (m, pixel_count):
        patch_coords = set()
        candidates = set()
        if pixel_count <= 0:
            return patch_coords
        seed_x = 0
        seed_y = 0
        patch_coords.add((seed_x, seed_y))
        neighbors = [(seed_x - 1, seed_y), (seed_x + 1, seed_y), (seed_x, seed_y - 1), (seed_x, seed_y + 1)]
        for nx, ny in neighbors:
            candidates.add((nx, ny))
        while len(patch_coords) < pixel_count and len(candidates) > 0:
            candidate_list = list(candidates)
            chosen = random.choice(candidate_list)
            candidates.remove(chosen)
            if chosen not in patch_coords:
                patch_coords.add(chosen)
                cx, cy = chosen
                new_neighbors = [(cx - 1, cy), (cx + 1, cy), (cx, cy - 1), (cx, cy + 1)]
                for nx, ny in new_neighbors:
                    if (nx, ny) not in patch_coords:
                        candidates.add((nx, ny))
        return patch_coords

    def __findPlacement (m, patch_coords):
        min_x = min(x for x, y in patch_coords)
        min_y = min(y for x, y in patch_coords)
        normalized_coords = [(x - min_x, y - min_y) for x, y in patch_coords]
        max_x = max(x for x, y in normalized_coords)
        max_y = max(y for x, y in normalized_coords)
        for attempt in range(1000):
            start_x = random.randint(0, m.width - max_x - 1) if max_x < m.width else 0
            start_y = random.randint(0, m.height - max_y - 1) if max_y < m.height else 0
            can_place = True
            for px, py in normalized_coords:
                canvas_x = start_x + px
                canvas_y = start_y + py
                if canvas_x >= m.width or canvas_y >= m.height:
                    can_place = False
                    break
                if tuple(m.canvas[canvas_y, canvas_x]) != m.background_color:
                    can_place = False
                    break
            if can_place:
                return [(start_x + px, start_y + py) for px, py in normalized_coords]
        return None

    def __findPlacement1px (m):
        for y in range(m.height):
            for x in range(m.width):
                if tuple(m.canvas[y, x]) == m.background_color:
                    return [(x, y)]
        return None

    def __placePatch (m, patch_coords, color):
        placement = m.__findPlacement(patch_coords)
        if placement is not None:
            for x, y in placement:
                if 0 <= x < m.width and 0 <= y < m.height:
                    m.canvas[y, x] = color
            m.__updateDisplay()
            return True
        return False

    def __placePatch1px (m, color):
        placement = m.__findPlacement1px()
        if placement is not None:
            x, y = placement[0]
            m.canvas[y, x] = color
            m.__updateDisplay()
            return True
        return False

    def __fillRemainingBackground (m):
        non_bg_colors = []
        for px_count, color in m.patch_data:
            if color != m.background_color:
                non_bg_colors.append(color)
        if len(non_bg_colors) == 0:
            return
        filled_count = 0
        for y in range(m.height):
            for x in range(m.width):
                if tuple(m.canvas[y, x]) == m.background_color:
                    color = random.choice(non_bg_colors)
                    m.canvas[y, x] = color
                    filled_count += 1
        print ("*** Filled %d remaining background pixels" %(filled_count))
        m.__updateDisplay()

    def __estimateMultiplier (m, patch_data):
        total_patch_pixels = sum(px_count for px_count, color in patch_data)
        canvas_pixels = m.width * m.height
        if total_patch_pixels == 0:
            return 1
        multiplier = max(1, (canvas_pixels + total_patch_pixels - 1) // total_patch_pixels)
        return multiplier

    def __updateDisplay (m):
        if m.canvas is not None and m.display_canvas is not None:
            img = Image.fromarray(m.canvas, mode='RGB')
            zoom_factor = min(m.display_canvas.winfo_width() / m.width, m.display_canvas.winfo_height() / m.height) if m.display_canvas.winfo_width() > 1 else 1.0
            if zoom_factor < 1.0:
                display_width = int(m.width * zoom_factor)
                display_height = int(m.height * zoom_factor)
                img = img.resize((display_width, display_height), Image.NEAREST)
            else:
                display_width = m.width
                display_height = m.height
            photo = ImageTk.PhotoImage(img)
            m.display_canvas.delete("all")
            center_x = m.display_canvas.winfo_width() // 2 if m.display_canvas.winfo_width() > 1 else display_width // 2
            center_y = m.display_canvas.winfo_height() // 2 if m.display_canvas.winfo_height() > 1 else display_height // 2
            m.display_canvas.create_image(center_x, center_y, anchor=tk.CENTER, image=photo, tags="texture")
            m.display_canvas.image = photo
            m.root.update()

    def __generateTexture (m):
        m.canvas = np.zeros((m.height, m.width, 3), dtype=np.uint8)
        m.canvas[:, :] = m.background_color
        m.__updateDisplay()
        expanded_patches = []
        for px_count, color in m.patch_data:
            expanded_patches.append((px_count, color))
        multiplier = m.__estimateMultiplier(m.patch_data)
        print ("*** Estimated multiplier: %d" %(multiplier))
        for i in range(multiplier):
            for px_count, color in m.patch_data:
                expanded_patches.append((px_count, color))
        expanded_patches.sort(key=lambda x: x[0], reverse=True)
        multi_px_patches = [(px_count, color) for px_count, color in expanded_patches if px_count > 1]
        single_px_patches = [(px_count, color) for px_count, color in expanded_patches if px_count == 1]
        multi_px_count = len(multi_px_patches)
        single_px_count = len(single_px_patches)
        multi_px_placed = 0
        for idx, (px_count, color) in enumerate(multi_px_patches):
            patch_coords = m.__generatePatchShape(px_count)
            if m.__placePatch(patch_coords, color):
                multi_px_placed += 1
            print ("*** Placing multi-pixel patches: %d/%d placed" %(multi_px_placed, multi_px_count), end='\r')
            sys.stdout.flush()
        print ("")
        print ("*** Placed %d multi-pixel patches" %(multi_px_placed))
        single_px_placed = 0
        for idx, (px_count, color) in enumerate(single_px_patches):
            if m.__placePatch1px(color):
                single_px_placed += 1
            print ("*** Placing single-pixel patches: %d/%d placed" %(single_px_placed, single_px_count), end='\r')
            sys.stdout.flush()
        print ("")
        print ("*** Placed %d single-pixel patches" %(single_px_placed))
        print ("*** Total placed: %d patches" %(multi_px_placed + single_px_placed))
        m.__fillRemainingBackground()

    def __saveTexture (m, output_path):
        img = Image.fromarray(m.canvas, mode='RGB')
        img.save(output_path)

    def __init__ (m, root, height, width, patch_file_path):
        m.root = root
        m.height = height
        m.width = width
        m.background_color = (255, 0, 255)
        print ("*** Reading patch file: %s" %(patch_file_path))
        m.patch_data = m.__parsePatchFile(patch_file_path)
        m.patch_data.sort(key=lambda x: x[0], reverse=True)
        total_pixel_count = sum(px_count for px_count, color in m.patch_data)
        print ("*** Loaded %d patch entries" %(len(m.patch_data)))
        print ("*** Total patch pixel count: %d" %(total_pixel_count))
        m.canvas = None
        m.display_canvas = tk.Canvas(root, width=800, height=800, bg="gray")
        m.display_canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        m.__generateTexture()
        output_path = patch_file_path.replace("_patches.txt", "_texture.png").replace(".txt", "_texture.png")
        m.__saveTexture(output_path)
        print ("*** Saved texture to %s" %(output_path))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Patch Texture Generator")
    height = 512
    width = 512
    patch_file_path = "/home/w/Projects/img-content/colors-grassland3_palette_patches.txt"
    generator = PatchTextureGenerator(root, height, width, patch_file_path)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
