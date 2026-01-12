#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from PIL import Image, ImageTk, ImageDraw
import tkinter as tk
import random
import math

#================================================================================================================================#
#=> - Class: TreePlacementPatternGenerator -
#================================================================================================================================#

class TreePlacementPatternGenerator:

    TREE_COLOR1_RGB = (3, 31, 22)  # (9, 61, 45)
    TREE_COLOR2_RGB = (9, 61, 45) # (0, 102, 51)
    TREE_PX_WIDTH = 5
    TREE_PX_HEIGHT = 10
    TREE_LEFT1_BRIGHTNESS_FACTOR = 1.3
    TREE_LEFT2_BRIGHTNESS_FACTOR = 2.0
    TREE_RIGHT1_BRIGHTNESS_FACTOR = 0.7
    TREE_RIGHT2_BRIGHTNESS_FACTOR = 0.3
    TREE_SHADOW_ALPHA = 120
    TREE_SHADOW_ANGLE = 30

    MAIN_TREE_COUNT = 300
    TIER_2_TREE_COUNT = 20
    TIER_3_TREE_COUNT = 10

    def __getRandomTreeColor(m):
        t = random.uniform(0.0, 1.0)
        r1, g1, b1 = m.TREE_COLOR1_RGB
        r2, g2, b2 = m.TREE_COLOR2_RGB
        r = int(r1 + (r2 - r1) * t)
        g = int(g1 + (g2 - g1) * t)
        b = int(b1 + (b2 - b1) * t)
        gb_ratio1 = float(g1) / float(b1) if b1 > 0 else 2.0
        gb_ratio2 = float(g2) / float(b2) if b2 > 0 else 2.0
        target_ratio = gb_ratio1 + (gb_ratio2 - gb_ratio1) * t
        if b > 0 and g <= b:
            g = int(b * target_ratio)
        return (r, g, b)
    
    def __generateTreeDecal(m, color):
        width = m.TREE_PX_WIDTH + random.randint(-1, 1)
        height = m.TREE_PX_HEIGHT + random.randint(-1, 1)
        decal = Image.new('RGBA', (width, height), (0, 0, 0, 0))
        pixels = decal.load()
        for y in range(height):
            y_factor = (y / float(height - 1)) if height > 1 else 0.0
            for x in range(width):
                if x == 0 or x == width - 1:
                    top_alpha = 255.0
                else:
                    center_x = (width - 1) / 2.0
                    dist_from_center = abs(x - center_x)
                    max_dist = center_x if width > 1 else 1.0
                    center_weight = dist_from_center / max_dist if max_dist > 0 else 0.0
                    top_alpha = 255.0 * center_weight + 127.0 * (1.0 - center_weight)
                alpha = int(top_alpha * y_factor)
                alpha += random.randint(-8, 8)
                alpha = max(0, min(255, alpha))
                alpha = 255 - alpha
                r, g, b = color[0], color[1], color[2]
                if width == 1:
                    factor = m.TREE_LEFT1_BRIGHTNESS_FACTOR
                elif width == 2:
                    factor = m.TREE_LEFT1_BRIGHTNESS_FACTOR if x == 0 else m.TREE_RIGHT2_BRIGHTNESS_FACTOR
                elif width == 3:
                    if x == 0:
                        factor = m.TREE_LEFT1_BRIGHTNESS_FACTOR
                    elif x == 1:
                        factor = (m.TREE_LEFT2_BRIGHTNESS_FACTOR + m.TREE_RIGHT1_BRIGHTNESS_FACTOR) / 2.0
                    else:
                        factor = m.TREE_RIGHT2_BRIGHTNESS_FACTOR
                elif width == 4:
                    if x == 0:
                        factor = m.TREE_LEFT1_BRIGHTNESS_FACTOR
                    elif x == 1:
                        factor = m.TREE_LEFT2_BRIGHTNESS_FACTOR
                    elif x == 2:
                        factor = m.TREE_RIGHT1_BRIGHTNESS_FACTOR
                    else:
                        factor = m.TREE_RIGHT2_BRIGHTNESS_FACTOR
                else:
                    if x == 0:
                        factor = m.TREE_LEFT1_BRIGHTNESS_FACTOR
                    elif x == 1:
                        factor = m.TREE_LEFT2_BRIGHTNESS_FACTOR
                    elif x == width - 2:
                        factor = m.TREE_RIGHT1_BRIGHTNESS_FACTOR
                    elif x == width - 1:
                        factor = m.TREE_RIGHT2_BRIGHTNESS_FACTOR
                    else:
                        t = float(x - 1) / float(width - 3) if width > 3 else 0.0
                        factor = m.TREE_LEFT2_BRIGHTNESS_FACTOR + (m.TREE_RIGHT1_BRIGHTNESS_FACTOR - m.TREE_LEFT2_BRIGHTNESS_FACTOR) * t
                r = int(r * factor)
                g = int(g * factor)
                b = int(b * factor)
                r = min(255, max(0, r))
                g = min(255, max(0, g))
                b = min(255, max(0, b))
                pixels[x, y] = (r, g, b, alpha)
        decal = decal.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        return decal
    
    def __generateTreeShadow(m, tree_height):
        angle_rad = math.radians(m.TREE_SHADOW_ANGLE)
        shadow_length = tree_height * 1.2
        shadow_width = max(1, tree_height * 0.6 - 2)
        shadow_w = int(shadow_length + shadow_width * 2)
        shadow_h = int(shadow_length + shadow_width * 2)
        if shadow_w < 1 or shadow_h < 1:
            return None
        shadow = Image.new('RGBA', (shadow_w, shadow_h), (0, 0, 0, 0))
        draw = ImageDraw.Draw(shadow)
        center_x = shadow_w / 2.0
        center_y = shadow_h / 2.0
        bbox = [center_x - shadow_length/2, center_y - shadow_width/2, center_x + shadow_length/2, center_y + shadow_width/2]
        draw.ellipse(bbox, fill=(0, 0, 0, m.TREE_SHADOW_ALPHA))
        shadow = shadow.rotate(m.TREE_SHADOW_ANGLE, expand=True, fillcolor=(0, 0, 0, 0))
        return shadow

    def __generateTreeLocations(m, tree_count, height, width, center_x, center_y):
        inner_h = height
        inner_w = width
        outer_h = int(height * 1.5)
        outer_w = int(width * 1.5)
        locations = []
        for i in range(int(tree_count * 0.8)):
            while True:
                angle = random.uniform(0, 2 * math.pi)
                r_factor = random.uniform(0, 1)
                r_factor = math.sqrt(r_factor)
                edge_thin = random.uniform(0, 1)
                if r_factor > 0.85 and edge_thin < 0.3:
                    continue
                x_offset = r_factor * (inner_w / 2) * math.cos(angle)
                y_offset = r_factor * (inner_h / 2) * math.sin(angle)
                x = int(center_x + x_offset)
                y = int(center_y + y_offset)
                if x >= 0 and x < m.canvas_size and y >= 0 and y < m.canvas_size:
                    locations.append((x, y))
                    break
        for i in range(int(tree_count * 0.2)):
            while True:
                angle = random.uniform(0, 2 * math.pi)
                r_inner = 0.67
                r_outer = 1.0
                r_factor = random.uniform(r_inner, r_outer)
                edge_thin = random.uniform(0, 1)
                if r_factor > 0.95 and edge_thin < 0.2:
                    continue
                x_offset = r_factor * (outer_w / 2) * math.cos(angle)
                y_offset = r_factor * (outer_h / 2) * math.sin(angle)
                dist_inner = math.sqrt((x_offset / (inner_w / 2)) ** 2 + (y_offset / (inner_h / 2)) ** 2)
                if dist_inner <= 1.0:
                    continue
                x = int(center_x + x_offset)
                y = int(center_y + y_offset)
                if x >= 0 and x < m.canvas_size and y >= 0 and y < m.canvas_size:
                    locations.append((x, y))
                    break
        return locations

    def __save(m):
        if not m.output_path:
            print ("*** Error: No output path set")
            return
        if not hasattr(m, 'trees_list') or not hasattr(m, 'shadows_list'):
            print ("*** Error: No trees generated yet")
            return
        final_decal = Image.new('RGBA', (m.canvas_size, m.canvas_size), (0, 0, 0, 0))
        final_pixels = final_decal.load()
        shadow_mask = {}
        for shadow, x, y in m.shadows_list:
            shadow_width, shadow_height = shadow.size
            paste_x = x - shadow_width // 2
            paste_y = y - shadow_height // 2
            shadow_pixels = shadow.load()
            for sy in range(shadow_height):
                for sx in range(shadow_width):
                    px = paste_x + sx
                    py = paste_y + sy
                    if px < 0 or py < 0 or px >= m.canvas_size or py >= m.canvas_size:
                        continue
                    sr, sg, sb, sa = shadow_pixels[sx, sy]
                    if sa > 0:
                        mask_key = (px, py)
                        existing_shadow_alpha = shadow_mask.get(mask_key, 0)
                        if sa > existing_shadow_alpha:
                            final_pixels[px, py] = (sr, sg, sb, sa)
                            shadow_mask[mask_key] = sa
        sorted_trees = sorted(m.trees_list, key=lambda item: item[2])
        for decal, x, y in sorted_trees:
            decal_width, decal_height = decal.size
            paste_x = x - decal_width // 2
            paste_y = y - decal_height // 2
            decal_pixels = decal.load()
            for dy in range(decal_height):
                for dx in range(decal_width):
                    px = paste_x + dx
                    py = paste_y + dy
                    if px < 0 or py < 0 or px >= m.canvas_size or py >= m.canvas_size:
                        continue
                    tr, tg, tb, ta = decal_pixels[dx, dy]
                    if ta > 0:
                        existing_r, existing_g, existing_b, existing_a = final_pixels[px, py]
                        if existing_a == 0:
                            final_pixels[px, py] = (tr, tg, tb, ta)
                        else:
                            src_alpha = float(ta) / 255.0
                            dst_alpha = float(existing_a) / 255.0
                            result_alpha = src_alpha + dst_alpha * (1.0 - src_alpha)
                            if result_alpha > 0:
                                new_r = int((tr * src_alpha + existing_r * dst_alpha * (1.0 - src_alpha)) / result_alpha)
                                new_g = int((tg * src_alpha + existing_g * dst_alpha * (1.0 - src_alpha)) / result_alpha)
                                new_b = int((tb * src_alpha + existing_b * dst_alpha * (1.0 - src_alpha)) / result_alpha)
                                new_a = int(result_alpha * 255.0)
                                final_pixels[px, py] = (new_r, new_g, new_b, new_a)
        bbox = final_decal.getbbox()
        if bbox:
            final_decal = final_decal.crop(bbox)
        final_decal.save(m.output_path)
        print ("*** Saved decal to %s" %(m.output_path))

    def __init__(m, canvas_size=600, height=100, width=50, output_path=None):
        m.canvas_size = canvas_size
        m.height = height
        m.width = width
        m.background_texture = None
        m.left_texture_image = None
        m.right_marker_image = Image.new('RGB', (canvas_size, canvas_size), (255, 255, 255))
        m.output_path = output_path
        m.trees_list = []
        m.shadows_list = []
        m.root = tk.Tk()
        m.root.title("Tree Placement Pattern Generator")
        button_frame = tk.Frame(m.root)
        button_frame.pack(side=tk.TOP, pady=5)
        regenerate_btn = tk.Button(button_frame, text="Regenerate", command=m.regenerate)
        regenerate_btn.pack(side=tk.LEFT, padx=5)
        save_btn = tk.Button(button_frame, text="Save", command=m.__save)
        save_btn.pack(side=tk.LEFT, padx=5)
        m.left_canvas = tk.Canvas(m.root, width=canvas_size, height=canvas_size, bg="white")
        m.right_canvas = tk.Canvas(m.root, width=canvas_size, height=canvas_size, bg="white")
        m.left_canvas.pack(side=tk.LEFT, padx=5, pady=5)
        m.right_canvas.pack(side=tk.LEFT, padx=5, pady=5)
        m.left_image_id = None
        m.right_image_id = None
        
    def loadBackgroundTexture(m, texture_path):
        if not os.path.exists(texture_path):
            print ("*** Error: Texture file not found: %s" %(texture_path))
            return False
        img = Image.open(texture_path)
        m.background_texture = img.convert('RGB')
        print ("*** Loaded background texture: %s" %(texture_path))

    def createCanvases(m):
        if m.background_texture:
            texture_cropped = m.background_texture.copy()
            if texture_cropped.width > m.canvas_size or texture_cropped.height > m.canvas_size:
                left = (texture_cropped.width - m.canvas_size) // 2
                top = (texture_cropped.height - m.canvas_size) // 2
                right = left + m.canvas_size
                bottom = top + m.canvas_size
                texture_cropped = texture_cropped.crop((left, top, right, bottom))
            if texture_cropped.width < m.canvas_size or texture_cropped.height < m.canvas_size:
                texture_cropped = texture_cropped.resize((m.canvas_size, m.canvas_size), Image.Resampling.LANCZOS)
            m.left_texture_image = texture_cropped.copy().convert('RGBA')
            photo = ImageTk.PhotoImage(texture_cropped)
            m.left_image_id = m.left_canvas.create_image(0, 0, anchor=tk.NW, image=photo)
            m.left_canvas.image = photo
            print ("*** Created left canvas with background texture")
        else:
            m.left_texture_image = Image.new('RGBA', (m.canvas_size, m.canvas_size), (255, 255, 255, 255))
            print ("*** Warning: No background texture loaded, left canvas will be white")
        m.shadow_mask = {}
        photo_right = ImageTk.PhotoImage(m.right_marker_image)
        m.right_image_id = m.right_canvas.create_image(0, 0, anchor=tk.NW, image=photo_right)
        m.right_canvas.image = photo_right
        print ("*** Created right canvas (white) for tree placement markers")
        m.generateTreesAndShadows()
    
    def generateTreesAndShadows(m):
        m.right_marker_image = Image.new('RGB', (m.canvas_size, m.canvas_size), (255, 255, 255))
        if m.background_texture:
            texture_cropped = m.background_texture.copy()
            if texture_cropped.width > m.canvas_size or texture_cropped.height > m.canvas_size:
                left = (texture_cropped.width - m.canvas_size) // 2
                top = (texture_cropped.height - m.canvas_size) // 2
                right = left + m.canvas_size
                bottom = top + m.canvas_size
                texture_cropped = texture_cropped.crop((left, top, right, bottom))
            if texture_cropped.width < m.canvas_size or texture_cropped.height < m.canvas_size:
                texture_cropped = texture_cropped.resize((m.canvas_size, m.canvas_size), Image.Resampling.LANCZOS)
            m.left_texture_image = texture_cropped.copy().convert('RGBA')
        else:
            m.left_texture_image = Image.new('RGBA', (m.canvas_size, m.canvas_size), (255, 255, 255, 255))
        m.shadow_mask = {}
        random.seed()
        center_x = m.canvas_size // 2
        center_y = m.canvas_size // 2
        all_locations = []
        main_locations = m.__generateTreeLocations(m.MAIN_TREE_COUNT, m.height, m.width, center_x, center_y)
        all_locations.extend(main_locations)
        tier2_h = int(m.height * 0.6)
        tier2_w = int(m.width * 0.6)
        for i in range(5):
            offset_angle = random.uniform(0, 2 * math.pi)
            offset_r_factor = random.uniform(1.7, 1.8)
            offset_x = offset_r_factor * (m.width / 2) * math.cos(offset_angle)
            offset_y = offset_r_factor * (m.height / 2) * math.sin(offset_angle)
            tier2_center_x = int(center_x + offset_x)
            tier2_center_y = int(center_y + offset_y)
            tier2_locations = m.__generateTreeLocations(m.TIER_2_TREE_COUNT, tier2_h, tier2_w, tier2_center_x, tier2_center_y)
            all_locations.extend(tier2_locations)
        all_locations.sort(key=lambda loc: loc[1])
        shadows_list = []
        trees_list = []
        for x, y in all_locations:
            m.markTreeLocation(x, y)
            color = m.__getRandomTreeColor()
            decal = m.__generateTreeDecal(color)
            shadow = m.__generateTreeShadow(decal.height)
            if shadow:
                shadows_list.append((shadow, x + 4, y))
            trees_list.append((decal, x, y))
        m.trees_list = trees_list
        m.shadows_list = shadows_list
        for shadow, x, y in shadows_list:
            m.overlayShadow(shadow, x, y)
        for decal, x, y in trees_list:
            m.overlayDecal(decal, x, y)
        m.updateRightCanvas()
        m.updateLeftCanvas()
    
    def regenerate(m):
        m.generateTreesAndShadows()
    
    def overlayShadow(m, shadow, x, y):
        if not m.left_texture_image:
            return False
        shadow_width, shadow_height = shadow.size
        paste_x = x - shadow_width // 2
        paste_y = y - shadow_height // 2
        if paste_x < 0 or paste_y < 0 or paste_x + shadow_width > m.canvas_size or paste_y + shadow_height > m.canvas_size:
            return False
        region = m.left_texture_image.crop((paste_x, paste_y, paste_x + shadow_width, paste_y + shadow_height))
        shadow_pixels = shadow.load()
        region_pixels = region.load()
        for sy in range(shadow_height):
            for sx in range(shadow_width):
                sr, sg, sb, sa = shadow_pixels[sx, sy]
                if sa > 0:
                    px = paste_x + sx
                    py = paste_y + sy
                    mask_key = (px, py)
                    existing_shadow_alpha = m.shadow_mask.get(mask_key, 0)
                    if sa > existing_shadow_alpha:
                        rr, rg, rb, ra = region_pixels[sx, sy]
                        blend_alpha = float(sa) / 255.0
                        new_r = int(rr * (1.0 - blend_alpha) + sr * blend_alpha)
                        new_g = int(rg * (1.0 - blend_alpha) + sg * blend_alpha)
                        new_b = int(rb * (1.0 - blend_alpha) + sb * blend_alpha)
                        region_pixels[sx, sy] = (new_r, new_g, new_b, ra)
                        m.shadow_mask[mask_key] = sa
        m.left_texture_image.paste(region, (paste_x, paste_y))
        return True
    
    def overlayDecal(m, decal, x, y):
        if not m.left_texture_image:
            return False
        decal_width, decal_height = decal.size
        paste_x = x - decal_width // 2
        paste_y = y - decal_height // 2
        if paste_x < 0 or paste_y < 0 or paste_x + decal_width > m.canvas_size or paste_y + decal_height > m.canvas_size:
            return False
        m.left_texture_image.paste(decal, (paste_x, paste_y), decal)
        return True
    
    def updateLeftCanvas(m):
        if not m.left_texture_image:
            return
        rgb_image = m.left_texture_image.convert('RGB')
        photo = ImageTk.PhotoImage(rgb_image)
        m.left_canvas.delete(m.left_image_id)
        m.left_image_id = m.left_canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.left_canvas.image = photo
    
    def updateRightCanvas(m):
        photo_right = ImageTk.PhotoImage(m.right_marker_image)
        m.right_canvas.delete(m.right_image_id)
        m.right_image_id = m.right_canvas.create_image(0, 0, anchor=tk.NW, image=photo_right)
        m.right_canvas.image = photo_right
        
    def markTreeLocation(m, x, y, dot_size=2):
        if not m.right_canvas:
            print ("*** Error: Canvases not created yet")
            return False
        m.right_canvas.create_oval(x - dot_size, y - dot_size, x + dot_size, y + dot_size, fill="black", outline="black")
        draw = ImageDraw.Draw(m.right_marker_image)
        draw.ellipse([x - dot_size, y - dot_size, x + dot_size, y + dot_size], fill=(0, 0, 0))
        return True
    
    def run(m):
        m.root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    width, height = 100, 50
    texture_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.png"
    output_path = "/home/w/Projects/img-content/texture-grassland3-pine.png"
    generator = TreePlacementPatternGenerator(canvas_size=600, height=height, width=width, output_path=output_path)
    generator.loadBackgroundTexture(texture_path)
    generator.createCanvases()
    generator.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
