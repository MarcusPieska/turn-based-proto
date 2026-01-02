#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from map_tile import MapTile
from map_model import MapModel

#================================================================================================================================#
#=> - Class: DiamondTileMaker
#================================================================================================================================#

class DiamondTileMaker:

    def __do_print (m, message):
        print ("*** " + message)

    def __coords_to_indices (m, x, y, tiles):
        tile, _, _ = m.coords_to_tile (x, y, tiles)
        if tile is None:
            return None, None
        for row in range(len(tiles)):
            for col in range(len(tiles[row])):
                if tiles[row][col] is tile:
                    return row, col
        return None, None

    def __init__ (m, map_w, map_h, tile_w, tile_h, model):
        model.tiler = m
        m.tile_w = tile_w
        m.tile_h = tile_h
        m.half_w = tile_w // 2
        m.half_h = tile_h // 2
        m.num_cols = int(map_w // tile_w)
        m.num_rows = int((map_h - tile_h) // m.half_h)
        m.x_min = int((map_w - (m.num_cols * tile_w)) / 2)
        m.y_min = int((map_h - (m.num_rows * m.half_h)) / 2 - m.half_h / 2)
        m.model = model
        
        tiles, lines = [], []
        y = m.y_min + m.half_h
        row = 0
        left_pts, right_pts, bottom_pts, top_pts = [], [], [], []
        while row < m.num_rows:
            x_offset = m.half_w if (row % 2 == 1) else 0
            x = m.x_min + x_offset + m.half_w
            
            col = 0
            tile_row = []
            while x + m.half_w <= map_w - m.x_min:  
                cx, cy = int(x), int(y)
                top = (cx, cy - m.half_h)
                right = (cx + m.half_w, cy)
                bottom = (cx, cy + m.half_h)
                left = (cx - m.half_w, cy)

                if (col == 0) and ((row % 2) == 0):
                    left_pts.append(left)
                if (col == m.num_cols - 1) and ((row % 2) == 0):
                    right_pts.append(right)
                
                if row == 0:
                    top_pts.append(top)
                elif row == m.num_rows - 1:
                    bottom_pts.append(bottom)

                tile_row.append(MapTile((top, right, bottom, left)))
                x += tile_w
                col += 1
            
            tiles.append(tile_row)
            y += m.half_h
            row += 1
        model.set_tiles (tiles)

        for pt1, pt2 in zip(left_pts + bottom_pts, top_pts + right_pts): 
            lines.append((pt1, pt2))
        for pt1, pt2 in zip(right_pts + bottom_pts[::-1], top_pts[::-1] + left_pts): 
            lines.append((pt1, pt2))
        model.set_lines (lines)

    def coords_to_tile (m, x, y, tiles=None):
        tiles = m.model.get_tiles () if tiles is None else tiles
        if len(tiles) == 0:
            return None
        best_tile, best_row, best_col = None, None, None
        best_dist = float('inf')
        for row in range(len(tiles)):
            row_y = m.y_min + m.half_h + row * m.half_h
            for col in range(len(tiles[row])):
                tile = tiles[row][col]
                cx = tile.pts[0][0]
                cy = row_y
                dx = abs(x - cx)
                dy = abs(y - cy)
                if dx * m.half_h + dy * m.half_w <= m.half_w * m.half_h:
                    dist = dx * dx + dy * dy
                    if dist < best_dist:
                        best_dist = dist
                        best_tile = tile
                        best_row = row
                        best_col = col
        return best_tile, best_row, best_col

    def coords_to_near_tiles (m, x, y):
        tiles = m.model.get_tiles ()
        row, col = m.__coords_to_indices (x, y, tiles)
        if row is None or col is None:
            return [None, None, None, None]
        selected = []
        row_offset = -1 if row % 2 == 0 else 0

        target_col = col + row_offset
        target_row = row - 1
        if target_row >= 0:
            selected.append(tiles[target_row][target_col] if target_col >= 0 else None)
            target_col += 1
            selected.append(tiles[target_row][target_col] if target_col < len(tiles[target_row]) else None)
        else: selected.extend([None, None])

        target_col = col + row_offset
        target_row = row + 1
        if target_row < len(tiles):
            selected.append(tiles[target_row][target_col] if target_col >= 0 else None)
            target_col += 1
            selected.append(tiles[target_row][target_col] if target_col < len(tiles[target_row]) else None)
        else: selected.extend([None, None])
        
        return selected

    def coords_to_diagonal_tiles (m, x, y):
        tiles = m.model.get_tiles ()
        row, col = m.__coords_to_indices (x, y, tiles)
        if row is None or col is None:
            return [None, None, None, None]
        selected = []
        target_col = col - 1
        selected.append(tiles[row][target_col] if target_col >= 0 else None)
        target_col = col + 1
        selected.append(tiles[row][target_col] if target_col < len(tiles[row]) else None)
        
        if row >= 2:
            target_row = row - 2
            selected.append(tiles[target_row][col] if (col >= 0 and col < len(tiles[target_row])) else None)
        else: selected.append(None)
        
        if row < len(tiles) - 2:
            target_row = row + 2
            selected.append(tiles[target_row][col] if (col >= 0 and col < len(tiles[target_row])) else None)
        else: selected.append(None)
        
        return selected

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    pass

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
