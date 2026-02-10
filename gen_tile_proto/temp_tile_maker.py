#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import math
import tkinter as tk
from PIL import Image, ImageTk

#================================================================================================================================#
#=> - Class: TempTileMaker
#================================================================================================================================#

class TempTileMaker:

    def __buildUi (m):
        m.main_frame = tk.Frame(m.root)
        m.main_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)

        m.tag_frame = tk.Frame(m.main_frame)
        m.tag_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        tk.Label(m.tag_frame, text="Tag:").pack(side=tk.LEFT, padx=5)
        m.tag_entry = tk.Entry(m.tag_frame, width=30)
        m.tag_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)

        m.rgb_frame = tk.Frame(m.main_frame)
        m.rgb_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        tk.Label(m.rgb_frame, text="RGB:").pack(side=tk.LEFT, padx=5)
        m.rgb_entry = tk.Entry(m.rgb_frame, width=30)
        m.rgb_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        m.rgb_entry.bind("<KeyRelease>", m.__onRgbChange)

        m.confirm_frame = tk.Frame(m.main_frame)
        m.confirm_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        tk.Label(m.confirm_frame, text="Parsed:").pack(side=tk.LEFT, padx=5)
        m.confirm_entry = tk.Entry(m.confirm_frame, width=30)
        m.confirm_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        m.confirm_entry.config(state="readonly")

        m.canvas_frame = tk.Frame(m.main_frame)
        m.canvas_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canvas = tk.Canvas(m.canvas_frame, width=200, height=150, bg="gray")
        m.canvas.pack(side=tk.TOP, padx=5, pady=5)

        m.button_frame = tk.Frame(m.main_frame)
        m.button_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.save_button = tk.Button(m.button_frame, text="Save Tile", command=m.__saveTile, width=15)
        m.save_button.pack(side=tk.LEFT, padx=5)

        m.__drawTile()

    def __parseRgb (m, rgb_string):
        try:
            parts = rgb_string.split(",")
            if len(parts) != 3:
                return None
            r = int(parts[0].strip())
            g = int(parts[1].strip())
            b = int(parts[2].strip())
            if 0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255:
                return (r, g, b)
            return None
        except:
            return None

    def __onRgbChange (m, event):
        rgb_string = m.rgb_entry.get()
        rgb = m.__parseRgb(rgb_string)
        if rgb is not None:
            m.current_rgb = rgb
            m.confirm_entry.config(state="normal")
            m.confirm_entry.delete(0, tk.END)
            m.confirm_entry.insert(0, f"({rgb[0]}, {rgb[1]}, {rgb[2]})")
            m.confirm_entry.config(state="readonly")
            m.__drawTile()
        else:
            m.confirm_entry.config(state="normal")
            m.confirm_entry.delete(0, tk.END)
            m.confirm_entry.insert(0, "Invalid RGB")
            m.confirm_entry.config(state="readonly")

    def __createDiamondTile (m, width, height, color):
        img = Image.new("RGB", (width, height), "black")
        pixels = img.load()
        center_x = width // 2
        center_y = height // 2
        for y in range(height):
            for x in range(width):
                dx = abs(x - center_x)
                dy = abs(y - center_y)
                if dx * height + dy * width < width * height // 2:
                    pixels[x, y] = color
        return img

    def __drawTile (m):
        m.canvas.delete("all")
        if hasattr(m, 'current_rgb') and m.current_rgb is not None:
            img = m.__createDiamondTile(100, 50, m.current_rgb)
            img_resized = img.resize((200, 100), Image.NEAREST)
            photo = ImageTk.PhotoImage(img_resized)
            m.canvas.create_image(100, 75, image=photo, anchor=tk.CENTER)
            m.canvas.image = photo

    def __saveTile (m):
        if not hasattr(m, 'current_rgb') or m.current_rgb is None:
            return
        tag = m.tag_entry.get().strip()
        if not tag:
            return
        
        img = m.__createDiamondTile(100, 50, m.current_rgb)
        save_path = f"../../DEL_temp_tile_{tag}.png"
        img.save(save_path)
        print(f"Tile saved to: {save_path}")

    def __init__ (m, root):
        m.root = root
        m.current_rgb = None
        m.__buildUi()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Temp Tile Maker")
    maker = TempTileMaker(root)
    root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
