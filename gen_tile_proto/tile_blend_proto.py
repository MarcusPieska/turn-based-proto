#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from PIL import Image, ImageTk
import numpy as np

SIZE_BOOST = 0

#================================================================================================================================#
#=> - Class: TileBlendEditor -
#================================================================================================================================#

class TileBlendEditor:

    def __toggleGrid (m):
        m.show_grid = not m.show_grid
        m.__drawDiamondGrid()

    def __calculateTilePositions (m):
        m.tile_positions = []
        half_width = m.tile_width // 2
        half_height = m.tile_height // 2
        center_x = m.canv_w // 2
        center_y = m.canv_h // 2
        total_height = m.num_rows * half_height
        start_y = center_y - total_height // 2
        for row_idx in range(m.num_rows):
            row_positions = []
            num_tiles = m.row_counts[row_idx]
            y_center = start_y + row_idx * half_height + half_height
            if row_idx % 2 == 0:
                row_offset = 0
            else:
                row_offset = half_width
            center_tile_idx = num_tiles // 2
            x_center_tile = center_x + row_offset
            for tile_idx in range(num_tiles):
                offset_from_center = tile_idx - center_tile_idx
                x_center = x_center_tile + offset_from_center * m.tile_width
                top_left = (x_center - half_width, y_center - half_height)
                row_positions.append(top_left)
            m.tile_positions.append(row_positions)

    def __drawDiamondGrid (m):
        m.canv_diamond.delete("grid")
        if not m.show_grid:
            return
        half_width = m.tile_width // 2
        half_height = m.tile_height // 2
        for row_idx in range(m.num_rows):
            for top_left in m.tile_positions[row_idx]:
                x_top_left, y_top_left = top_left
                x_center = x_top_left + half_width
                y_center = y_top_left + half_height
                top = (x_center, y_center - half_height)
                right = (x_center + half_width, y_center)
                bottom = (x_center, y_center + half_height)
                left = (x_center - half_width, y_center)
                m.canv_diamond.create_line(top[0], top[1], right[0], right[1], fill="light gray", width=1, tags="grid")
                m.canv_diamond.create_line(right[0], right[1], bottom[0], bottom[1], fill="light gray", width=1, tags="grid")
                m.canv_diamond.create_line(bottom[0], bottom[1], left[0], left[1], fill="light gray", width=1, tags="grid")
                m.canv_diamond.create_line(left[0], left[1], top[0], top[1], fill="light gray", width=1, tags="grid")
        m.canv_diamond.tag_raise("grid")
    
    def __init__ (m, root, tile_width, tile_height):
        m.root = root
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.show_grid = True
        m.canv_w, m.canv_h = 1200, 800
        m.row_counts = [1, 2, 3, 4, 3, 2, 1]
        m.num_rows = len(m.row_counts)
        m.max_tiles_per_row = max(m.row_counts)
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.grid_btn = tk.Button(m.top_frame, text="Toggle Grid", command=m.__toggleGrid, width=15)
        m.grid_btn.pack(side=tk.LEFT, padx=5)
        
        m.canv_frame = tk.Frame(root)
        m.canv_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_diamond = tk.Canvas(m.canv_frame, width=m.canv_w, height=m.canv_h, bg="gray")
        m.canv_diamond.pack(side=tk.LEFT, padx=5)
        m.__calculateTilePositions()
        m.__drawDiamondGrid()

    def __loadTileWithAlpha (m, tile_path):
        img = Image.open(tile_path).convert("RGB")
        img = img.resize((m.tile_width, m.tile_height), Image.NEAREST)
        img_array = np.array(img)
        alpha = np.ones((m.tile_height, m.tile_width), dtype=np.uint8) * 255
        black_mask = (img_array[:, :, 0] == 0) & (img_array[:, :, 1] == 0) & (img_array[:, :, 2] == 0)
        alpha[black_mask] = 0
        img_rgba = Image.fromarray(np.dstack([img_array, alpha]), mode='RGBA')
        return ImageTk.PhotoImage(img_rgba)

    def __tileParts (m, tiles, parts):
        m.canv_diamond.delete("tile")
        m.tile_images = []
        for part, tile in zip(parts, tiles):
            for pos in part:
                x, y = pos
                m.canv_diamond.create_image(x, y, anchor=tk.NW, image=tile, tags="tile")
            m.tile_images.append(tile)

    def tile_two_split_diagonal (m, tile_one, tile_two):
        alpha_img = Image.open("../../DEL_temp_tile_alpha_north_east.png").convert("RGB")
        alpha_img = alpha_img.resize((m.tile_width, m.tile_height), Image.NEAREST)
        alpha_array = np.array(alpha_img)
        alpha_grey = alpha_array[:, :, 0].astype(np.float32) / 255.0
        
        tile1_array = np.array(Image.open(tile_one).convert("RGB").resize((m.tile_width, m.tile_height), Image.NEAREST))
        tile2_array = np.array(Image.open(tile_two).convert("RGB").resize((m.tile_width, m.tile_height), Image.NEAREST))
        alpha_factor = 1.0 - alpha_grey
        alpha_factor = alpha_factor[:, :, np.newaxis]
        tile_blend_array = (tile1_array * alpha_factor + tile2_array * (1.0 - alpha_factor)).astype(np.uint8)
        
        tile_blend_alpha = np.ones((m.tile_height, m.tile_width), dtype=np.uint8) * 255
        black_mask = (tile_blend_array[:, :, 0] == 0) & (tile_blend_array[:, :, 1] == 0) & (tile_blend_array[:, :, 2] == 0)
        tile_blend_alpha[black_mask] = 0
        tile_blend_rgba = Image.fromarray(np.dstack([tile_blend_array, tile_blend_alpha]), mode='RGBA')
        tile_blend_img = ImageTk.PhotoImage(tile_blend_rgba)
        
        part1 = []
        part1.extend ([m.tile_positions[0][0]])
        part1.extend ([m.tile_positions[1][0], m.tile_positions[1][1]])
        part1.extend ([m.tile_positions[2][1], m.tile_positions[2][2]])
        part1.extend ([m.tile_positions[3][2], m.tile_positions[3][3]])
        part1.extend ([m.tile_positions[4][2]])

        part_blend = [m.tile_positions[2][0], m.tile_positions[3][1], m.tile_positions[4][1], m.tile_positions[5][1]]

        part2 = [p for sublist in m.tile_positions for p in sublist]
        part2 = [p for p in part2 if p not in part1 and p not in part_blend]
        tile1_img = m.__loadTileWithAlpha(tile_one)
        tile2_img = m.__loadTileWithAlpha(tile_two)
        m.__tileParts([tile1_img, tile2_img, tile_blend_img], [part1, part2, part_blend])

    def tile_two_split_horizontal (m, tile_one, tile_two):
        part1 = []
        part1.extend (m.tile_positions[0])
        part1.extend (m.tile_positions[1])
        part1.extend (m.tile_positions[2])
        part2 = [p for sublist in m.tile_positions for p in sublist]
        part2 = [p for p in part2 if p not in part1]
        tile1_img = m.__loadTileWithAlpha(tile_one)
        tile2_img = m.__loadTileWithAlpha(tile_two)
        m.__tileParts([tile1_img, tile2_img], [part1, part2])

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    tiles = []
    tiles.append("/home/w/Projects/img-content/temp_tile_coast.png")
    tiles.append("/home/w/Projects/img-content/temp_tile_desert.png")
    tiles.append("/home/w/Projects/img-content/temp_tile_grassland.png")
    tiles.append("/home/w/Projects/img-content/temp_tile_ocean.png")
    tiles.append("/home/w/Projects/img-content/temp_tile_plains.png")
    tiles.append("/home/w/Projects/img-content/temp_tile_tundra.png")
    
    root = tk.Tk()
    root.title("Tile Blend Editor")
    tile_width = 100
    tile_height = 50
    editor = TileBlendEditor(root, tile_width, tile_height)
    editor.tile_two_split_diagonal (tiles[2], tiles[1])
    #editor.tile_two_split_horizontal (tiles[2], tiles[1])
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
