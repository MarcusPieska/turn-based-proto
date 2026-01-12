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
#=> - Class: NavFrame
#================================================================================================================================#

class NavFrame:

    def __init__ (m, parent, map_gen, on_image_changed_cb):
        m.parent = parent
        m.map_gen = map_gen
        m.on_image_changed_cb = on_image_changed_cb
        m.frame = tk.Frame(parent)
        
        m.prev_btn = tk.Button(m.frame, text="<", command=m.__prevImage, width=2)
        m.prev_btn.pack(side=tk.LEFT, padx=5)
        
        m.index_label = tk.Label(m.frame, text="Image: 0/0")
        m.index_label.pack(side=tk.LEFT, padx=5)
        
        m.next_btn = tk.Button(m.frame, text=">", command=m.__nextImage, width=2)
        m.next_btn.pack(side=tk.LEFT, padx=5)

    def show (m, before_widget):
        m.frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5, before=before_widget)
        m.update()

    def hide (m):
        m.frame.pack_forget()

    def update (m):
        count = m.map_gen.getCount()
        current_idx = m.map_gen.getCurrentIndex()
        if count > 0:
            m.index_label.config(text="Image: " + str(current_idx + 1) + "/" + str(count))
        else:
            m.index_label.config(text="Image: 0/0")

    def __prevImage (m):
        m.map_gen.incrementIndex (backwards=True)
        m.on_image_changed_cb ()
        m.update ()

    def __nextImage (m):
        m.map_gen.incrementIndex ()
        m.on_image_changed_cb ()
        m.update ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
