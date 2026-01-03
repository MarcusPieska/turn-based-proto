#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
from PIL import Image
import tkinter as tk

#================================================================================================================================#
#=> - Class: TileMetadata
#================================================================================================================================#

class TileMetadata:

    def __init__ (m):
        m.temp_tag = 0
        m.continent = None
        m.forrest = False
        m.river = False

    def getTempTag (m):
        return m.temp_tag

    def setTempTag (m, temp_tag):
        m.temp_tag = temp_tag

    def getContinent (m):
        return m.continent

    def setContinent (m, continent):
        m.continent = continent

    def makeForest (m):
        m.forrest = True

    def removeForest (m):
        m.forrest = False

    def makeRiver (m):
        m.river = True

    def removeRiver (m):
        m.river = False

#================================================================================================================================#
#=> - Class: MapImgModel
#================================================================================================================================#

class MapImgModel:

    HIGHLIGHT_COLOR = (255, 255, 255)

    TERRAIN_OCEAN = 0
    TERRAIN_SEA = 1
    TERRAIN_COAST = 2
    TERRAIN_GRASSLAND = 3
    TERRAIN_GRASSLAND_HILLS = 4
    TERRAIN_PLAINS = 5
    TERRAIN_PLAINS_HILLS = 6
    TERRAIN_DESERT = 7
    TERRAIN_DESERT_HILLS = 8
    TERRAIN_TUNDRA = 9
    TERRAIN_TUNDRA_HILLS = 10
    TERRAIN_SNOW = 11
    TERRAIN_SNOW_HILLS = 12
    TERRAIN_MOUNTAIN = 13

    TERRAIN_COLORS = {
        TERRAIN_OCEAN: (0, 0, 139),
        TERRAIN_SEA: (0, 0, 205),
        TERRAIN_COAST: (176, 196, 222),
        TERRAIN_GRASSLAND: (34, 139, 34),
        TERRAIN_GRASSLAND_HILLS: (0, 100, 0),
        TERRAIN_PLAINS: (238, 203, 173),
        TERRAIN_PLAINS_HILLS: (205, 133, 63),
        TERRAIN_DESERT: (238, 232, 170),
        TERRAIN_DESERT_HILLS: (210, 180, 140),
        TERRAIN_TUNDRA: (245, 245, 220),
        TERRAIN_TUNDRA_HILLS: (192, 192, 192),
        TERRAIN_SNOW: (255, 250, 250),
        TERRAIN_SNOW_HILLS: (211, 211, 211),
        TERRAIN_MOUNTAIN: (139, 69, 19),
    }

    def VALID_COORDS (m, x, y):
        return x >= 0 and x < m.width and y >= 0 and y < m.height

    def __init__ (m, width, height):
        m.next_continent_id = 0
        m.width = width
        m.height = height
        m.terrain_map = np.zeros((height, width), dtype=np.uint8)
        m.metadata_map = np.zeros((height, width), dtype=object)
        m.highlight_map = np.zeros((height, width), dtype=np.uint8)
        for y in range(height):
            for x in range(width):
                m.metadata_map[y, x] = TileMetadata()

    def getTerrain (m, x, y):
        if not m.VALID_COORDS(x, y):
            return None
        return m.terrain_map[y, x]

    def setTerrain (m, x, y, terrain_type):
        if not m.VALID_COORDS(x, y):
            return False
        m.terrain_map[y, x] = terrain_type
        return True

    def getMetadata (m, x, y):
        if not m.VALID_COORDS(x, y):
            return None
        return m.metadata_map[y, x]

    def setMetadata (m, x, y, metadata):
        if not m.VALID_COORDS(x, y):
            return False
        m.metadata_map[y, x] = metadata
        return True

    def toImage (m):
        img_array = np.zeros((m.height, m.width, 3), dtype=np.uint8)
        for y in range(m.height):
            for x in range(m.width):
                terrain = m.terrain_map[y, x]
                img_array[y, x] = m.TERRAIN_COLORS.get(terrain, (0, 0, 0))
                if m.highlight_map[y, x] == 1:
                    img_array[y, x] = m.HIGHLIGHT_COLOR
        return Image.fromarray(img_array)

    def save (m, filename):
        img = m.toImage()
        img.save(filename)
        import pickle
        metadata_file = filename.replace('.png', '_metadata.pkl')
        with open(metadata_file, 'wb') as f:
            pickle.dump(m.metadata_map, f)

    def load (m, filename):
        img = Image.open(filename)
        img_array = np.array(img)
        m.width = img_array.shape[1]
        m.height = img_array.shape[0]
        m.terrain_map = np.zeros((m.height, m.width), dtype=np.uint8)
        color_to_terrain = {v: k for k, v in m.TERRAIN_COLORS.items()}
        for y in range(m.height):
            for x in range(m.width):
                pixel_color = tuple(img_array[y, x])
                terrain = color_to_terrain.get(pixel_color, m.TERRAIN_OCEAN)
                m.terrain_map[y, x] = terrain
        import pickle
        metadata_file = filename.replace('.png', '_metadata.pkl')
        if os.path.exists(metadata_file):
            with open(metadata_file, 'rb') as f:
                m.metadata_map = pickle.load(f)
        else:
            m.metadata_map = np.empty((m.height, m.width), dtype=object)
            for y in range(m.height):
                for x in range(m.width):
                    m.metadata_map[y, x] = TileMetadata()

    def displayOnCanvas (m, canvas, zoom=100):
        img = m.toImage()
        if zoom != 100:
            new_width = int(img.width * zoom / 100)
            new_height = int(img.height * zoom / 100)
            img = img.resize((new_width, new_height), Image.NEAREST)
        from PIL import ImageTk
        photo = ImageTk.PhotoImage(img)
        canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        canvas.image = photo

    def getContinentSet (m):
        continents = set()
        for y in range(m.height):
            for x in range(m.width):
                if m.getMetadata(x, y).getContinent() is not None:
                    continents.add(m.getMetadata(x, y).getContinent())
        return continents

    def getNextContinentId (m):
        return m.next_continent_id

    def setNextContinentId (m, next_continent_id):
        m.next_continent_id = next_continent_id

    def highlightPixels(m, points):
        for x, y in points:
            m.highlight_map[y, x] = 1

    def clearHighlights(m):
        m.highlight_map.fill(0)

#================================================================================================================================#
#=> - Test functions -
#================================================================================================================================#

def test_basic_operations ():
    model = MapImgModel(100, 100)
    assert model.width == 100
    assert model.height == 100
    assert model.getTerrain(50, 50) == MapImgModel.TERRAIN_OCEAN
    model.setTerrain(50, 50, MapImgModel.TERRAIN_GRASSLAND)
    assert model.getTerrain(50, 50) == MapImgModel.TERRAIN_GRASSLAND
    assert model.getTerrain(-1, 50) == None
    assert model.getTerrain(200, 50) == None
    print("*** Basic operations test passed")

def test_metadata ():
    model = MapImgModel(50, 50)
    metadata = model.getMetadata(25, 25)
    assert metadata is not None
    assert metadata.continent == None
    assert metadata.forrest == False
    metadata.continent = 1
    metadata.forrest = True
    assert model.getMetadata(25, 25).continent == 1
    assert model.getMetadata(25, 25).forrest == True
    print("*** Metadata test passed")

def test_image_conversion ():
    model = MapImgModel(10, 10)
    model.setTerrain(5, 5, MapImgModel.TERRAIN_GRASSLAND)
    model.setTerrain(3, 3, MapImgModel.TERRAIN_MOUNTAIN)
    img = model.toImage()
    assert img.size == (10, 10)
    assert img.mode == "RGB"
    print("*** Image conversion test passed")

def test_save_load ():
    model = MapImgModel(20, 20)
    model.setTerrain(10, 10, MapImgModel.TERRAIN_DESERT)
    metadata = model.getMetadata(10, 10)
    metadata.continent = 5
    test_file = "test_map.png"
    model.save(test_file)
    loaded_model = MapImgModel(1, 1)
    loaded_model.load(test_file)
    assert loaded_model.width == 20
    assert loaded_model.height == 20
    assert loaded_model.getTerrain(10, 10) == MapImgModel.TERRAIN_DESERT
    assert loaded_model.getMetadata(10, 10).continent == 5
    if os.path.exists(test_file):
        os.remove(test_file)
    metadata_file = test_file.replace('.png', '_metadata.pkl')
    if os.path.exists(metadata_file):
        os.remove(metadata_file)
    print("*** Save/load test passed")

def test_display ():
    width, height = 400, 400
    model = MapImgModel (width, height)
    for y in range (height):
        for x in range (width):
            if (x + y) % 40 < 20:
                model.setTerrain (x, y, MapImgModel.TERRAIN_GRASSLAND)
            elif (x + y) % 40 < 40:
                model.setTerrain (x, y, MapImgModel.TERRAIN_OCEAN)
    root = tk.Tk ()
    root.title ("Map Display Test")
    zoom = 200
    canvas = tk.Canvas (root, width=width * zoom // 100, height=height * zoom // 100)
    canvas.pack ()
    model.displayOnCanvas (canvas, zoom)
    print ("*** Display test - window should show checkerboard pattern")
    print ("*** Close window to continue")
    
    root.mainloop()

def test_all ():
    test_basic_operations()
    test_metadata()
    test_image_conversion()
    test_save_load()
    test_display()
    print("*** All tests completed")

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    test_all ()
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
