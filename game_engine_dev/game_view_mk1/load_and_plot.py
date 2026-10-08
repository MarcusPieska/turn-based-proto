#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk

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

    def __init__ (m, root, w_width, w_height, m_width, m_height):
        m.w_width = w_width
        m.w_height = w_height
        m.m_width = m_width
        m.m_height = m_height

        m.canvas_frame = tk.Frame(root)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        m.canvas = MapCanvas(m.canvas_frame, w_width, w_height)
        m.canvas.configure(scrollregion=(0, 0, m_width, m_height))
        m.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        m.scrollbar_y = tk.Scrollbar(m.canvas_frame, orient=tk.VERTICAL, command=m.canvas.yview)
        m.scrollbar_y.pack(side=tk.RIGHT, fill=tk.Y)
        m.canvas.configure(yscrollcommand=m.scrollbar_y.set)

        m.scrollbar_x = tk.Scrollbar(root, orient=tk.HORIZONTAL, command=m.canvas.xview)
        m.scrollbar_x.pack(side=tk.BOTTOM, fill=tk.X)
        m.canvas.configure(xscrollcommand=m.scrollbar_x.set)

    def draw_line (m, pt1, pt2):
        m.canvas.create_line (pt1[0], pt1[1], pt2[0], pt2[1], tags=m.LINE_TAG)

    def draw_polygon (m, corners):
        m.canvas.create_polygon (
            corners[0][0], corners[0][1],
            corners[1][0], corners[1][1],
            corners[2][0], corners[2][1],
            corners[3][0], corners[3][1],
            outline="black", fill="", tags=m.LINE_TAG
        )

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def load_tiles_from_file (filename):
    tiles = []
    with open(filename, 'r') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue
            corners_str = line.split(';')
            corners = []
            for corner_str in corners_str:
                x, y = corner_str.split(',')
                corners.append((int(x), int(y)))
            if len(corners) == 4:
                tiles.append(corners)
    return tiles

def plot_tiles (filename, map_width, map_height):
    root = tk.Tk()
    root.title("Loaded Map")
    
    game_map = GameMap(root, 1600, 800, map_width, map_height)
    tiles = load_tiles_from_file(filename)
    
    for corners in tiles:
        game_map.draw_polygon(corners)
    
    root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    plot_tiles("tiles.dat", 6050, 6050)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

