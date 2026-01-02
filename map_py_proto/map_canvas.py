#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk

from tile_regular import RegularTileMaker
from tile_diamond import DiamondTileMaker
from map_model import MapModel
from map_zoomer import MapZoomer

#================================================================================================================================#
#=> - Class: MapCanvas
#================================================================================================================================#

class MapCanvas(tk.Canvas):

    def __init__ (m, parent, width, height):
        tk.Canvas.__init__(m, parent, width=width, height=height)

#================================================================================================================================#
#=> - Class: GameMap
#================================================================================================================================#

class GameMap:

    LINE_TAG = "line"
    MARKER_TAG = "marker"
    ZOOM_DEFAULT = 100
    ZOOM_MAX = 200
    ZOOM_MIN = 10
    ZOOM_STEP = 10
    SCROLL_AMOUNT = 10
    
    class MockClickEvent:

        def __init__ (m, x, y):
            m.x = x
            m.y = y
    
    def __do_print (m, message):
        print ("*** " + message)

    def __draw_line (m, pt1, pt2):
        m.canvas.create_line (pt1[0], pt1[1], pt2[0], pt2[1], tags=m.LINE_TAG)

    def __highlight_tile (m, tile, color):
        if m.zoom_level != m.ZOOM_DEFAULT:
            corners = []
            for corner in tile.corners:
                corners.append((int(corner[0] * m.zoom_level / 100.0), int(corner[1] * m.zoom_level / 100.0)))
        else:
            corners = tile.corners
        return m.canvas.create_polygon (corners, fill=color, tags=m.MARKER_TAG)

    def __on_canvas_click (m, event):
        xo = m.canvas.canvasx (event.x)
        yo = m.canvas.canvasy (event.y)
        if m.zoom_level != m.ZOOM_DEFAULT:
            xo = int(xo / (m.zoom_level / 100.0))
            yo = int(yo / (m.zoom_level / 100.0))
        tile, row, col = m.model.coords_to_tile (xo, yo)
        if tile is not None:
            m.prev_click_row_col = (row, col)
            if m.clicked_tile is not None:
                m.canvas.delete (m.clicked_tile)
            for t in m.near_tiles:
                m.canvas.delete (t)
            m.near_tiles = []
            for t in m.diagonal_tiles:
                m.canvas.delete (t)
            m.diagonal_tiles = []

            m.clicked_tile = m.__highlight_tile (tile, "darkgreen")
            for t in m.model.coords_to_near_tiles (xo, yo):
                if t: m.near_tiles.append(m.__highlight_tile (t, "forestgreen"))
            for t in m.model.coords_to_diagonal_tiles (xo, yo):
                if t: m.diagonal_tiles.append(m.__highlight_tile (t, "lightgreen"))
            m.canvas.tag_raise (m.LINE_TAG, m.MARKER_TAG)

        else:
            m.__do_print ("No tile found at coordinates: " + str(xo) + ", " + str(yo))

    def __on_key_release (m, event):
        pass

    def __handle_zoom (m, zoom_in=True):
        zoom_factor_changed = False
        if zoom_in:
            if m.zoom_level + m.ZOOM_STEP <= m.ZOOM_MAX:
                m.zoom_level += m.ZOOM_STEP
                zoom_factor_changed = True
        else:
            if m.zoom_level - m.ZOOM_STEP >= m.ZOOM_MIN:
                m.zoom_level -= m.ZOOM_STEP
                zoom_factor_changed = True

        if zoom_factor_changed:
            m.canvas.delete (m.LINE_TAG)
            lines_to_draw = None
            if m.zoom_level == m.ZOOM_DEFAULT:
                lines_to_draw = m.model.get_lines ()
                m.tiles = m.model.get_tiles ()
            else:
                zoomer = MapZoomer (m.model)
                zoomer.zoom_with_factor (m.zoom_level / 100.0)
                lines_to_draw = zoomer.get_zoomed_lines ()
                m.tiles = zoomer.get_zoomed_tiles ()
            for line in lines_to_draw:
                m.__draw_line (line[0], line[1])

            if m.prev_click_row_col is not None:
                tile = m.tiles[m.prev_click_row_col[0]][m.prev_click_row_col[1]]
                x = sum([corner[0] for corner in tile.corners]) / len(tile.corners) - m.canvas.canvasx (0)
                y = sum([corner[1] for corner in tile.corners]) / len(tile.corners) - m.canvas.canvasy (0)
                m.__on_canvas_click (m.MockClickEvent (x, y))

    def __on_key_press (m, event):
        key = event.keysym

        if key == "Up":
            m.canvas.yview_scroll (-m.SCROLL_AMOUNT, "units")
        elif key == "Down":
            m.canvas.yview_scroll (m.SCROLL_AMOUNT, "units")
        elif key == "Left":
            m.canvas.xview_scroll (-m.SCROLL_AMOUNT, "units")
        elif key == "Right":
            m.canvas.xview_scroll (m.SCROLL_AMOUNT, "units")
        
        elif key in ["plus", "equal"]:
            m.__handle_zoom (zoom_in=True)
        elif key in ["minus", "underscore"]:
            m.__handle_zoom (zoom_in=False)

    def __init__ (m, root, w_width, w_height, m_width, m_height, t_width, t_height, model):
        m.clicked_tile = None
        m.near_tiles, m.diagonal_tiles = [], []
        m.zoom_level = 100
        m.prev_click_row_col = None
        
        m.w_width = w_width
        m.w_height = w_height
        m.m_width = m_width
        m.m_height = m_height
        m.model = model

        m.canvas_frame = tk.Frame (root)
        m.canvas_frame.pack (side=tk.TOP, fill=tk.BOTH, expand=True)

        m.canvas = MapCanvas (m.canvas_frame, w_width, w_height)
        m.canvas.configure (scrollregion=(0, 0, m_width, m_height))
        m.canvas.pack (side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.canvas.bind ("<Button-1>", m.__on_canvas_click)

        m.scrollbar_y = tk.Scrollbar (m.canvas_frame, orient=tk.VERTICAL, command=m.canvas.yview)
        m.scrollbar_y.pack (side=tk.RIGHT, fill=tk.Y)
        m.canvas.configure (yscrollcommand=m.scrollbar_y.set)

        m.scrollbar_x = tk.Scrollbar (root, orient=tk.HORIZONTAL, command=m.canvas.xview)
        m.scrollbar_x.pack (side=tk.BOTTOM, fill=tk.X)
        m.canvas.configure (xscrollcommand=m.scrollbar_x.set)

        root.bind ("<Key>", m.__on_key_press)
        root.bind ("<KeyRelease>", m.__on_key_release)

        for line in model.get_lines ():
            m.__draw_line (line[0], line[1])
        m.tiles = model.get_tiles ()

#================================================================================================================================#
#=> - Test function -
#================================================================================================================================#

def test_map_canvas ():
    root = tk.Tk ()
    root.title ("Game Map")
    game_map = GameMap (root, 600, 600, 6000, 6000)
    root.mainloop ()

def test_regular_tile_maker ():
    root = tk.Tk()
    root.title("Game Map")
    m_width, m_height = 6050, 6050
    t_width, t_height = 100, 100
    w_width, w_height = 1600, 800

    model = MapModel (t_width, t_height)
    game_map = GameMap (root, w_width, w_height, m_width, m_height, t_width, t_height, model)
    tiler = RegularTileMaker (m_width, m_height, t_width, t_height, model)

    root.mainloop()

def test_diamond_tile_maker1 ():
    root = tk.Tk()
    root.title("Game Map")
    m_width, m_height = 6050, 6050
    t_width, t_height = 100, 100
    w_width, w_height = 1600, 800

    model = MapModel (t_width, t_height)
    game_map = GameMap (root, w_width, w_height, m_width, m_height, t_width, t_height, model)
    tiler = DiamondTileMaker (m_width, m_height, t_width, t_height, model)

    root.mainloop()

def test_diamond_tile_maker2 ():
    root = tk.Tk()
    root.title("Game Map")
    m_width, m_height = 6050, 6050
    t_width, t_height = 100, 60
    w_width, w_height = 1600, 800

    model = MapModel (t_width, t_height)
    tiler = DiamondTileMaker (m_width, m_height, t_width, t_height, model)
    game_map = GameMap (root, w_width, w_height, m_width, m_height, t_width, t_height, model)
    
    root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    #test_map_canvas ()
    #test_regular_tile_maker ()
    #test_diamond_tile_maker1 ()
    test_diamond_tile_maker2 ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
