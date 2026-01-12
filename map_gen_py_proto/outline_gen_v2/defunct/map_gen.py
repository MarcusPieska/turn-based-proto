#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
from outline_gen import OutlineGenerator

#================================================================================================================================#
#=> - Class: MapGenerator
#================================================================================================================================#

class MapGenerator:

    class ImageState:

        def __init__ (m, index, generator, image):
            m.index = index
            m.generator = generator
            m.image = image
            m.percent_x, m.percent_y = 0.0, 0.0
            m.seed = 0
            m.offset_x, m.offset_y = 0, 0

    def __init__ (m, canvas_width=200, canvas_height=100, zoom=8.0):
        m.canvas_width = canvas_width
        m.canvas_height = canvas_height
        m.zoom = zoom
        m.image_states = []
        m.current_index = -1

    #============================================================================================================================#
    #=> - Internal helpers -
    #============================================================================================================================#

    def __calculateMargins (m, horizontal=True):
        if not m.getPoints():
            return 0, 0
        coord, total = (0, m.canvas_width) if horizontal else (1, m.canvas_height)
        m1 = min(m.getPoints(), key=lambda x: x[coord])[coord]
        m2 = max(m.getPoints(), key=lambda x: x[coord])[coord]
        return m1 * m.zoom, (total - m2) * m.zoom

    #============================================================================================================================#
    #=> - Public interface -
    #============================================================================================================================#

    def addNew (m):
        gen_new = OutlineGenerator (canvas_width=m.canvas_width, canvas_height=m.canvas_height, zoom=m.zoom)
        gen_new.generateAngles ()
        gen_new.generateDepths ()
        image_matrix = gen_new.generateLines ()
        state = m.ImageState(len(m.image_states), gen_new, image_matrix.copy())
        m.image_states.append(state)
        m.current_index = len(m.image_states) - 1
        return m.current_index

    def getCurrent (m):
        return m.image_states[m.current_index]

    def incrementIndex (m, backwards=False):
        m.current_index += -1 if backwards else 1
        m.current_index %= len(m.image_states)

    def getCurrentIndex (m):
        return m.current_index

    def getCount (m):
        return len(m.image_states)

    def getPoints (m):
        return m.getCurrent().generator.points

    def getPercentX (m):
        return m.getCurrent().percent_x

    def getPercentY (m):
        return m.getCurrent().percent_y

    def getOffsetImage (m, callback=None):
        state = m.getCurrent()
        original = state.image
        offset_x, offset_y = int(state.offset_x), int(state.offset_y)
        img_h, img_w, _ = original.shape
        result = np.full((img_h, img_w, 3), np.array([32, 26, 120], dtype=np.uint8), dtype=np.uint8)
        
        src_y_start = int(max(0, -offset_y))
        src_y_end = int(min(img_h, img_h - offset_y))
        src_x_start = int(max(0, -offset_x))
        src_x_end = int(min(img_w, img_w - offset_x))
        
        dst_y_start = int(max(0, offset_y))
        dst_y_end = int(dst_y_start + (src_y_end - src_y_start))
        dst_x_start = int(max(0, offset_x))
        dst_x_end = int(dst_x_start + (src_x_end - src_x_start))
        
        if src_y_end > src_y_start and src_x_end > src_x_start:
            result[dst_y_start:dst_y_end, dst_x_start:dst_x_end] = original[src_y_start:src_y_end, src_x_start:src_x_end]
        
        if callback:
            callback(result)
        return result

    #============================================================================================================================#
    #=> - Shaping functions -
    #============================================================================================================================#

    def generateAngles (m):
        m.getCurrent().generator.generateAngles()

    def generateDepths (m):
        m.getCurrent().generator.generateDepths()

    def generateLines (m, new_seed=False):
        state = m.getCurrent ()
        if new_seed:
            state.seed += 1
        state.generator.line_gen.setRandomSeed(state.seed)
        image_matrix = state.generator.generateLines()
        state.image = image_matrix.copy()
        if state.offset_x != 0 or state.offset_y != 0:
            image_matrix = m.getOffsetImage()
        return image_matrix

    def setRotation (m, rotation):
        state = m.getCurrent()
        state.generator.rotation = rotation
        if state.offset_x != 0 or state.offset_y != 0:
            image_matrix = m.getOffsetImage()
        if state.generator.points:
            state.generator.calculatePoints()

    def getRotation (m):
        return m.getCurrent().generator.rotation

    def hasPoints (m):
        return m.getCurrent().generator.points

    #============================================================================================================================#
    #=> - Move functions -
    #============================================================================================================================#

    def moveHorizontal (m, marginPerc=0.0):
        m.getCurrent().percent_x = marginPerc
        left_margin, right_margin = m.__calculateMargins (horizontal=True)
        offset_x = -left_margin * abs(marginPerc) if marginPerc < 0 else right_margin * marginPerc
        m.getCurrent().offset_x = offset_x

    def moveVertical (m, marginPerc=0.0):
        m.getCurrent().percent_y = marginPerc
        bottom_margin, top_margin = m.__calculateMargins (horizontal=False)
        offset_y = -bottom_margin * abs(marginPerc) if marginPerc < 0 else top_margin * marginPerc
        m.getCurrent().offset_y = offset_y

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
