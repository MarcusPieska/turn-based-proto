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
#=> - Class: MtnTexGenerator
#================================================================================================================================#

class MtnTexGenerator:

    def __color_over_if_margins (m, old_color, new_color, margin):
        v1, v2, v3 = m.triangle
        pixels_to_change = []
        for y in range(m.height):
            for x in range(m.width):
                if not m.__pointInTriangle(x, y, v1, v2, v3) or tuple(m.image_matrix[y, x]) != old_color:
                    continue
                all_old_color = True
                for dy in range(-margin, margin + 1):
                    for dx in range(-margin, margin + 1):
                        if dx * dx + dy * dy > margin * margin:
                            continue
                        check_x = x + dx
                        check_y = y + dy
                        if 0 <= check_x < m.width and 0 <= check_y < m.height:
                            if m.__pointInTriangle(check_x, check_y, v1, v2, v3):
                                if tuple(m.image_matrix[check_y, check_x]) != old_color:
                                    all_old_color = False
                                    break
                    if not all_old_color: break
                if all_old_color:
                    pixels_to_change.append((x, y))
        for x, y in pixels_to_change:
            m.image_matrix[y, x] = new_color
        return pixels_to_change

    def __makeBaseTexture (m):
        m.image_matrix = np.zeros((m.height, m.width, 3), dtype=np.uint8)
        v1 = (0, m.height)
        v2 = (m.width, m.height)
        v3 = (m.width // 2, 0)
        m.triangle = (v1, v2, v3)
        for y in range(m.height):
            for x in range(m.width):
                if m.__pointInTriangle(x, y, v1, v2, v3):
                    m.image_matrix[y, x] = m.color_base

    def __adjustColor (m, color, factor):
        return tuple(max(0, min(255, int(c * factor))) for c in color)

    def __pointInTriangle (m, x, y, v1, v2, v3):
        def sign(p1, p2, p3):
            return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1])
        
        d1 = sign((x, y), v1, v2)
        d2 = sign((x, y), v2, v3)
        d3 = sign((x, y), v3, v1)
        
        has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
        has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
        
        return not (has_neg and has_pos)

    def __restore_pixels_to_base (m, pxs):
        for px in pxs:
            m.image_matrix[px[1], px[0]] = m.color_base

    def __init__ (m, size=200, color_base=(100, 100, 100), darken_factor=1.0):
        m.size = size
        m.pxh_w = 15
        m.pxs_w = 10
        m.pxd_m = 5
        m.color_base = color_base
        m.height = int(size * math.sqrt(3) / 2)
        m.width = size
        m.triangle = None
        m.pxs_highlight, m.pxs_dark_stripes, m.pxs_darkest_stripes = [], [], []
        m.color_darker = m.__adjustColor (m.color_base, 0.7*darken_factor)
        m.color_darkest = m.__adjustColor (m.color_base, 0.6*darken_factor)
        m.color_bright = m.__adjustColor (m.color_base, 1.3*darken_factor)
        m.color_snow_blue = (240*darken_factor, 245*darken_factor, 250*darken_factor)
        m.color_snow_whitesmoke = (200*darken_factor, 200*darken_factor, 200*darken_factor)

        m.stipes_bottom_xpx = list(range(0, m.size, 10))
        m.stipes_top_xpx = [x + 5 for x in m.stipes_bottom_xpx]
        m.stipes_bottom_xpx = m.stipes_bottom_xpx[1:-1]
        
        m.__makeBaseTexture ()

    def reset (m):
        m.__makeBaseTexture ()
        m.pxs_highlight, m.pxs_dark_stripes, m.pxs_darkest_stripes = [], [], []

    def drawHighlights (m):
        m.__restore_pixels_to_base (m.pxs_highlight)
        m.pxs_highlight = []

        widths, width_adds = [], []
        current_width = m.pxh_w
        for y in range(m.height):
            if random.randint(0, 99) < 5 and current_width > 1:
                current_width -= 1
            if random.randint(0, 99) < 10:
                width_adds.extend([1, 1, 1, 1])
            widths.append(current_width if not width_adds else current_width + width_adds.pop(0))
        v1, v2, v3 = m.triangle
        for y in range(m.height):
            left_x = int((m.width // 2) * (m.height - y) / m.height)
            right_x = int(m.width - (m.width // 2) * (m.height - y) / m.height)
            w = widths[y]
            for offset in range(-w//2, w//2 + 1):
                x_left = left_x + offset
                x_right = right_x + offset
                if 0 <= x_left < m.width and m.__pointInTriangle(x_left, y, v1, v2, v3):
                    m.pxs_highlight.append((x_left, y))
                    m.image_matrix[y, x_left] = m.color_bright
                if 0 <= x_right < m.width and m.__pointInTriangle(x_right, y, v1, v2, v3):
                    m.pxs_highlight.append((x_right, y))
                    m.image_matrix[y, x_right] = m.color_bright
        
        for start_x in m.stipes_top_xpx:
            current_width = m.pxs_w * 2
            current_x = start_x
            shift_dir = 1
            for y in range(m.height):
                if current_width == 0:
                    break
                if random.randint(0, 99) < 20:
                    current_width -= 1
                    if current_width < 0:
                        break
                if random.randint(0, 99) < 10:
                    current_x += shift_dir
                    shift_dir = -shift_dir
                for offset in range(-current_width//2, current_width//2 + 1):
                    x = current_x + offset
                    if 0 <= x < m.width and m.__pointInTriangle(x, y, v1, v2, v3):
                        m.pxs_highlight.append((x, y))
                        m.image_matrix[y, x] = m.color_bright

    def drawDarkStripes (m):
        m.__restore_pixels_to_base (m.pxs_dark_stripes)
        m.pxs_dark_stripes = []
        
        v1, v2, v3 = m.triangle
        for start_x in m.stipes_bottom_xpx:
            if random.randint(0, 99) < 20:
                continue
            current_width = m.pxs_w
            current_x = start_x
            shift_dir = 1
            found_highlight = False
            for y in range(m.height - 1, -1, -1):
                if current_width == 0:
                    break
                look_up_y = y - current_width * 2
                if look_up_y >= 0 and not found_highlight:
                    if 0 <= current_x < m.width and m.__pointInTriangle(current_x, look_up_y, v1, v2, v3):
                        if tuple(m.image_matrix[look_up_y, current_x]) == m.color_bright:
                            found_highlight = True
                if found_highlight:
                    current_width -= 1
                    if current_width <= 0:
                        break
                else:
                    if random.randint(0, 99) < 15:
                        current_width -= 1
                        if current_width < 0:
                            break
                if random.randint(0, 99) < 10:
                    current_x += shift_dir
                    shift_dir = -shift_dir
                for offset in range(-current_width//2, current_width//2 + 1):
                    x = current_x + offset
                    if 0 <= x < m.width and m.__pointInTriangle(x, y, v1, v2, v3):
                        m.pxs_dark_stripes.append((x, y))
                        m.image_matrix[y, x] = m.color_darker

    def drawDarkestStripes (m):
        m.__restore_pixels_to_base (m.pxs_darkest_stripes)
        m.pxs_darkest_stripes = m.__color_over_if_margins (m.color_base, m.color_darkest, m.pxd_m)
        m.pxs_darkest_stripes = m.__color_over_if_margins (m.color_bright, m.color_snow_blue, m.pxd_m * 2)

    def getUpdatedTexture (m):
        return m.image_matrix, m.triangle

#================================================================================================================================#
#=> - Class: TexDisplayer
#================================================================================================================================#

class TexDisplayer:

    def __init__ (m, root, size=200, color_base=(100, 100, 100)):
        m.root = root
        m.generator = MtnTexGenerator(size=size, color_base=color_base)
        m.texture_image = None
        m.triangle = None
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.reset_btn = tk.Button(m.top_frame, text="Reset", command=m.__reset, width=15)
        m.reset_btn.pack(side=tk.LEFT, padx=5)

        m.highlights_btn = tk.Button(m.top_frame, text="Make highlights", command=m.__makeHighlights, width=15)
        m.highlights_btn.pack(side=tk.LEFT, padx=5)
        
        m.dark_stripes_btn = tk.Button(m.top_frame, text="Make dark stripes", command=m.__makeDarkStripes, width=15)
        m.dark_stripes_btn.pack(side=tk.LEFT, padx=5)
        
        m.darkest_stripes_btn = tk.Button(m.top_frame, text="Make darkest stripes", command=m.__makeDarkestStripes, width=15)
        m.darkest_stripes_btn.pack(side=tk.LEFT, padx=5)

        m.blur_btn = tk.Button(m.top_frame, text="Blur texture", command=m.__blurTexture, width=15)
        m.blur_btn.pack(side=tk.LEFT, padx=5)

        m.save_btn = tk.Button(m.top_frame, text="Save texture", command=m.__saveTexture, width=15)
        m.save_btn.pack(side=tk.LEFT, padx=5)
        
        m.seed = 0
        random.seed(m.seed)
        m.set_seed_btn = tk.Button(m.top_frame, text="Set seed", command=m.__setSeed, width=15)
        m.set_seed_btn.pack(side=tk.LEFT, padx=5)
        m.seed_entry = tk.Entry(m.top_frame, width=10)
        m.seed_entry.pack(side=tk.LEFT, padx=5)
        m.seed_entry.insert(0, str(m.seed))

        m.canvas = tk.Canvas(root, width=400, height=400, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        

        m.__displayTexture()

    def __displayTexture (m):
        m.texture_image, m.triangle = m.generator.getUpdatedTexture()
        m.canvas.delete("all")
        if m.texture_image is not None:
            img = Image.fromarray(m.texture_image, mode='RGB')
            photo = ImageTk.PhotoImage(img)
            m.canvas.create_image(200, 200, anchor=tk.CENTER, image=photo)
            m.canvas.image = photo

    def __reset (m):
        random.seed(m.seed)
        m.generator.reset()
        m.__displayTexture()

    def __makeHighlights (m):
        m.generator.drawHighlights()
        m.__displayTexture()

    def __makeDarkStripes (m):
        m.generator.drawDarkStripes()
        m.__displayTexture()

    def __makeDarkestStripes (m):
        m.generator.drawDarkestStripes()
        m.__displayTexture()

    def __setSeed (m):
        try:
            seed = int(m.seed_entry.get())
            random.seed(seed)
            print("*** Seed set to: ", seed)
            m.seed = seed
        except ValueError:
            print("*** Invalid seed")

    def __blurTexture (m):
        m.texture_image, m.triangle = m.generator.getUpdatedTexture()
        if m.texture_image is not None:
            black_pixels = np.ones_like(m.texture_image) * 255
            black_mask = np.all(m.texture_image == [0, 0, 0], axis=2)
            black_pixels[black_mask] = [0, 0, 0]
            img = Image.fromarray(m.texture_image, mode='RGB')
            blurred_img = img.filter(ImageFilter.GaussianBlur(radius=1.5))
            blurred_array = np.array(blurred_img)
            blurred_array[black_mask] = [0, 0, 0]
            m.generator.image_matrix = blurred_array
            m.__displayTexture()

    def __saveTexture (m):
        converted = cv2.cvtColor(m.texture_image, cv2.COLOR_RGB2BGR)
        c = m.generator.color_base
        cv2.imwrite("../../DEL_temp_mtn_tex_%d_%d_%d_%d.png" %(m.seed, c[0], c[1], c[2]), converted)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    gray = (100, 100, 100)
    medium_earthy_brown = (120, 90, 70)
    darker_rocky_brown = (101, 75, 60)
    lighter_tan_brown = (140, 110, 85)
    rich_brown = (139, 90, 43)

    all_colors = [gray, medium_earthy_brown, darker_rocky_brown, lighter_tan_brown, rich_brown]
    for color in all_colors:
        root = tk.Tk()
        root.title("Mountain Texture Generator")
        displayer = TexDisplayer(root, size=200, color_base=color)
        root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
