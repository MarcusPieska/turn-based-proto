#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import copy

#================================================================================================================================#
#=> - Class: MapModel
#================================================================================================================================#

class MapModel:

    def __init__ (m, tile_width, tile_height):
        m.lines, m.tiles = [], []
        m.tile_width = tile_width
        m.tile_height = tile_height

    def get_lines_deep_copy (m):
        return copy.deepcopy(m.lines)

    def get_tiles_deep_copy (m):
        return copy.deepcopy(m.tiles)

    def get_lines (m):
        return m.lines

    def get_tiles (m):
        return m.tiles

    def set_lines (m, lines):
        m.lines = lines

    def set_tiles (m, tiles):
        m.tiles = tiles

    def coords_to_tile (m, x, y):
        return m.tiler.coords_to_tile (x, y)

    def coords_to_near_tiles (m, x, y):
        return m.tiler.coords_to_near_tiles (x, y)

    def coords_to_diagonal_tiles (m, x, y, tiles=None):
        return m.tiler.coords_to_diagonal_tiles (x, y)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    pass

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
