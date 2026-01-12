#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
import random
from line_gen import LineGenerator
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: LineGenGUI
#================================================================================================================================#

class LineGenGUI:

    def __init__ (m, root):
        m.root = root
        m.line_length = 0
        m.line_angle = 0
        m.bend_factor = 0.0
        m.peak_position = 0.5
        m.generator = LineGenerator()
        
        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.new_button = tk.Button(m.top_frame, text="New line", command=m.__newLine, width=10)
        m.new_button.pack(side=tk.LEFT, padx=5)
        
        m.label = tk.Label(m.top_frame, text="Length: 0")
        m.label.pack(side=tk.LEFT, padx=5)
        
        m.slider_frame = tk.Frame(root)
        m.slider_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.angle_label = tk.Label(m.slider_frame, text="Angle: 0")
        m.angle_label.pack(side=tk.LEFT, padx=5)
        
        m.angle_slider = tk.Scale(m.slider_frame, from_=0, to=360, orient=tk.HORIZONTAL, command=m.__onAngleChange, length=400)
        m.angle_slider.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(root, width=800, height=600, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.__newLine()

    def __newLine (m):
        m.line_length = random.randint(5, 50)
        m.bend_factor = random.uniform(0.0, 1.0)
        m.peak_position = random.uniform(0.2, 0.8)
        m.label.config(text="Length: " + str(m.line_length))
        m.__drawLine()

    def __onAngleChange (m, value):
        m.line_angle = int(float(value))
        m.angle_label.config(text="Angle: " + str(m.line_angle))
        m.__drawLine()

    def __drawLine (m):
        m.canvas.delete("all")
        if m.line_length == 0:
            return
        
        image_matrix = m.generator.generateLine(
            line_length=m.line_length,
            angle=m.line_angle,
            bend_factor=m.bend_factor,
            peak_position=m.peak_position,
            width=800,
            height=600,
            zoom=4.0
        )
        
        img = Image.fromarray(image_matrix, mode='L')
        photo = ImageTk.PhotoImage(img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Line Generator")
    gui = LineGenGUI(root)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
