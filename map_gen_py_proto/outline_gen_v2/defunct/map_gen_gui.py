#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
import math
import numpy as np
from PIL import Image, ImageTk
from map_gen import MapGenerator
from outline_gen_gui_phase_land_shaping import LandShapingMenuFrame
from outline_gen_gui_phase_fine_tuning import FineTuningMenuFrame
from outline_gen_gui_phase_climate_shaping import ClimateMenuFrame
from outline_gen_gui_phase_resource_placement import ResourceMenuFrame
from outline_gen_gui_frm_nav import NavFrame

#================================================================================================================================#
#=> - Class: MapGenGUI
#================================================================================================================================#

class MapGenGUI:

    def __drawCnv (m):
        m.canvas.delete("all")
        state = m.map_gen.getCurrent()
        if not state:
            return
        current = state.generator
        if not current.angles:
            return
        
        center_x = current.canvas_width * current.zoom // 2
        center_y = current.canvas_height * current.zoom // 2
        max_radius = min(current.canvas_width, current.canvas_height) * current.zoom * 0.45
        
        rotation = m.map_gen.getRotation()
        for i, angle in enumerate(current.angles):
            rotated_angle = (angle + rotation) % 360.0
            angle_rad = math.radians(rotated_angle)
            end_x = center_x + max_radius * math.cos(angle_rad)
            end_y = center_y - max_radius * math.sin(angle_rad)
            color = "#%02x%02x%02x" % (200, 200, 200)
            m.canvas.create_line(center_x, center_y, end_x, end_y, fill=color, width=1)
            
            if i < len(current.depths):
                depth = current.depths[i]
                depth_radius = max_radius * depth
                depth_x = center_x + depth_radius * math.cos(angle_rad)
                depth_y = center_y - depth_radius * math.sin(angle_rad)
                m.canvas.create_oval(depth_x - 2, depth_y - 2, depth_x + 2, depth_y + 2, fill="blue", outline="blue")

    def __drawImg (m, image_matrix):
        m.canvas.delete("all")
        result_image = image_matrix.copy()
        current_idx = m.map_gen.getCurrentIndex()
        white = np.array([255, 255, 255], dtype=np.uint8)
        img_h, img_w, _ = result_image.shape
        for i, state in enumerate(m.map_gen.image_states):
            if i != current_idx:
                gen = state.generator
                outline_pixels = gen.getAllOutlinePixels()
                if outline_pixels:
                    for px, py in outline_pixels:
                        img_x = px + int(state.offset_x)
                        img_y = py + int(state.offset_y)
                        if 0 <= img_x < img_w and 0 <= img_y < img_h:
                            result_image[img_y, img_x] = white
        
        img = Image.fromarray(result_image, mode='RGB')
        photo = ImageTk.PhotoImage(img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo

    def __onImageChanged (m):
        m.nav_frame.update()
        state = m.map_gen.getCurrent()
        if not state:
            m.canvas.delete("all")
            return
        if state.generator.points:
            m.map_gen.getOffsetImage (callback=m.__drawImg)
        else:
            m.__drawCanvas()

    def __setLandShapingFrm (m):
        m.curr_phase_frm = LandShapingMenuFrame (m.phase_frm, m.map_gen, m.canvas, m.__setTuningFrm, m.__drawCnv, m.__drawImg)
        m.curr_phase_frm.show ()
        m.curr_phase_frm.initialize ()

    def __setTuningFrm (m):
        m.curr_phase_frm.hide ()
        m.curr_phase_frm = FineTuningMenuFrame (m.phase_frm, m.map_gen, m.canvas, m.__setClimateFrm, m.__drawCnv, m.__drawImg)
        m.curr_phase_frm.show ()
        m.curr_phase_frm.initialize ()
        
    def __setClimateFrm (m):
        m.curr_phase_frm.hide ()
        m.curr_phase_frm = ClimateMenuFrame (m.phase_frm, m.map_gen, m.canvas, m.__setResourceFrm, m.__drawCnv, m.__drawImg)
        m.curr_phase_frm.show ()
        m.curr_phase_frm.initialize ()

    def __setResourceFrm (m):
        m.curr_phase_frm.hide ()
        m.curr_phase_frm = ResourceMenuFrame (m.phase_frm, m.map_gen, m.canvas, m.__concludePhase, m.__drawCnv, m.__drawImg)
        m.curr_phase_frm.show ()
        m.curr_phase_frm.initialize ()

    def __concludePhase (m):
        m.curr_phase_frm.hide ()
        m.curr_phase_frm = None
        print ("*** Phase concluded")

    def __init__ (m, root, width=200, height=100, zoom=4.0):
        m.root = root
        m.map_gen = MapGenerator (canvas_width=200, canvas_height=100, zoom=8.0)
        
        m.phase_frm = tk.Frame(root)
        m.phase_frm.pack(side=tk.TOP, fill=tk.X)
        
        height = m.map_gen.canvas_height * m.map_gen.zoom
        width = m.map_gen.canvas_width * m.map_gen.zoom
        m.canvas = tk.Canvas (root, width=width, height=height, bg="white")
        m.canvas.pack (side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.nav_frame = NavFrame(root, m.map_gen, m.__onImageChanged)
        m.nav_frame.show(m.canvas)
        
        m.__setLandShapingFrm ()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Map Generator")
    width, height, zoom_factor = 1800, 800, 1.0
    gui = MapGenGUI(root, width=width, height=height, zoom=zoom_factor)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
