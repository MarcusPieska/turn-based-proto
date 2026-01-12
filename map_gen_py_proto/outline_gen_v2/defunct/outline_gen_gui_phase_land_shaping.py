#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from outline_gen_gui_frm_move import MoveFrame
from outline_gen_gui_frm_mng import ManageFrame
from outline_gen_gui_frm_interact import InteractFrame

#================================================================================================================================#
#=> - Class: LandShapingMenuFrame
#================================================================================================================================#

class LandShapingMenuFrame:

    def __updateDisplay (m):
        state = m.map_gen.getCurrent()
        if not state:
            m.draw_canvas_cb()
            return
        if state.generator.points:
            m.map_gen.getOffsetImage(callback=m.draw_image_matrix_cb)
        else:
            m.draw_canvas_cb()

    def __updateUI (m):
        current_frame = m.getCurrentFrame()
        if current_frame == "manage":
            m.__updateDisplay()
        elif current_frame == "interact":
            m.__updateDisplay()
        elif current_frame == "move":
            m.move_frame.setSliders(m.map_gen.getPercentX(), m.map_gen.getPercentY())
            m.map_gen.getOffsetImage(callback=m.draw_image_matrix_cb)

    def __toggleFrame (m, frame_name):
        if m.current_frame == frame_name:
            return
        clicked_pair = None
        for btn, frame, name in m.frame_pairs:
            if name == frame_name:
                clicked_pair = (btn, frame, name)
                break
        if not clicked_pair:
            return
        for btn, frame, name in m.frame_pairs:
            if name == frame_name:
                btn.config(state=tk.DISABLED)
                frame.show(None)
                if name == "manage":
                    m.manage_frame.updateRotation(m.map_gen.getRotation())
            else:
                btn.config(state=tk.NORMAL)
                frame.hide()
        m.current_frame = frame_name
        m.__updateUI()

    def __addNew (m):
        m.map_gen.addNew()
        m.__updateUI()
        if m.current_frame:
            m.__toggleFrame(m.current_frame)

    def __init__ (m, parent, map_gen, canvas, next_phase_cb, draw_canvas_cb, draw_image_matrix_cb):
        m.parent = parent
        m.map_gen = map_gen
        m.canvas = canvas
        m.draw_canvas_cb = draw_canvas_cb
        m.draw_image_matrix_cb = draw_image_matrix_cb
        m.current_frame = None
        m.frame = tk.Frame (parent)
        
        #==> ManageFrame <========================================================================================================#
        def generate_angles_cb ():
            m.map_gen.generateAngles ()
            m.draw_canvas_cb ()
        def generate_depths_cb ():
            m.map_gen.generateDepths ()
            m.draw_canvas_cb ()
        def generate_lines_cb (new_seed=False):
            m.map_gen.generateLines (new_seed)
            m.map_gen.getOffsetImage (callback=m.draw_image_matrix_cb)
        def set_rotation_cb (rotation):
            m.map_gen.setRotation (rotation)
            if m.map_gen.hasPoints ():
                m.map_gen.generateLines ()
                m.map_gen.getOffsetImage (callback=m.draw_image_matrix_cb)
            else:
                m.draw_canvas_cb ()
        m.manage_frame = ManageFrame (parent, generate_angles_cb, generate_depths_cb, generate_lines_cb, set_rotation_cb)
        m.manage_btn = tk.Button (m.frame, text="Manage", command=lambda: m.__toggleFrame("manage"), width=12)
        m.manage_btn.pack (side=tk.LEFT, padx=5)
        
        #==> InteractFrame <======================================================================================================#
        def collide_cb():
            pass
        def cut_cb():
            pass
        m.interact_frame = InteractFrame(parent, collide_cb, cut_cb)
        m.interact_btn = tk.Button(m.frame, text="Interact", command=lambda: m.__toggleFrame("interact"), width=12)
        m.interact_btn.pack(side=tk.LEFT, padx=5)
        
        #==> MoveFrame <==========================================================================================================#
        def move_x_cb(percent):
            m.map_gen.moveHorizontal(percent)
            m.map_gen.getOffsetImage(callback=m.draw_image_matrix_cb)
        def move_y_cb(percent):
            m.map_gen.moveVertical(percent)
            m.map_gen.getOffsetImage(callback=m.draw_image_matrix_cb)
        def show_image_cb():
            m.map_gen.getOffsetImage(callback=m.draw_image_matrix_cb)
        m.move_frame = MoveFrame(parent, move_x_cb, move_y_cb, show_image_cb)
        m.move_btn = tk.Button(m.frame, text="Move", command=lambda: m.__toggleFrame("move"), width=12)
        m.move_btn.pack(side=tk.LEFT, padx=5)
        
        #==> Other buttons <=======================================================================================================#
        m.add_new_btn = tk.Button(m.frame, text="Add new", command=m.__addNew, width=12)
        m.add_new_btn.pack(side=tk.LEFT, padx=5)
        
        m.next_phase_btn = tk.Button(m.frame, text="Next phase", command=next_phase_cb, width=12)
        m.next_phase_btn.pack(side=tk.RIGHT, padx=5)
        
        m.frame_pairs = [
            (m.manage_btn, m.manage_frame, "manage"),
            (m.interact_btn, m.interact_frame, "interact"),
            (m.move_btn, m.move_frame, "move")
        ]

    def show (m):
        siblings = m.parent.pack_slaves()
        m.frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5, before=None if len(siblings) == 0 else siblings[0])

    def hide (m):
        m.frame.pack_forget()
    
    def getCurrentFrame (m):
        return m.current_frame
    
    def initialize (m):
        m.map_gen.addNew()
        m.__toggleFrame("manage")

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
