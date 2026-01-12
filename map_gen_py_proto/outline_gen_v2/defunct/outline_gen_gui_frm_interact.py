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
#=> - Class: InteractFrame
#================================================================================================================================#

class InteractFrame:

    def __init__ (m, parent, collide_cb, cut_cb):
        m.parent = parent
        m.collide_cb = collide_cb
        m.cut_cb = cut_cb
        m.frame = tk.Frame(parent, height=SUBFRAME_HEIGHT)
        m.frame.pack_propagate(False)

        m.row1 = tk.Frame(m.frame)
        m.row1.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.collide_btn = tk.Button(m.row1, text="Collide", command=m.collide_cb, width=12)
        m.collide_btn.pack(side=tk.LEFT, padx=5)
        
        m.cut_btn = tk.Button(m.row1, text="Cut", command=m.cut_cb, width=12)
        m.cut_btn.pack(side=tk.LEFT, padx=5)

    def show (m, before_widget):
        m.frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5, before=before_widget)

    def hide (m):
        m.frame.pack_forget()
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
