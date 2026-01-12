#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from outline_gen_gui_frm_globals import SUBFRAME_HEIGHT
from outline_gen_gui_frm_globals import SUBFRAME_BTN_WIDTH

#================================================================================================================================#
#=> - Class: ManageFrame
#================================================================================================================================#

class ManageFrame:

    def __init__ (m, parent, gen_angles_cb, gen_depths_cb, gen_lines_cb, set_rotation_cb):
        m.parent = parent
        m.gen_angles_cb = gen_angles_cb
        m.gen_depths_cb = gen_depths_cb
        m.gen_lines_cb = gen_lines_cb
        m.set_rotation_cb = set_rotation_cb
        m.frame = tk.Frame(parent, height=SUBFRAME_HEIGHT)
        m.frame.pack_propagate(False)
        
        m.new_angles_btn = tk.Button(m.frame, text="New angles", command=m.__newAngles, width=SUBFRAME_BTN_WIDTH)
        m.new_angles_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_depths_btn = tk.Button(m.frame, text="New depths", command=m.__newDepths, width=SUBFRAME_BTN_WIDTH)
        m.new_depths_btn.pack(side=tk.LEFT, padx=5)
        
        m.new_lines_btn = tk.Button(m.frame, text="New lines", command=m.__newLines, width=SUBFRAME_BTN_WIDTH)
        m.new_lines_btn.pack(side=tk.LEFT, padx=5)
        
        m.rotation_label = tk.Label(m.frame, text="Rotation:")
        m.rotation_label.pack(side=tk.LEFT, padx=(10, 5))
        
        m.rotation_var = tk.DoubleVar(value=0.0)
        m.rotation_slider = tk.Scale(m.frame, from_=0, to=360, orient=tk.HORIZONTAL, length=200, resolution=1.0)
        m.rotation_slider.configure(variable=m.rotation_var)
        m.rotation_slider.configure(command=m.__onRotationChange)
        m.rotation_slider.pack(side=tk.LEFT, padx=5)

    def show (m, before_widget):
        m.frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5, before=before_widget)

    def hide (m):
        m.frame.pack_forget()

    def updateRotation (m, rotation):
        m.rotation_slider.set(rotation)

    def __onRotationChange (m, value):
        m.set_rotation_cb(float(value))

    def __newAngles (m):
        m.gen_angles_cb()

    def __newDepths (m):
        m.gen_depths_cb()

    def __newLines (m):
        m.gen_lines_cb(new_seed=True)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
