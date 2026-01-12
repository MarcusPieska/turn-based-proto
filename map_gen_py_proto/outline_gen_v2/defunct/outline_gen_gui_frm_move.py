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

#================================================================================================================================#
#=> - Class: MoveFrame
#================================================================================================================================#

class MoveFrame:

    def __onXChange (m, value):
        m.move_x_cb (float(value))

    def __onYChange (m, value):
        m.move_y_cb (float(value))

    def __init__ (m, parent, move_x_cb, move_y_cb, show_image_cb):
        m.parent = parent
        m.move_x_cb = move_x_cb
        m.move_y_cb = move_y_cb
        m.show_image_cb = show_image_cb
        m.frame = tk.Frame(parent, height=SUBFRAME_HEIGHT)
        m.frame.pack_propagate(False)

        m.row1 = tk.Frame(m.frame)
        m.row1.pack(side=tk.TOP, fill=tk.X, padx=5, pady=0)
        
        m.x_label = tk.Label(m.row1, text="X:")
        m.x_label.pack(side=tk.LEFT, padx=5, pady=0)
        
        m.x_var = tk.DoubleVar(value=0.0)
        m.x_slider = tk.Scale(m.row1, from_=-1.0, to=1.0, orient=tk.HORIZONTAL, length=400, resolution=0.01)
        m.x_slider.configure(variable=m.x_var)
        m.x_slider.configure(command=m.__onXChange)
        m.x_slider.set(0)
        m.x_slider.pack(side=tk.LEFT, padx=5, pady=0, fill=tk.X, expand=True)
        
        m.row2 = tk.Frame(m.frame)
        m.row2.pack(side=tk.TOP, fill=tk.X, padx=5, pady=0)
        
        m.y_label = tk.Label(m.row2, text="Y:")
        m.y_label.pack(side=tk.LEFT, padx=5, pady=0)
        
        m.y_var = tk.DoubleVar(value=0.0)
        m.y_slider = tk.Scale(m.row2, from_=-1.0, to=1.0, orient=tk.HORIZONTAL, length=400, resolution=0.01)
        m.y_slider.configure(variable=m.y_var)
        m.y_slider.configure(command=m.__onYChange)
        m.y_slider.set(0)
        m.y_slider.pack(side=tk.LEFT, padx=5, pady=0, fill=tk.X, expand=True)

    def setSliders (m, x_percent, y_percent):
        m.x_slider.set(x_percent)
        m.y_slider.set(y_percent)

    def show (m, before_widget):
        m.frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5, before=before_widget)
        m.show_image_cb()

    def hide (m):
        m.frame.pack_forget()
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
