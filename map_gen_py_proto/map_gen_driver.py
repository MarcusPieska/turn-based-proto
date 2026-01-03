#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from map_img_model import MapImgModel

import random

#================================================================================================================================#
#=> - Pangea shaping functions
#================================================================================================================================#

def drawLineBetweenPoints (m, model, x1, y1, x2, y2):
    points = []
    dx = abs(x2 - x1)
    dy = abs(y2 - y1)
    sx = 1 if x1 < x2 else -1
    sy = 1 if y1 < y2 else -1
    err = dx - dy
    x, y = x1, y1
    while True:
        if model.VALID_COORDS(x, y):
            points.append((x, y))
        if x == x2 and y == y2:
            break
        e2 = 2 * err
        if e2 > -dy:
            err -= dy
            x += sx
        if e2 < dx:
            err += dx
            y += sy
    return points

#================================================================================================================================#

def getRandomPangeaCoastTile (m, model):
    while True:
        use_row = random.choice([True, False])
        offset = random.choice([1, -1])
        if use_row:
            row = random.randint(0, model.height - 1)
            start = 0 if offset == 1 else model.width - 1
            stop = model.width if offset == 1 else -1
            for x in range(start, stop, offset):
                if model.getTerrain(x, row) == MapImgModel.TERRAIN_GRASSLAND:
                    return x, row
        else:
            col = random.randint(0, model.width - 1)
            start = 0 if offset == 1 else model.height - 1
            stop = model.height if offset == 1 else -1
            for y in range(start, stop, offset):
                if model.getTerrain(col, y) == MapImgModel.TERRAIN_GRASSLAND:
                    return col, y

def isPangeaCoastalTile (m, model, x, y):
    if not model.VALID_COORDS(x, y):
        return False
    if model.getTerrain(x, y) != MapImgModel.TERRAIN_GRASSLAND:
        return False
    neighbors = [(x-1, y), (x+1, y), (x, y-1), (x, y+1)]
    for nx, ny in neighbors:
        if model.VALID_COORDS(nx, ny):
            if model.getTerrain(nx, ny) == MapImgModel.TERRAIN_OCEAN:
                return True
    return False

def walkPangeaCoastLineToNewCoastTile (m, model, x, y, steps):
    curr_x, curr_y = x, y
    for step in range(steps):
        neighbors = [(curr_x-1, curr_y), (curr_x+1, curr_y), (curr_x, curr_y-1), (curr_x, curr_y+1)]
        neighbors.extend([(curr_x-1, curr_y-1), (curr_x+1, curr_y-1), (curr_x-1, curr_y+1), (curr_x+1, curr_y+1)])
        valid_next = []
        for nx, ny in neighbors:
            if isPangeaCoastalTile(m, model, nx, ny):
                if model.getMetadata(nx, ny).getTempTag() == 0:
                    model.getMetadata(nx, ny).setTempTag(1)
                    valid_next.append((nx, ny))
                    break
        if not valid_next:
            return None, None
        next_x, next_y = random.choice(valid_next)
        curr_x, curr_y = next_x, next_y
    return curr_x, curr_y

def getPangeaDistantOffShorePoint (m, model, x, y, bulge_fract):
    center_x = model.width // 2
    center_y = model.height // 2
    dx = x - center_x
    dy = y - center_y
    if dx == 0 and dy == 0:
        return None, None
    dist_to_center = (dx ** 2 + dy ** 2) ** 0.5
    if dist_to_center == 0:
        return None, None
    dir_x = dx / dist_to_center
    dir_y = dy / dist_to_center
    boundary_points = []
    if dir_x > 0:
        t = (model.width - 1 - x) / dir_x
        if t > 0:
            by = y + dir_y * t
            if 0 <= by < model.height:
                boundary_points.append((model.width - 1, int(by)))
    elif dir_x < 0:
        t = (0 - x) / dir_x
        if t > 0:
            by = y + dir_y * t
            if 0 <= by < model.height:
                boundary_points.append((0, int(by)))
    if dir_y > 0:
        t = (model.height - 1 - y) / dir_y
        if t > 0:
            bx = x + dir_x * t
            if 0 <= bx < model.width:
                boundary_points.append((int(bx), model.height - 1))
    elif dir_y < 0:
        t = (0 - y) / dir_y
        if t > 0:
            bx = x + dir_x * t
            if 0 <= bx < model.width:
                boundary_points.append((int(bx), 0))
    if not boundary_points:
        return None, None
    closest = min(boundary_points, key=lambda p: ((p[0] - x) ** 2 + (p[1] - y) ** 2))
    mid_x = int(x + (closest[0] - x) * bulge_fract)
    mid_y = int(y + (closest[1] - y) * bulge_fract)
    return mid_x, mid_y

def fillPolygon (m, model, vertices):
    if len(vertices) < 3:
        return
    min_y = min(v[1] for v in vertices)
    max_y = max(v[1] for v in vertices)
    filled_points = []
    for y in range(min_y, max_y + 1):
        intersections = []
        for i in range(len(vertices)):
            v1 = vertices[i]
            v2 = vertices[(i + 1) % len(vertices)]
            if v1[1] != v2[1]:
                if min(v1[1], v2[1]) <= y < max(v1[1], v2[1]) or (y == max(v1[1], v2[1]) and v1[1] != v2[1]):
                    x = int(v1[0] + (y - v1[1]) * (v2[0] - v1[0]) / (v2[1] - v1[1]))
                    intersections.append(x)
        intersections.sort()
        for i in range(0, len(intersections) - 1, 2):
            for x in range(intersections[i], intersections[i + 1] + 1):
                if model.VALID_COORDS(x, y):
                    filled_points.append((x, y))
    return filled_points

#================================================================================================================================#

def makePangeaBlob (m, model, radius_size):
    model.clearHighlights ()
    max_radius = min(model.width, model.height) // 2
    radius = max_radius * radius_size
    radius_squared = radius ** 2
    center_x, center_y = model.width // 2, model.height // 2
    continent_id = 0
    for y in range(model.height):
        for x in range(model.width):
            model.getMetadata(x, y).setTempTag(0)
            dist = (x - center_x) ** 2 + (y - center_y) ** 2
            if dist <= radius_squared:
                model.getMetadata(x, y).setContinent(continent_id)
                model.setTerrain(x, y, MapImgModel.TERRAIN_GRASSLAND)
            else:
                model.getMetadata(x, y).setContinent(None)
                model.setTerrain(x, y, MapImgModel.TERRAIN_OCEAN)
    model.highlightPixels ([(center_x, center_y)])
    model.setNextContinentId (1)

def selectPangeaCoastLinePoints (m, model):
    for y in range(model.height):
        for x in range(model.width):
            model.getMetadata(x, y).setTempTag(0)
    model.clearHighlights()
    points = []
    x, y = getRandomPangeaCoastTile(m, model)
    points.append((x, y))
    while True:
        steps = random.randint(20, 60)
        x, y = walkPangeaCoastLineToNewCoastTile(m, model, x, y, steps)
        if x is None or y is None:
            break
        points.append((x, y))
    model.highlightPixels(points)
    return points

def makePangeaBulge (m, model, p1, p2, bulge_fract):
    rp1 = getPangeaDistantOffShorePoint (m, model, p1[0], p1[1], bulge_fract)
    rp2 = getPangeaDistantOffShorePoint (m, model, p2[0], p2[1], bulge_fract)
    if rp1 is None or rp2 is None:
        return
    polygon_vertices = [p1, p2, rp2, rp1]
    all_points = []
    edges = [(p1, rp1), (p2, rp2), (rp1, rp2), (p1, p2)]
    for edge_start, edge_end in edges:
        line_points = drawLineBetweenPoints(m, model, edge_start[0], edge_start[1], edge_end[0], edge_end[1])
        all_points.extend(line_points)
    fill_points = fillPolygon(m, model, polygon_vertices)
    if fill_points:
        all_points.extend(fill_points)
    for px, py in all_points:
        if model.VALID_COORDS(px, py):
            model.setTerrain(px, py, MapImgModel.TERRAIN_GRASSLAND)
            model.getMetadata(px, py).setContinent(0)

#================================================================================================================================#
#=> - Continent shaping functions
#================================================================================================================================#


#================================================================================================================================#
#=> - Class: MapGenGUI
#================================================================================================================================#

class MapGenGUI:

    STAGE_PANGEA = 0
    STAGE_SPLIT = 1

    def __init__ (m, root, map_width, map_height, zoom_factor=1.0):
        m.map_width = map_width
        m.map_height = map_height
        m.zoom_factor = zoom_factor
        m.model = MapImgModel(map_width, map_height)
        m.current_stage = m.STAGE_PANGEA
        m.current_continent = 0
        
        #=> Pangea stage variables <=============================================================================================#
        m.pangea_coast_line_pts = []
        m.pangea_coast_line_pt_idx = -1
        m.pangea_rad_sizes = [0.3, 0.4, 0.5, 0.6, 0.7, 0.8]
        m.pangea_rad_sizes_idx = 2
        m.pangea_bulge_fract = [0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8]
        m.pangea_bulge_fract_idx = 2

        m.main_frame = tk.Frame(root)
        m.main_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        m.sidebar = tk.Frame(m.main_frame, width=300, bg="lightgray")
        m.sidebar.pack(side=tk.LEFT, fill=tk.Y)
        m.sidebar.pack_propagate(False)
        
        m.finish_button = tk.Button(m.sidebar, text="Finish stage", command=m.__finishStage, width=20)
        m.finish_button.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.continent_frame = tk.Frame(m.sidebar)
        m.continent_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.continent_prev = tk.Button(m.continent_frame, text="<", command=m.__prevContinent, width=1)
        m.continent_prev.pack(side=tk.LEFT)
        m.continent_next = tk.Button(m.continent_frame, text=">", command=m.__nextContinent, width=1)
        m.continent_next.pack(side=tk.LEFT)
        m.continent_label = tk.Label(m.continent_frame, text="Continent: -", width=12)
        m.continent_label.pack(side=tk.LEFT, padx=5)
        
        m.pangea_frame = tk.Frame(m.sidebar)
        m.split_frame = tk.Frame(m.sidebar)
        
        m.__setupPangeaStage()
        m.__setupSplitStage()
        
        m.pangea_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.split_frame.pack_forget()
        
        m.canvas_frame = tk.Frame(m.main_frame)
        m.canvas_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        scaled_width = int(map_width * zoom_factor)
        scaled_height = int(map_height * zoom_factor)
        m.canvas = tk.Canvas(m.canvas_frame, width=scaled_width, height=scaled_height)
        m.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.canvas.configure(scrollregion=(0, 0, scaled_width, scaled_height))
        
        m.scrollbar_y = tk.Scrollbar(m.canvas_frame, orient=tk.VERTICAL, command=m.canvas.yview)
        m.scrollbar_y.pack(side=tk.RIGHT, fill=tk.Y)
        m.canvas.configure(yscrollcommand=m.scrollbar_y.set)
        
        m.scrollbar_x = tk.Scrollbar(root, orient=tk.HORIZONTAL, command=m.canvas.xview)
        m.scrollbar_x.pack(side=tk.BOTTOM, fill=tk.X)
        m.canvas.configure(xscrollcommand=m.scrollbar_x.set)
        
        m.__updateDisplay()

        m.continents = m.model.getContinentSet()

    def __setupPangeaStage (m):
        m.generate_frm = tk.Frame(m.pangea_frame)
        m.generate_frm.pack(side=tk.TOP, fill=tk.X, pady=2)
        m.generate_btn = tk.Button(m.generate_frm, text="Generate Blob", command=m.__generatePangeaBlob, width=20)
        m.generate_btn.pack(side=tk.LEFT, fill=tk.X, pady=2)
        tag = m.pangea_rad_sizes[m.pangea_rad_sizes_idx]
        m.gen_rad_btn = tk.Button(m.generate_frm, text=str(tag), command=m.__togglePangeaRadialSize, width=10)
        m.gen_rad_btn.pack(side=tk.LEFT, fill=tk.X, pady=2)

        m.select_frm = tk.Frame(m.pangea_frame)
        m.select_frm.pack(side=tk.TOP, fill=tk.X, pady=2)
        m.select_btn = tk.Button(m.select_frm, text="Select points", command=m.__selectPangeaCoastLinePoints, width=20)
        m.select_btn.pack(side=tk.LEFT, fill=tk.X, pady=2)
        m.select_pair_btn = tk.Button(m.select_frm, text="Toggle pairs", command=m.__selectPangeaCoastLinePair, width=10)
        m.select_pair_btn.pack(side=tk.LEFT, fill=tk.X, pady=2)

        m.bulge_frm = tk.Frame(m.pangea_frame)
        m.bulge_frm.pack(side=tk.TOP, fill=tk.X, pady=2)
        m.bulge_btn = tk.Button(m.bulge_frm, text="Smooth Edges", command=m.__addPangeaBulge, width=20)
        m.bulge_btn.pack(side=tk.LEFT, fill=tk.X, pady=2)
        tag = m.pangea_bulge_fract[m.pangea_bulge_fract_idx]
        m.bulge_fract_btn = tk.Button(m.bulge_frm, text=str(tag), command=m.__togglePangeaBulgeFract, width=10)
        m.bulge_fract_btn.pack(side=tk.LEFT, fill=tk.X, pady=2)

    def __setupSplitStage (m):
        split_btn1 = tk.Button(m.split_frame, text="Draw Split Line", command=m.__drawSplitLine, width=20)
        split_btn1.pack(side=tk.TOP, fill=tk.X, pady=2)
        split_btn2 = tk.Button(m.split_frame, text="Separate Continents", command=m.__separateContinents, width=20)
        split_btn2.pack(side=tk.TOP, fill=tk.X, pady=2)
        split_btn3 = tk.Button(m.split_frame, text="Rotate Continent", command=m.__rotateContinent, width=20)
        split_btn3.pack(side=tk.TOP, fill=tk.X, pady=2)
        split_btn4 = tk.Button(m.split_frame, text="Add Islands", command=m.__addIslands, width=20)
        split_btn4.pack(side=tk.TOP, fill=tk.X, pady=2)

    def __finishStage (m):
        if m.current_stage == m.STAGE_PANGEA:
            m.current_stage = m.STAGE_SPLIT
            m.pangea_frame.pack_forget()
            m.split_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        else:
            m.current_stage = m.STAGE_PANGEA
            m.split_frame.pack_forget()
            m.pangea_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

    def __prevContinent (m):
        if m.current_continent >= 0:
            m.current_continent -= 1
        else:
            m.current_continent = len(m.continents) - 1
        m.__updateContinentLabel()

    def __nextContinent (m):
        if m.current_continent < len(m.continents) - 1:
            m.current_continent += 1
        else:
            m.current_continent = -1
        m.__updateContinentLabel()

    def __updateContinentLabel (m):
        continent_label = "-" if m.current_continent == -1 else m.continents[m.current_continent]
        m.continent_label.config(text="Continent: " + str(continent_label))

    def __updateDisplay (m):
        zoom_percent = int(m.zoom_factor * 100)
        m.model.displayOnCanvas(m.canvas, zoom_percent)

    #============================================================================================================================#

    def __togglePangeaRadialSize (m):
        m.pangea_rad_sizes_idx = (m.pangea_rad_sizes_idx + 1) % len(m.pangea_rad_sizes)
        m.gen_rad_btn.config(text=str(m.pangea_rad_sizes[m.pangea_rad_sizes_idx]))

    def __selectPangeaCoastLinePair (m):
        m.pangea_coast_line_pt_idx = (m.pangea_coast_line_pt_idx + 1) % len(m.pangea_coast_line_pts)
        pt1 = m.pangea_coast_line_pts[m.pangea_coast_line_pt_idx]
        pt2 = m.pangea_coast_line_pts[(m.pangea_coast_line_pt_idx + 1) % len(m.pangea_coast_line_pts)]
        m.model.clearHighlights()
        m.model.highlightPixels([(pt1[0], pt1[1]), (pt2[0], pt2[1])])
        m.__updateDisplay()

    def __togglePangeaBulgeFract (m):
        m.pangea_bulge_fract_idx = (m.pangea_bulge_fract_idx + 1) % len(m.pangea_bulge_fract)
        m.bulge_fract_btn.config(text=str(m.pangea_bulge_fract[m.pangea_bulge_fract_idx]))

    def __generatePangeaBlob (m):
        makePangeaBlob (m, m.model, m.pangea_rad_sizes[m.pangea_rad_sizes_idx])
        m.__updateDisplay()

    def __selectPangeaCoastLinePoints (m):
        m.pangea_coast_line_pts = selectPangeaCoastLinePoints (m, m.model)
        m.__updateDisplay()

    def __addPangeaBulge (m):
        if len(m.pangea_coast_line_pts) < 2:
            return
        p1 = m.pangea_coast_line_pts[m.pangea_coast_line_pt_idx]
        p2 = m.pangea_coast_line_pts[(m.pangea_coast_line_pt_idx + 1) % len(m.pangea_coast_line_pts)]
        makePangeaBulge (m, m.model, p1, p2, m.pangea_bulge_fract[m.pangea_bulge_fract_idx])
        m.__updateDisplay()

    #============================================================================================================================#

    def __addIslands (m):
        pass

    def __drawSplitLine (m):
        pass

    def __separateContinents (m):
        pass

    def __rotateContinent (m):
        pass

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title ("Map Generator")
    gui = MapGenGUI (root, 300, 200, zoom_factor=4.0)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
