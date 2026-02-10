#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from PIL import Image, ImageTk, ImageDraw
import math

#================================================================================================================================#
#=> - Global Parameters -
#================================================================================================================================#

SHADER_WIDTH = 102
SHADER_HEIGHT = 102
BUTTON_WIDTH = 12
CANVAS_WIDTH = 500
CANVAS_HEIGHT = 500

ROW_DISTANCE_MIN = 4
ROW_DISTANCE_MAX = 10

#================================================================================================================================#
#=> - Class: FarmShaderGen
#================================================================================================================================#

class FarmShaderGen:

    def __init__ (m, frame):
        m.frame = frame
        m.row_distance = 6
        
        m.shader_img = Image.new("L", (SHADER_WIDTH, SHADER_HEIGHT), 255)
        
        m.button_row = tk.Frame(frame)
        m.button_row.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        m.btn_generate = tk.Button(m.button_row, text="Generate", command=lambda: m.__generate_shader(), width=BUTTON_WIDTH)
        m.btn_generate.pack(side=tk.LEFT, padx=5)
        
        m.btn_decrease = tk.Button(m.button_row, text="<", command=lambda: m.__decrease_distance(), width=2)
        m.btn_decrease.pack(side=tk.LEFT, padx=5)
        
        m.btn_increase = tk.Button(m.button_row, text=">", command=lambda: m.__increase_distance(), width=2)
        m.btn_increase.pack(side=tk.LEFT, padx=5)
        
        m.label_distance = tk.Label(m.button_row, text=str(m.row_distance))
        m.label_distance.pack(side=tk.LEFT, padx=5)
        
        m.btn_save = tk.Button(m.button_row, text="Save", command=lambda: m.__save_shader(), width=BUTTON_WIDTH)
        m.btn_save.pack(side=tk.LEFT, padx=5)
        
        m.canvas = tk.Canvas(frame, width=CANVAS_WIDTH, height=CANVAS_HEIGHT, bg="white")
        m.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas.images = []
        
        m.__generate_shader()
        m.__update_canvas()

    def __decrease_distance (m):
        if m.row_distance > ROW_DISTANCE_MIN:
            m.row_distance -= 1
            m.label_distance.config(text=str(m.row_distance))
            m.__generate_shader()
            m.__update_canvas()

    def __increase_distance (m):
        if m.row_distance < ROW_DISTANCE_MAX:
            m.row_distance += 1
            m.label_distance.config(text=str(m.row_distance))
            m.__generate_shader()
            m.__update_canvas()

    def __save_shader (m):
        filename = "farm_row_shader_%dpx.png" % m.row_distance
        m.shader_img.save(filename)

    def __generate_shader (m):
        m.shader_img = Image.new("L", (SHADER_WIDTH, SHADER_HEIGHT), 255)
        pixels = m.shader_img.load()
        
        num_rows = SHADER_HEIGHT // m.row_distance
        remainder = SHADER_HEIGHT % m.row_distance
        top_padding = remainder // 2
        bottom_padding = remainder // 2
        
        period = m.row_distance
        
        for y in range(top_padding):
            for x in range(SHADER_WIDTH):
                pixels[x, y] = 128
        
        for row_idx in range(num_rows):
            row_start_y = top_padding + row_idx * m.row_distance
            for row_y in range(m.row_distance):
                y = row_start_y + row_y
                sine_value = math.sin((2 * math.pi * row_y) / period)
                grayscale = int(128 + 127 * sine_value)
                grayscale = max(0, min(255, grayscale))
                for x in range(SHADER_WIDTH):
                    pixels[x, y] = grayscale
        
        bottom_start_y = top_padding + num_rows * m.row_distance
        for y in range(bottom_start_y, SHADER_HEIGHT):
            for x in range(SHADER_WIDTH):
                pixels[x, y] = 128
        
        m.__update_canvas()
    
    def __update_canvas (m):
        m.canvas.delete("all")
        m.canvas.images = []
        
        display_img = m.shader_img.resize((CANVAS_WIDTH, CANVAS_HEIGHT), Image.NEAREST)
        photo = ImageTk.PhotoImage(display_img)
        m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.images.append(photo)

#================================================================================================================================#
#=> - Class: ShaderApp
#================================================================================================================================#

class ShaderApp:

    def __init__ (m, root):
        m.root = root
        
        m.main_frame = tk.Frame(root)
        m.main_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        m.shader_gen = FarmShaderGen(m.main_frame)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Farm Shader Generator")
    shader_app = ShaderApp(root)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
