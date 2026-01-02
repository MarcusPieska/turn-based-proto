#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from map_tile import MapTile

#================================================================================================================================#
#=> - Class: RegularTileMaker
#================================================================================================================================#

class RegularTileMaker:

    def __do_print (m, message):
        print ("*** " + message)

    def __coords_to_tile_indices (m, x, y, tiles):
        if x < m.x_min or x > m.x_max or y < m.y_min or y > m.y_max:
            return None
        x_index = int((x - m.x_min) // m.tile_w)
        y_index = int((y - m.y_min) // m.tile_h)
        if x_index < 0 or x_index >= len(tiles) or y_index < 0 or y_index >= len(tiles[x_index]):
            return None
        return x_index, y_index

    def __init__ (m, map_w, map_h, tile_w, tile_h, model):
        model.tiler = m
        m.x_min = (map_w % tile_w) / 2
        m.y_min = (map_h % tile_h) / 2
        m.x_max = map_w - m.x_min
        m.y_max = map_h - m.y_min
        m.tile_w = tile_w
        m.tile_h = tile_h
        m.model = model

        tiles, lines = [], []
        x = m.x_min
        while x <= m.x_max:
            lines.append(((x, m.y_min), (x, m.y_max)))
            x += m.tile_w
        y = m.y_min
        while y <= m.y_max:
            lines.append(((m.x_min, y), (m.x_max, y)))
            y += m.tile_h
        model.set_lines (lines)

        x = m.x_min
        while x < m.x_max:
            y = m.y_min
            tile_column = []
            while y < m.y_max:
                pts = ((x, y), (x + m.tile_w, y), (x + m.tile_w, y + m.tile_h), (x, y + m.tile_h))
                tile_column.append(MapTile(pts))
                y += m.tile_h
            tiles.append(tile_column)
            x += m.tile_w
        model.set_tiles (tiles)
        
    def coords_to_tile (m, x, y):
        tiles = m.model.get_tiles ()
        if x < m.x_min or x > m.x_max or y < m.y_min or y > m.y_max:
            return None
        x_index = int((x - m.x_min) // m.tile_w)
        y_index = int((y - m.y_min) // m.tile_h)
        if x_index < 0 or x_index >= len(tiles) or y_index < 0 or y_index >= len(tiles[x_index]):
            return None
        return tiles[x_index][y_index]

    def coords_to_near_tiles (m, x, y):
        tiles = m.model.get_tiles ()
        selected = []
        selected.append(m.coords_to_tile(x, y - m.tile_h, tiles))
        selected.append(m.coords_to_tile(x + m.tile_w, y, tiles))
        selected.append(m.coords_to_tile(x, y + m.tile_h, tiles))
        selected.append(m.coords_to_tile(x - m.tile_w, y, tiles))
        return selected

    def coords_to_diagonal_tiles (m, x, y):
        tiles = m.model.get_tiles ()
        selected = []
        selected.append(m.coords_to_tile(x + m.tile_w, y - m.tile_h, tiles))
        selected.append(m.coords_to_tile(x + m.tile_w, y + m.tile_h, tiles))
        selected.append(m.coords_to_tile(x - m.tile_w, y - m.tile_h, tiles))
        selected.append(m.coords_to_tile(x - m.tile_w, y + m.tile_h, tiles))
        return selected

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    pass

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
