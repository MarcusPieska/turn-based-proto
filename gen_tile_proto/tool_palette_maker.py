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
#=> - Class: PaletteMaker
#================================================================================================================================#

class PaletteMaker:

    def __loadImage (m):
        if os.path.exists(m.image_path):
            img = Image.open(m.image_path)
            img_rgb = img.convert('RGB')
            m.image_array = np.array(img_rgb)
            m.__displayImage()
            m.__analyzeColors()
        else:
            m.image_array = None

    def __analyzeColors (m):
        if m.image_array is None:
            return
        unique_colors = set()
        color_to_coords = {}
        height, width = m.image_array.shape[:2]
        for y in range(height):
            for x in range(width):
                r, g, b = m.image_array[y, x]
                color_tuple = (int(r), int(g), int(b))
                unique_colors.add(color_tuple)
                if color_tuple not in color_to_coords:
                    color_to_coords[color_tuple] = []
                color_to_coords[color_tuple].append((x, y))
        m.color_to_coords = color_to_coords
        print ("*** Found %d unique colors" %(len(unique_colors)))
        m.__mergeColors()

    def __colorDistance (m, color1, color2):
        r1, g1, b1 = color1
        r2, g2, b2 = color2
        return abs(r1 - r2) + abs(g1 - g2) + abs(b1 - b2)

    def __mergeColors (m):
        if m.image_array is None or m.color_to_coords is None:
            return
        color_list = list(m.color_to_coords.keys())
        initial_count = len(color_list)
        print ("*** Starting merge: %d colors -> %d colors" %(initial_count, m.num_colors))
        print ("*** Creating distance table for %d colors" %(initial_count), end='\r')
        sys.stdout.flush()
        distance_table = {}
        for i in range(initial_count):
            for j in range(i + 1, initial_count):
                color1 = color_list[i]
                color2 = color_list[j]
                distance = m.__colorDistance(color1, color2)
                if color1 < color2:
                    distance_table[(color1, color2)] = distance
                else:
                    distance_table[(color2, color1)] = distance
        print ("")
        iteration = 0
        while len(color_list) > m.num_colors:
            if len(distance_table) == 0:
                break
            min_distance = min(distance_table.values())
            closest_pair = None
            for pair, dist in distance_table.items():
                if dist == min_distance:
                    closest_pair = pair
                    break
            if closest_pair is None:
                break
            color1, color2 = closest_pair
            count1 = len(m.color_to_coords[color1])
            count2 = len(m.color_to_coords[color2])
            if count1 < count2:
                merge_from, merge_to = color1, color2
            elif count2 < count1:
                merge_from, merge_to = color2, color1
            else:
                if random.random() < 0.5:
                    merge_from, merge_to = color1, color2
                else:
                    merge_from, merge_to = color2, color1
            keys_to_remove = []
            for pair in distance_table.keys():
                if merge_from in pair:
                    keys_to_remove.append(pair)
            for key in keys_to_remove:
                del distance_table[key]
            m.color_to_coords[merge_to].extend(m.color_to_coords[merge_from])
            del m.color_to_coords[merge_from]
            color_list.remove(merge_from)
            iteration += 1
            remaining = len(color_list)
            print ("*** Merging iteration %d: %d colors remaining" %(iteration, remaining), end='\r')
            sys.stdout.flush()
        print ("")
        final_colors = list(m.color_to_coords.keys())
        print ("*** Merge complete: %d colors" %(len(final_colors)))
        m.setPalette(final_colors)
        m.__repaintImage()
        with open(m.save_path, "w") as ptr:
            for color, coords in m.color_to_coords.items():
                ptr.write("%s\n" %(":".join([str(c) for c in color])))

    def __repaintImage (m):
        if m.image_array is None or m.color_to_coords is None:
            return
        height, width = m.image_array.shape[:2]
        repainted = np.zeros((height, width, 3), dtype=np.uint8)
        palette_colors = list(m.color_to_coords.keys())
        for y in range(height):
            for x in range(width):
                original_color = tuple(m.image_array[y, x])
                closest_color = None
                min_dist = float('inf')
                for palette_color in palette_colors:
                    dist = m.__colorDistance(original_color, palette_color)
                    if dist < min_dist:
                        min_dist = dist
                        closest_color = palette_color
                if closest_color is not None:
                    repainted[y, x] = closest_color
        m.image_array = repainted
        m.__displayImage()
        img = Image.fromarray(repainted, mode='RGB')
        img.save(m.palette_img_save)

    def __displayImage (m):
        m.image_canvas.delete("all")
        if m.image_array is not None:
            img = Image.fromarray(m.image_array, mode='RGB')
            photo = ImageTk.PhotoImage(img)
            canvas_width = m.image_canvas.winfo_width()
            canvas_height = m.image_canvas.winfo_height()
            if canvas_width <= 1:
                canvas_width = m.image_canvas.winfo_reqwidth()
            if canvas_height <= 1:
                canvas_height = m.image_canvas.winfo_reqheight()
            center_x = canvas_width // 2
            center_y = canvas_height // 2
            m.image_canvas.create_image(center_x, center_y, anchor=tk.CENTER, image=photo, tags="image")
            m.image_canvas.image = photo

    def __updatePaletteDisplay (m):
        for i, canvas in enumerate(m.palette_canvases):
            canvas.delete("all")
            if i < len(m.palette_colors):
                color = m.palette_colors[i]
                color_hex = "#%02x%02x%02x" % color
                canvas.config(bg=color_hex)

    def __init__ (m, root, image_path, num_colors):
        m.root = root
        m.image_path = image_path
        if image_path.endswith(".png"):
            m.save_path = image_path.replace(".png", "_palette.txt")
            m.palette_img_save = image_path.replace(".png", "_palette.png")
        elif image_path.endswith(".ppm"):
            m.save_path = image_path.replace(".ppm", "_palette.txt")
            m.palette_img_save = image_path.replace(".ppm", "_palette.ppm")
        else:
            raise ValueError("Unsupported image format: %s" %(image_path))
        m.save_path = image_path.replace(".png", "_palette.txt")
        m.palette_img_save = image_path.replace(".png", "_palette.png")
        m.num_colors = num_colors
        m.palette_colors = [(0, 0, 0)] * num_colors
        
        m.palette_frame = tk.Frame(root)
        m.palette_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.palette_canvases = []
        color_canvas_size = 80
        for i in range(num_colors):
            canvas = tk.Canvas(m.palette_frame, width=color_canvas_size, height=color_canvas_size, bg="gray", highlightthickness=1, highlightbackground="black")
            canvas.pack(side=tk.LEFT, padx=2)
            m.palette_canvases.append(canvas)
        
        m.image_frame = tk.Frame(root)
        m.image_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.image_canvas = tk.Canvas(m.image_frame, bg="#f0f0f0")
        m.image_canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        m.image_canvas.bind("<Configure>", lambda e: m.__displayImage())
        
        m.__loadImage()
        m.__updatePaletteDisplay()

    def setPalette (m, colors):
        m.palette_colors = colors[:m.num_colors]
        if len(m.palette_colors) < m.num_colors:
            m.palette_colors.extend([(0, 0, 0)] * (m.num_colors - len(m.palette_colors)))
        m.__updatePaletteDisplay()

    def getPalette (m):
        return m.palette_colors.copy()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print ("*** Error: Usage: %s <image_path>" %(sys.argv[0]))
        sys.exit(1)
    image_path = sys.argv[1]
    
    root = tk.Tk()
    root.title("Palette Maker")
    num_colors = 8
    maker = PaletteMaker(root, image_path, num_colors)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
