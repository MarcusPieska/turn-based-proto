#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

from PIL import Image, ImageTk
import tkinter as tk

#================================================================================================================================#
#=> - Class: ForestDecalLoader -
#================================================================================================================================#

class ForestDecalLoader:
    
    def __init__(m, canvas_size=600):
        m.canvas_size = canvas_size
        m.root = tk.Tk()
        m.root.title("Forest Decal Loader")
        m.canvas = tk.Canvas(m.root, width=canvas_size, height=canvas_size, bg="white")
        m.canvas.pack(padx=5, pady=5)
        m.image_id = None
        
    def loadTexture(m, texture_path):
        if not os.path.exists(texture_path):
            print ("*** Error: Texture file not found: %s" %(texture_path))
            return False
        img = Image.open(texture_path)
        texture = img.convert('RGB')
        if texture.width > m.canvas_size or texture.height > m.canvas_size:
            left = (texture.width - m.canvas_size) // 2
            top = (texture.height - m.canvas_size) // 2
            right = left + m.canvas_size
            bottom = top + m.canvas_size
            texture = texture.crop((left, top, right, bottom))
        if texture.width < m.canvas_size or texture.height < m.canvas_size:
            texture = texture.resize((m.canvas_size, m.canvas_size), Image.Resampling.LANCZOS)
        m.texture_image = texture.convert('RGBA')
        print ("*** Loaded texture: %s" %(texture_path))
        return True
    
    def loadDecal(m, decal_path):
        if not os.path.exists(decal_path):
            print ("*** Error: Decal file not found: %s" %(decal_path))
            return False
        decal = Image.open(decal_path)
        if decal.mode != 'RGBA':
            decal = decal.convert('RGBA')
        decal_width, decal_height = decal.size
        center_x = m.canvas_size // 2
        center_y = m.canvas_size // 2
        paste_x = center_x - decal_width // 2
        paste_y = center_y - decal_height // 2
        composite = m.texture_image.copy()
        composite.paste(decal, (paste_x, paste_y), decal)
        photo = ImageTk.PhotoImage(composite.convert('RGB'))
        m.canvas.delete(m.image_id)
        m.image_id = m.canvas.create_image(0, 0, anchor=tk.NW, image=photo)
        m.canvas.image = photo
        print ("*** Loaded decal: %s" %(decal_path))
        return True
    
    def run(m):
        m.root.mainloop()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    texture_path = "/home/w/Projects/img-content/texture-grassland3/colors-grassland3_palette_texture_blurred.png"
    decal_path = "/home/w/Projects/img-content/texture-grassland3-pine.png"
    loader = ForestDecalLoader(canvas_size=600)
    loader.loadTexture(texture_path)
    loader.loadDecal(decal_path)
    loader.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
