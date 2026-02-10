#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from PIL import Image, ImageTk, ImageDraw
import random
import math
import cv2
import numpy as np

#================================================================================================================================#
#=> - Notes -
#================================================================================================================================#
#
#  This is make 1 of our farm tile generator
#  Possible improvements:
#  - Use shader to alternative between two colors, and not just one color and two shades of that color
#  - Placement of colored pixels to represent flowering crops
#  - Placement of green shrubs on tilled rows to depict, e.g., potatoe plants
#  - Line tiles with trees and/or hedges
#  - Enable the 4 quadrants to be split at point other than the center
#  - Fundamentally different crops; like wines ranks, wheat fields, rice fields, etc.
#  - Large farm plots where 2 x 2 tiles assemble into a single farm plot
#
#  Many of the above will have to be done in the morpher, and not in the farm layout
#  The morpher will need to know where the tilling rows are to place crop plants
#
#================================================================================================================================#
#=> - Global Parameters -
#================================================================================================================================#

RAW_WIDTH = 102
RAW_HEIGHT = 102
TILE_WIDTH = 102
TILE_HEIGHT = 51
BUTTON_WIDTH = 12
ZOOM_FACTOR = 4
CANVAS_WIDTH = 500
CANVAS_HEIGHT = 500

TEST_MODE = True

LINE_SPACING = 12
LINE_START = LINE_SPACING // 2
LINE_THICKNESS = 4

SHADER_RANGE = 15

#================================================================================================================================#
#=> - Helper Functions -
#================================================================================================================================#

def get_avg_rgb (img):
    pixel_count = img.width * img.height
    total_r, total_g, total_b = 0, 0, 0
    for x in range(img.width):
        for y in range(img.height):
            r, g, b = img.getpixel((x, y))
            total_r += r
            total_g += g
            total_b += b
    return total_r // pixel_count, total_g // pixel_count, total_b // pixel_count

def offset_img_rgb (img, rgb_offsets):
    new_img = img.copy()
    for x in range(img.width):
        for y in range(img.height):
            r, g, b = img.getpixel((x, y))
            new_img.putpixel((x, y), (r + rgb_offsets[0], g + rgb_offsets[1], b + rgb_offsets[2]))
    return new_img

def optain_shader_images ():
    prefix, suffix = "farm_row_shader_", "px.png"
    shader_images_px, shader_images, px_indices = [], [], []
    for filename in os.listdir("."):
        if filename.startswith(prefix) and filename.endswith(suffix):
            number_str = filename[len(prefix):-len(suffix)]
            number = int(number_str)
            shader_images_px.append((number, filename))
    shader_images_px.sort(key=lambda x: x[0])
    for px, filename in shader_images_px:
        img = Image.open(filename).convert("L")
        shader_images.append(img)
        px_indices.append(px)
    return shader_images, px_indices

#================================================================================================================================#
#=> - Class: ColorSet -
#================================================================================================================================#

class ColorSet:

    def __init__ (m):
        m.color_1 = "#4a7c59"
        m.color_2 = "#5a8c69"
        m.color_3 = "#6a9c79"
        m.color_4 = "#7aac89"
        m.colors = [m.color_1, m.color_2, m.color_3, m.color_4]
    
    def shuffle_colors (m):
        random.shuffle(m.colors)

    def rotate_colors (m, steps=1):
        steps = steps % len(m.colors)
        m.colors = m.colors[steps:] + m.colors[:steps] 

    def get_colors (m):
        return m.colors

#================================================================================================================================#
#=> - Class: FarmLayout -
#================================================================================================================================#

class FarmLayout:

    def __init__ (m, frame, color_set):
        m.frame = frame
        m.color_set = color_set
        m.scale_factor = ZOOM_FACTOR
        m.layout_idx = 0
        m.layout_count = len(m.__get_layouts())
        m.rotation_angles = [0, 90, 180, 270]
        m.rotation_idx = 0
        m.tilling_orientations = [True, True, True, True]
        
        m.layout_img = Image.new("RGB", (CANVAS_WIDTH, CANVAS_HEIGHT), (255, 255, 255))
        
        m.farm_noise = Image.open("farm_noise.png").convert("RGB")
        m.farm_noise_rgb = get_avg_rgb(m.farm_noise)
        
        m.shader_images, m.shader_px_indices = optain_shader_images()
        m.shader_indices = [0] * 4
        
        cropped_width = m.scale_factor * RAW_WIDTH
        cropped_height = m.scale_factor * RAW_HEIGHT
        scaled_xo = (CANVAS_WIDTH - cropped_width) // 2
        scaled_yo = (CANVAS_HEIGHT - cropped_height) // 2
        
        noise_scaled = m.farm_noise.resize((cropped_width, cropped_height), Image.LANCZOS)
        noise_canvas = Image.new("RGB", (CANVAS_WIDTH, CANVAS_HEIGHT), (255, 255, 255))
        noise_canvas.paste(noise_scaled, (scaled_xo, scaled_yo))
        m.noise_array = np.array(noise_canvas)
        
        m.shader_scaled_list = []
        m.shader_scaled_rotated_list = []
        for shader_img in m.shader_images:
            shader_scaled = shader_img.resize((cropped_width, cropped_height), Image.LANCZOS)
            m.shader_scaled_list.append(shader_scaled)
            m.shader_scaled_rotated_list.append(shader_scaled.rotate(90, expand=True))
        
        m.button_row = tk.Frame(frame)
        m.button_row.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.btn_cycle = tk.Button(m.button_row, text="Cycle", command=lambda: m.__cycle_layout(), width=BUTTON_WIDTH)
        m.btn_cycle.pack(side=tk.LEFT, padx=5)
        
        m.btn_rotate = tk.Button(m.button_row, text="Rotate", command=lambda: m.__cycle_rotation(), width=BUTTON_WIDTH)
        m.btn_rotate.pack(side=tk.LEFT, padx=5)
        
        m.content_frame = tk.Frame(frame)
        m.content_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.left_button_frame = tk.Frame(m.content_frame)
        m.left_button_frame.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.tilling_row = tk.Frame(m.left_button_frame)
        m.tilling_row.pack(side=tk.TOP, padx=5, pady=5)
        
        m.tilling_buttons = []
        for i in range(4):
            btn = tk.Button(m.tilling_row, text="-", command=lambda idx=i: m.__button_callback(idx), width=2)
            btn.pack(side=tk.LEFT, padx=5)
            m.tilling_buttons.append(btn)
        
        m.effect_row = tk.Frame(m.left_button_frame)
        m.effect_row.pack(side=tk.TOP, padx=5, pady=5)
        
        m.shader_buttons = []
        for i in range(4):
            initial_text = str(m.shader_px_indices[m.shader_indices[i]]) if len(m.shader_px_indices) > 0 else "-"
            btn = tk.Button(m.effect_row, text=initial_text, command=lambda idx=i: m.__effect_callback(idx), width=2)
            btn.bind("<Button-3>", lambda e, idx=i: m.__effect_callback_reverse(idx))
            btn.pack(side=tk.LEFT, padx=5)
            m.shader_buttons.append(btn)
        
        m.canvas = tk.Canvas(m.content_frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white")
        m.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas.images = []
        
        m.__update_button_states()
        
        m.__generate_texture()
        m.__draw_layout()
        m.__update_canvas()

    def __generate_texture (m):
        m.texture_img = Image.new("RGB", (RAW_WIDTH, RAW_HEIGHT), (200, 200, 200))
        draw = ImageDraw.Draw(m.texture_img)
        
        for i in range(0, RAW_WIDTH, 10):
            for j in range(0, RAW_HEIGHT, 10):
                color = (random.randint(150, 255), random.randint(150, 255), random.randint(150, 255))
                draw.rectangle([i, j, i + 10, j + 10], fill=color)

    def __get_quadrant_coords (m):
        canvas_center_x = 250
        canvas_center_y = 250
        scaled_width = RAW_WIDTH * m.scale_factor
        scaled_height = RAW_HEIGHT * m.scale_factor
        
        x1 = canvas_center_x - scaled_width / 2
        y1 = canvas_center_y - scaled_height / 2
        x2 = canvas_center_x + scaled_width / 2
        y2 = canvas_center_y + scaled_height / 2
        mid_x = canvas_center_x
        mid_y = canvas_center_y
        
        top_left = [(x1, y1), (mid_x, y1), (mid_x, mid_y), (x1, mid_y)]
        top_right = [(mid_x, y1), (x2, y1), (x2, mid_y), (mid_x, mid_y)]
        bottom_left = [(x1, mid_y), (mid_x, mid_y), (mid_x, y2), (x1, y2)]
        bottom_right = [(mid_x, mid_y), (x2, mid_y), (x2, y2), (mid_x, y2)]
        
        return top_left, top_right, bottom_left, bottom_right

    def __get_layouts (m):
        top_left, top_right, bottom_left, bottom_right = m.__get_quadrant_coords()
        
        all_field = [(top_left[0][0], top_left[0][1]), (top_right[1][0], top_right[1][1]), 
                     (bottom_right[2][0], bottom_right[2][1]), (bottom_left[3][0], bottom_left[3][1])]
        
        layouts = [
            [all_field],
            [top_left, top_right, bottom_left, bottom_right],
            [[(top_left[0][0], top_left[0][1]), (top_right[1][0], top_right[1][1]), 
              (top_right[2][0], top_right[2][1]), (top_left[3][0], top_left[3][1])], 
             bottom_left, bottom_right],
            [[(top_left[0][0], top_left[0][1]), (top_right[1][0], top_right[1][1]), 
              (top_right[2][0], top_right[2][1]), (top_left[3][0], top_left[3][1])],
             [(bottom_left[0][0], bottom_left[0][1]), (bottom_right[1][0], bottom_right[1][1]), 
              (bottom_right[2][0], bottom_right[2][1]), (bottom_left[3][0], bottom_left[3][1])]],
            [[(top_left[0][0], top_left[0][1]), (top_right[1][0], top_right[1][1]), 
              (top_right[2][0], top_right[2][1]), (bottom_left[1][0], bottom_left[1][1]), 
              (bottom_left[2][0], bottom_left[2][1]), (bottom_left[3][0], bottom_left[3][1]), 
              (top_left[3][0], top_left[3][1])], 
             bottom_right]
        ]
        
        return layouts

    def __update_button_states (m):
        layouts = m.__get_layouts()
        num_layouts = len(layouts[m.layout_idx])
        for i, (btn1, btn2) in enumerate(zip(m.tilling_buttons, m.shader_buttons)):
            if i < num_layouts:
                btn1.config(state=tk.NORMAL, bg="lightgray")
                btn2.config(state=tk.NORMAL, bg="lightgray")
                if len(m.shader_px_indices) > 0 and i < len(m.shader_indices):
                    shader_idx = m.shader_indices[i]
                    if shader_idx < len(m.shader_px_indices):
                        btn2.config(text=str(m.shader_px_indices[shader_idx]))
            else:
                btn1.config(state=tk.DISABLED, bg="darkgray")
                btn2.config(state=tk.DISABLED, bg="darkgray")

    def __cycle_layout (m):
        layouts = m.__get_layouts()
        m.layout_idx = (m.layout_idx + 1) % len(layouts)
        m.__update_button_states()
        m.__draw_layout()

    def __cycle_rotation (m):
        m.rotation_idx = (m.rotation_idx + 1) % len(m.rotation_angles)
        m.__draw_layout()

    def __button_callback (m, idx):
        m.tilling_orientations[idx] = not m.tilling_orientations[idx]
        if m.tilling_buttons[idx].config("text")[-1] == "|":
            m.tilling_buttons[idx].config(text="-")
        else:
            m.tilling_buttons[idx].config(text="|")
        m.__draw_layout()

    def __effect_callback (m, idx):
        if len(m.shader_px_indices) > 0:
            m.shader_indices[idx] = (m.shader_indices[idx] + 1) % len(m.shader_px_indices)
            m.shader_buttons[idx].config(text=str(m.shader_px_indices[m.shader_indices[idx]]))
            m.__draw_layout()

    def __effect_callback_reverse (m, idx):
        if len(m.shader_px_indices) > 0:
            m.shader_indices[idx] = (m.shader_indices[idx] - 1) % len(m.shader_px_indices)
            m.shader_buttons[idx].config(text=str(m.shader_px_indices[m.shader_indices[idx]]))
            m.__draw_layout()

    def __rotate_point (m, x, y, angle, cx, cy):
        rad = math.radians(angle)
        cos_a = math.cos(rad)
        sin_a = math.sin(rad)
        dx = x - cx
        dy = y - cy
        new_x = cx + dx * cos_a - dy * sin_a
        new_y = cy + dx * sin_a + dy * cos_a
        return new_x, new_y

    def __draw_layout (m):
        m.layout_img = Image.new("RGB", (CANVAS_WIDTH, CANVAS_HEIGHT), (255, 255, 255))
        draw = ImageDraw.Draw(m.layout_img)
        
        layouts = m.__get_layouts()
        current_layout = layouts[m.layout_idx]
        rotation_angle = m.rotation_angles[m.rotation_idx]
        canvas_center_x = CANVAS_WIDTH // 2
        canvas_center_y = CANVAS_HEIGHT // 2
        colors = m.color_set.get_colors()
        
        for idx, field in enumerate(current_layout):
            fill_color_hex = colors[idx % len(colors)] if TEST_MODE else None
            if fill_color_hex:
                fill_color_rgb = tuple(int(fill_color_hex[i:i+2], 16) for i in (1, 3, 5))
            else:
                fill_color_rgb = None
            
            rotated_points = []
            for point in field:
                x, y = point[0], point[1]
                if rotation_angle != 0:
                    x, y = m.__rotate_point(x, y, rotation_angle, canvas_center_x, canvas_center_y)
                rotated_points.append((x, y))
            
            if fill_color_rgb:
                draw.polygon(rotated_points, fill=fill_color_rgb)
            else:
                draw.polygon(rotated_points)
        
        layout_array = np.array(m.layout_img)
        blended_float = layout_array.astype(np.float32)
        
        for idx, field in enumerate(current_layout):
            fill_color_hex = colors[idx % len(colors)] if TEST_MODE else None
            if fill_color_hex:
                field_rgb = tuple(int(fill_color_hex[i:i+2], 16) for i in (1, 3, 5))
            else:
                field_rgb = None
            
            rotated_points = []
            for point in field:
                x, y = point[0], point[1]
                if rotation_angle != 0:
                    x, y = m.__rotate_point(x, y, rotation_angle, canvas_center_x, canvas_center_y)
                rotated_points.append((int(x), int(y)))

            mask = Image.new("L", (CANVAS_WIDTH, CANVAS_HEIGHT), 0)
            mask_draw = ImageDraw.Draw(mask)
            mask_draw.polygon(rotated_points, fill=255)
            mask_array = np.array(mask, dtype=np.float32) / 255.0
            
            x_coords = [p[0] for p in rotated_points]
            y_coords = [p[1] for p in rotated_points]
            min_x = max(0, min(x_coords))
            max_x = min(CANVAS_WIDTH, max(x_coords))
            min_y = max(0, min(y_coords))
            max_y = min(CANVAS_HEIGHT, max(y_coords))

            bbox_width = max_x - min_x
            bbox_height = max_y - min_y
            
            noise_crop = m.noise_array[min_y:max_y, min_x:max_x]
            noise_crop_img = Image.fromarray(noise_crop.astype(np.uint8))
            noise_rgb = get_avg_rgb(noise_crop_img)
            
            if field_rgb:
                rgb_offset = (field_rgb[0] - noise_rgb[0],  field_rgb[1] - noise_rgb[1],  field_rgb[2] - noise_rgb[2])
                noise_crop_offset = offset_img_rgb(noise_crop_img, rgb_offset)
                noise_crop_array = np.array(noise_crop_offset, dtype=np.float32)
            else:
                noise_crop_array = noise_crop.astype(np.float32)
            
            noise_full = np.zeros((CANVAS_HEIGHT, CANVAS_WIDTH, 3), dtype=np.float32)
            noise_full[min_y:max_y, min_x:max_x] = noise_crop_array
            
            mask_3d = np.stack([mask_array, mask_array, mask_array], axis=2)
            field_region = blended_float * mask_3d
            noise_region = noise_full * mask_3d
            blended_region = cv2.addWeighted(field_region, 0.4, noise_region, 0.6, 0)
            blended_float = blended_float * (1 - mask_3d) + blended_region
            
            shader_idx = m.shader_indices[idx] if idx < len(m.shader_indices) else 0
            if shader_idx >= len(m.shader_scaled_list):
                shader_idx = 0
            shader = m.shader_scaled_rotated_list[shader_idx] if not m.tilling_orientations[idx] else m.shader_scaled_list[shader_idx]
            shader_crop = shader.crop((0, 0, min(bbox_width, shader.width), min(bbox_height, shader.height)))
            shader_crop = shader_crop.resize((bbox_width, bbox_height), Image.LANCZOS)
            
            shader_crop_array = np.array(shader_crop, dtype=np.float32)
            adjustment_crop = (shader_crop_array / 255.0) * (2 * SHADER_RANGE) - SHADER_RANGE
            adjustment_crop_3d = np.stack([adjustment_crop, adjustment_crop, adjustment_crop], axis=2)
            
            shader_full = np.zeros((CANVAS_HEIGHT, CANVAS_WIDTH, 3), dtype=np.float32)
            shader_full[min_y:max_y, min_x:max_x] = adjustment_crop_3d
            
            blended_float = blended_float + shader_full * mask_3d
        
        shaded = np.clip(blended_float, 0, 255).astype(np.uint8)
        
        m.layout_img = Image.fromarray(shaded)
        
        m.__update_canvas()
    
    def __update_canvas (m):
        m.canvas.delete("all")
        m.canvas.images = []
        photo = ImageTk.PhotoImage(m.layout_img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.images.append(photo)

    def getFarmLayout (m):
        cropped_width = m.scale_factor * RAW_WIDTH
        cropped_height = m.scale_factor * RAW_HEIGHT
        scaled_xo = (CANVAS_WIDTH - cropped_width) // 2
        scaled_yo = (CANVAS_HEIGHT - cropped_height) // 2
        cropped = m.layout_img.crop((scaled_xo, scaled_yo, scaled_xo + cropped_width, scaled_yo + cropped_height))
        contiguous_img = Image.new("RGB", cropped.size)
        contiguous_img.paste(cropped)
        return contiguous_img

    #============================================================================================================================#

    def set_farm_layout (m, index):
        m.layout_idx = index % m.layout_count

    def set_tilling_orientation (m, orientations):
        m.tilling_orientations = orientations[:4]
    
    def set_shader_indices (m, indices):
        m.shader_indices = indices[:4]

    def set_rotation_index (m, index):
        m.rotation_idx = index % len(m.rotation_angles)

    def draw_farm_layout (m):
        m.__draw_layout()

#================================================================================================================================#
#=> - Class: TileMorpher -
#================================================================================================================================#

class TileMorpher:

    def __init__ (m, frame, getFarmLayout):
        m.index = 0
        m.frame = frame
        m.getFarmLayout = getFarmLayout
        m.scale_factor = 4
        
        m.tile_img = Image.new("RGB", (CANVAS_WIDTH, CANVAS_HEIGHT), (255, 255, 255))
        m.current_morphed_img = None
        
        m.button_row = tk.Frame(frame)
        m.button_row.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.btn_morph = tk.Button(m.button_row, text="Morph", command=lambda: m.__morph_image(), width=BUTTON_WIDTH)
        m.btn_morph.pack(side=tk.LEFT, padx=5)
        
        m.btn_save = tk.Button(m.button_row, text="Save", command=lambda: m.__save_image(), width=BUTTON_WIDTH)
        m.btn_save.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(frame, width=500, height=500, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas.images = []
        
        m.__draw_tile()
        m.__update_canvas()

    def __draw_tile (m):
        m.tile_img = Image.new("RGB", (CANVAS_WIDTH, CANVAS_HEIGHT), (255, 255, 255))
        draw = ImageDraw.Draw(m.tile_img)
        
        scaled_width = TILE_WIDTH * m.scale_factor
        scaled_height = TILE_HEIGHT * m.scale_factor
        
        canvas_center_x = 250
        canvas_center_y = 250
        
        diamond_points = [
            (canvas_center_x, canvas_center_y - scaled_height / 2),
            (canvas_center_x + scaled_width / 2, canvas_center_y),
            (canvas_center_x, canvas_center_y + scaled_height / 2),
            (canvas_center_x - scaled_width / 2, canvas_center_y)
        ]
        
        m.__update_canvas()
    
    def __update_canvas (m):
        m.canvas.delete("all")
        m.canvas.images = []
        
        if m.current_morphed_img is not None:
            display_img = m.current_morphed_img.copy()
        else:
            display_img = m.tile_img
        
        photo = ImageTk.PhotoImage(display_img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.images.append(photo)

    def __remove_black_outlines (m, img):
        img = img.convert("RGB")
        pixels = img.load()
        width, height = img.size
        white = (255, 255, 255)
        black = (0, 0, 0)
        
        for y in range(height):
            prev_color = white
            for x in range(width):
                pixel = pixels[x, y]
                if pixel == black:
                    if prev_color != white:
                        pixels[x, y] = prev_color
                    else:
                        next_x = x + 1
                        while next_x < width and pixels[next_x, y] == black:
                            next_x += 1
                        if next_x < width:
                            pixels[x, y] = pixels[next_x, y]
                        else:
                            pixels[x, y] = prev_color
                else:
                    prev_color = pixel
        
        for x in range(width):
            prev_color = white
            for y in range(height):
                pixel = pixels[x, y]
                if pixel == black:
                    if prev_color != white:
                        pixels[x, y] = prev_color
                    else:
                        next_y = y + 1
                        while next_y < height and pixels[x, next_y] == black:
                            next_y += 1
                        if next_y < height:
                            pixels[x, y] = pixels[x, next_y]
                        else:
                            pixels[x, y] = prev_color
                else:
                    prev_color = pixel
        
        return img

    def __morph_to_diamond (m, img, diamond_points):
        img_array = np.array(img.copy())
        if len(img_array.shape) == 2:
            img_array = cv2.cvtColor(img_array, cv2.COLOR_GRAY2BGR)
        elif img_array.shape[2] == 4:
            img_array = cv2.cvtColor(img_array, cv2.COLOR_RGBA2BGR)
        elif img_array.shape[2] == 3:
            pass
        else:
            img_array = cv2.cvtColor(img_array, cv2.COLOR_GRAY2BGR)
        
        h, w = img_array.shape[:2]
        src_pts = np.array([[0, 0], [w - 1, 0], [w - 1, h - 1], [0, h - 1]], dtype=np.float32)

        dst_pts = np.array(diamond_points, dtype=np.float32)
        matrix = cv2.getPerspectiveTransform(src_pts, dst_pts)
        size = (CANVAS_WIDTH, CANVAS_HEIGHT)
        warped_img = cv2.warpPerspective(img_array, matrix, size, borderMode=cv2.BORDER_TRANSPARENT, flags=cv2.INTER_LINEAR)
        warped_img = cv2.cvtColor(warped_img, cv2.COLOR_BGR2RGBA)
        warped_img = np.ascontiguousarray(warped_img, dtype=np.uint8)
        return Image.fromarray(warped_img)

    def __morph_image (m):
        farm_layout_img = m.getFarmLayout()
        
        scaled_width = TILE_WIDTH * m.scale_factor
        scaled_height = TILE_HEIGHT * m.scale_factor
        canvas_center_x = CANVAS_WIDTH // 2
        canvas_center_y = CANVAS_HEIGHT // 2
        
        dst_diamond_pts = [
            (canvas_center_x, canvas_center_y - scaled_height / 2),
            (canvas_center_x + scaled_width / 2, canvas_center_y),
            (canvas_center_x, canvas_center_y + scaled_height / 2),
            (canvas_center_x - scaled_width / 2, canvas_center_y)
        ]
        
        img_no_outlines = m.__remove_black_outlines(farm_layout_img)
        morphed_img = m.__morph_to_diamond(img_no_outlines, dst_diamond_pts)
        
        dst_mask = Image.new("L", (CANVAS_WIDTH, CANVAS_HEIGHT), 0)
        dst_mask_draw = ImageDraw.Draw(dst_mask)
        dst_mask_draw.polygon([(int(p[0]), int(p[1])) for p in dst_diamond_pts], fill=255)
        
        morphed_array = np.array(morphed_img)
        dst_mask_array = np.array(dst_mask)
        magenta_rgba = np.array([255, 0, 255, 255], dtype=np.uint8)
        
        for y in range(CANVAS_HEIGHT):
            for x in range(CANVAS_WIDTH):
                if dst_mask_array[y, x] == 0:
                    morphed_array[y, x] = magenta_rgba
        
        m.current_morphed_img = Image.fromarray(morphed_array)
        
        m.__update_canvas()

    def __save_image (m, name_tag="test", index=0):
        if m.current_morphed_img is not None:
            img_array = np.array(m.current_morphed_img)
            magenta = np.array([255, 0, 255])
            
            height, width = img_array.shape[:2]
            rgb_channels = img_array[:, :, :3]
            
            top = 0
            bottom = height
            left = 0
            right = width
            
            for y in range(height):
                row = rgb_channels[y, :]
                if not np.all(np.all(row == magenta, axis=1)):
                    top = y
                    break
            
            for y in range(height - 1, -1, -1):
                row = rgb_channels[y, :]
                if not np.all(np.all(row == magenta, axis=1)):
                    bottom = y + 1
                    break
            
            for x in range(width):
                col = rgb_channels[:, x]
                if not np.all(np.all(col == magenta, axis=1)):
                    left = x
                    break
            
            for x in range(width - 1, -1, -1):
                col = rgb_channels[:, x]
                if not np.all(np.all(col == magenta, axis=1)):
                    right = x + 1
                    break
            
            cropped = m.current_morphed_img.crop((left, top, right, bottom))
            resized = cropped.resize((TILE_WIDTH, TILE_HEIGHT), Image.NEAREST) # 
            resized.save("farm_tile_%s_%s.png" %(name_tag, str(index).zfill(3)))

    def count_tile_whites (m):
        scaled_width = TILE_WIDTH * m.scale_factor
        scaled_height = TILE_HEIGHT * m.scale_factor
        canvas_center_x = CANVAS_WIDTH // 2
        canvas_center_y = CANVAS_HEIGHT // 2
        
        diamond_points = [
            (canvas_center_x, canvas_center_y - scaled_height / 2),
            (canvas_center_x + scaled_width / 2, canvas_center_y),
            (canvas_center_x, canvas_center_y + scaled_height / 2),
            (canvas_center_x - scaled_width / 2, canvas_center_y)
        ]
        
        diamond_poly = [(int(p[0]), int(p[1])) for p in diamond_points]
        
        mask = Image.new("L", (CANVAS_WIDTH, CANVAS_HEIGHT), 0)
        mask_draw = ImageDraw.Draw(mask)
        mask_draw.polygon(diamond_poly, fill=255)
        
        canvas_img = Image.new("RGB", (CANVAS_WIDTH, CANVAS_HEIGHT), (255, 255, 255))
        if m.current_morphed_img is not None:
            canvas_img.paste(m.current_morphed_img, (0, 0), m.current_morphed_img)
        
        pixels = canvas_img.load()
        mask_pixels = mask.load()
        
        white = (255, 255, 255)
        count_inside = 0
        count_outside = 0
        
        width, height = canvas_img.size
        for y in range(height):
            for x in range(width):
                pixel = pixels[x, y]
                is_white = pixel == white
                is_inside = mask_pixels[x, y] > 0
                
                if is_white:
                    if is_inside:
                        count_inside += 1
                    else:
                        count_outside += 1
        
        return count_inside, count_outside

    #============================================================================================================================#

    def save_farm_tile (m, name_tag):
        m.__morph_image()
        m.__save_image(name_tag, m.index)
        m.index += 1
        print("*** Saved farm tile: %d" %m.index)

#================================================================================================================================#
#=> - Class: FarmGen -
#================================================================================================================================#

class FarmGen:

    def __init__ (m, root, color_set):
        m.root = root
        m.color_set = color_set
        
        m.canvas_frame = tk.Frame(root)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.frame_left = tk.Frame(m.canvas_frame)
        m.frame_left.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.frame_right = tk.Frame(m.canvas_frame)
        m.frame_right.pack(side=tk.LEFT, padx=5, pady=5)
        
        m.farm_layout = FarmLayout(m.frame_left, m.color_set)
        m.tile_morpher = TileMorpher(m.frame_right, m.farm_layout.getFarmLayout)

    #============================================================================================================================#

    def set_farm_layout (m, index):
        m.farm_layout.set_farm_layout(index)

    def set_tilling_orientation (m, orientations):
        m.farm_layout.set_tilling_orientation(orientations)
    
    def set_shader_indices (m, indices):
        m.farm_layout.set_shader_indices(indices)

    def set_rotation_index (m, index):
        m.farm_layout.set_rotation_index(index)

    def save_farm_layout (m, name_tag):
        m.farm_layout.draw_farm_layout()
        m.tile_morpher.save_farm_tile(name_tag)

#================================================================================================================================#
#=> - Helper Functions -
#================================================================================================================================#

def run_program (name_tag):
    if name_tag == "deep_green":
        color_set = ColorSet()
    else:
        print("*** Invalid program/color-set")
        exit(1)
    root = tk.Tk()
    root.title("Farm Generator")
    farm_gen = FarmGen(root, color_set)

    farm_gen.set_farm_layout(0)
    color_set.shuffle_colors()
    farm_gen.set_tilling_orientation([False, True, True, False])
    farm_gen.set_shader_indices([2, 2, 2, 2])
    farm_gen.save_farm_layout(name_tag)

    color_set.rotate_colors()
    farm_gen.set_tilling_orientation([True, True, True, False])
    farm_gen.save_farm_layout(name_tag)

    color_set.rotate_colors()
    farm_gen.set_tilling_orientation([False, True, True, False])
    farm_gen.save_farm_layout(name_tag)

    farm_gen.set_farm_layout(1)
    color_set.shuffle_colors()
    farm_gen.save_farm_layout(name_tag)

    farm_gen.set_farm_layout(2)
    color_set.shuffle_colors()
    farm_gen.save_farm_layout(name_tag)

    color_set.rotate_colors()
    farm_gen.save_farm_layout(name_tag)

    farm_gen.set_farm_layout(3)
    color_set.shuffle_colors()
    farm_gen.save_farm_layout(name_tag)

    color_set.rotate_colors()
    farm_gen.save_farm_layout(name_tag)

    farm_gen.set_rotation_index(1)
    color_set.shuffle_colors()
    farm_gen.save_farm_layout(name_tag)

    farm_gen.set_farm_layout(4)
    color_set.shuffle_colors()
    farm_gen.save_farm_layout(name_tag)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    program = None
    if len(sys.argv) > 1:
        program = sys.argv[1]

    if TEST_MODE:
        run_program(program)
    else:
        root = tk.Tk()
        root.title("Farm Generator")
        color_set = ColorSet()
        farm_gen = FarmGen(root, color_set)
        if TEST_MODE:
            farm_gen.farm_layout.btn_cycle.invoke()
            farm_gen.tile_morpher.btn_morph.invoke()
        root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
