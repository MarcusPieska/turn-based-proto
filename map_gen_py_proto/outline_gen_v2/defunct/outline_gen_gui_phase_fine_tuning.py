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
#=> - Class: FineTuningMenuFrame
#================================================================================================================================#

class FineTuningMenuFrame:

    def __init__ (m, parent, map_gen, canvas, next_phase_cb, draw_canvas_cb, draw_image_matrix_cb):
        m.parent = parent
        m.map_gen = map_gen
        m.canvas = canvas
        m.draw_canvas_cb = draw_canvas_cb
        m.draw_image_matrix_cb = draw_image_matrix_cb
        m.frame = tk.Frame (parent)
        
        #==> Other buttons <=======================================================================================================#
        m.next_phase_btn = tk.Button(m.frame, text="Next phase", command=next_phase_cb, width=12)
        m.next_phase_btn.pack(side=tk.RIGHT, padx=5)

    def show (m):
        siblings = m.parent.pack_slaves()
        m.frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5, before=None if len(siblings) == 0 else siblings[0])

    def hide (m):
        m.frame.pack_forget()

    def initialize (m):
        pass
        
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
