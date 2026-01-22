#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import math
import tkinter as tk
from PIL import Image, ImageTk
import noise

#================================================================================================================================#
#=> - Function: generate_noise_base
#================================================================================================================================#

def generate_noise_base (noise_image_path="noise_base.png"):
    seed = np.random.randint(0, 1000000)
    noise_w, noise_h = 1600, 800
    noise_data = np.zeros((noise_h, noise_w), dtype=np.float64)
    for y in range(noise_h):
        for x in range(noise_w):
            value, amplitude = 0.0, 1.0
            freq = 1.0 / 50.0
            for i in range(6):
                nx, ny = x * freq, y * freq
                nval = noise.pnoise2(nx, ny, repeatx=1024, repeaty=1024, base=seed + i)
                value += nval * amplitude
                amplitude *= 0.5
                freq *= 2.0
            noise_data[y, x] = value
    min_val = noise_data.min()
    max_val = noise_data.max()
    if max_val > min_val:
        normalized = (noise_data - min_val) / (max_val - min_val)
        noise_u8 = (normalized * 255.0).astype(np.uint8)
    else:
        noise_u8 = np.zeros_like(noise_data, dtype=np.uint8)
    img = Image.fromarray(noise_u8, mode='L')
    img.save(noise_image_path)

#================================================================================================================================#
#=> - Class: HeightDataGenerator
#================================================================================================================================#

class HeightDataGenerator:

    def __init__ (m, tile_width=100, tile_height=100, noise_image_path="noise_base.png"):
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.noise_image_path = noise_image_path
        m.noise_base = None
        if os.path.exists(m.noise_image_path):
            m.noise_base = np.array(Image.open(m.noise_image_path))
        m.alpha = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        cx, cy = m.tile_width / 2.0, m.tile_height / 2.0
        hw, hh = m.tile_width / 2.0, m.tile_height / 2.0
        m.outer_r = min(hw, hh)
        m.inner_r = m.outer_r * 0.5
        for y in range(m.tile_height):
            for x in range(m.tile_width):
                dx = abs(x - cx) / hw
                dy = abs(y - cy) / hh
                dist_to_boundary = 1.0 - (dx + dy)
                if dist_to_boundary > 0:
                    m.alpha[y, x] = 255

    def sample_from_noise (m, click_x, click_y, radius):
        if m.noise_base is None:
            return m.base_height_data.copy(), m.alpha
        h, w = m.noise_base.shape
        sampling_r = m.inner_r + (radius / 100.0) * (m.outer_r - m.inner_r)
        cx, cy = m.tile_width / 2.0, m.tile_height / 2.0
        result = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        circular_alpha = np.zeros((m.tile_height, m.tile_width), dtype=np.uint8)
        for y in range(m.tile_height):
            for x in range(m.tile_width):
                dx = (x - cx) / m.outer_r
                dy = (y - cy) / m.outer_r
                dist = math.sqrt(dx * dx + dy * dy)
                if dist <= 1.0:
                    circular_alpha[y, x] = 255
                    src_x = int(click_x + dx * sampling_r)
                    src_y = int(click_y + dy * sampling_r)
                    if 0 <= src_x < w and 0 <= src_y < h:
                        result[y, x] = m.noise_base[src_y, src_x]
        return result, circular_alpha

#================================================================================================================================#
#=> - Class: HeightDataDisplay
#================================================================================================================================#

class HeightDataDisplay:

    def __forget_all_widgets (m):
        for widget in m.widgets:
            widget.pack_forget()

    #==> Step 1: Noise base generation <=========================================================================================#

    def __show_noise_gen_page (m):
        m.__forget_all_widgets()
        m.cnv_gen.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.btn_gen.pack(side=tk.LEFT, padx=5, pady=5)
        m.btn_step2.pack(side=tk.LEFT, padx=5, pady=5)

    def __generate_noise_base (m):
        generate_noise_base(m.gen.noise_image_path)
        m.gen.noise_base = np.array(Image.open(m.gen.noise_image_path))
        h, w = m.gen.noise_base.shape
        img = Image.fromarray(m.gen.noise_base, mode='L')
        canvas_w = m.cnv_gen.winfo_width()
        canvas_h = m.cnv_gen.winfo_height()
        if canvas_w <= 1 or canvas_h <= 1:
            canvas_w, canvas_h = 1600, 800
        scale = min(canvas_h / h, canvas_w / w)
        new_w = int(w * scale)
        new_h = int(h * scale)
        img_resized = img.resize((new_w, new_h), Image.NEAREST)
        m.noise_gen_photo = ImageTk.PhotoImage(img_resized)
        m.cnv_gen.delete("all")
        m.cnv_gen.create_image(0, 0, image=m.noise_gen_photo, anchor=tk.NW)
        m.btn_step2.config(state=tk.NORMAL)

    def __save_noise_base (m):
        if m.gen.noise_base is None:
            m.__generate_noise_base()
        img = Image.fromarray(m.gen.noise_base, mode='L')
        img.save(m.gen.noise_image_path)
        m.curr_page = "selection"
        m.__show_selection_page()

    #==> Step 2: Noise selection <===============================================================================================#

    def __update_noise_display (m):
        h, w = m.gen.noise_base.shape
        img = Image.fromarray(m.gen.noise_base, mode='L')
        canvas_w, canvas_h = m.cnv_sel.winfo_width(), m.cnv_sel.winfo_height()
        if canvas_w <= 1 or canvas_h <= 1:
            canvas_w, canvas_h = 1600, 800
        scale = min(canvas_h / h, canvas_w / w) * 1.0
        new_w, new_h = int(w * scale), int(h * scale)
        m.noise_photo = ImageTk.PhotoImage(img.resize((new_w, new_h), Image.NEAREST))
        m.cnv_sel.delete("all")
        m.cnv_sel.create_image(0, 0, image=m.noise_photo, anchor=tk.NW)
        x, y = m.click_x, m.click_y
        xr, yr = m.tile_width // 2, m.tile_height // 2
        m.cnv_sel.create_oval(x - xr, y - yr, x + xr, y + yr, outline="red", width=2)

    def __show_selection_page (m):
        m.__forget_all_widgets()
        m.btn_step3.pack(side=tk.LEFT, padx=5, pady=5)
        m.btn_step3.config(state=tk.DISABLED)
        m.cnv_sel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.__update_noise_display()

    def __select_noise (m, event):
        m.click_x, m.click_y = int(event.x), int(event.y)
        m.btn_step3.config(state=tk.NORMAL)
        m.__update_noise_display()

    def __move_on_to_crop_selection_page (m):
        m.curr_page = "crop_selection"
        m.__show_display_page()

    #==> Step 3: Crop selection <================================================================================================#

    def __show_display_page (m):
        m.__forget_all_widgets()
        m.cnv_tile.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        m.slider.pack(side=tk.LEFT, padx=5, pady=5)
        m.btn_back.pack(side=tk.LEFT, padx=5, pady=5)
        m.btn_save_tile.config(state=tk.DISABLED)
        m.btn_save_tile.pack(side=tk.LEFT, padx=5, pady=5)
        m.__insert_cropped_noise()

    def __on_r_change (m, value):
        m.__insert_cropped_noise()

    def __on_delect_new_noise (m):
        m.curr_page = "selection"
        m.__show_selection_page()

    def __insert_cropped_noise (m):
        radius = m.r_slider_val.get()
        height_data, alpha_data = m.gen.sample_from_noise(m.click_x, m.click_y, radius)
        m.height_data = height_data
        m.alpha_data = alpha_data
        h, w = height_data.shape
        rgba_array = np.zeros((h, w, 4), dtype=np.uint8)
        rgba_array[:, :, 0] = height_data
        rgba_array[:, :, 1] = height_data
        rgba_array[:, :, 2] = height_data
        rgba_array[:, :, 3] = alpha_data
        img = Image.fromarray(rgba_array, mode='RGBA')
        scale = 4
        new_w, new_h = w * scale, h * scale
        img_resized = img.resize((new_w, new_h), Image.NEAREST)
        m.tile_photo = ImageTk.PhotoImage(img_resized)
        m.cnv_tile.delete("all")
        cx, cy = new_w // 2, new_h // 2
        outer_r, inner_r = m.gen.outer_r * scale, m.gen.inner_r * scale
        curr_r = m.gen.inner_r + (radius / 100.0) * (m.gen.outer_r - m.gen.inner_r)
        curr_r_px = curr_r * scale
        m.cnv_tile.create_image(cx, cy, image=m.tile_photo, anchor=tk.CENTER)
        m.cnv_tile.create_oval(cx - outer_r, cy - outer_r, cx + outer_r, cy + outer_r, outline="blue", width=2, tags="ovals")
        m.cnv_tile.create_oval(cx - inner_r, cy - inner_r, cx + inner_r, cy + inner_r, outline="green", width=2, tags="ovals")
        m.cnv_tile.create_oval(cx - curr_r_px, cy - curr_r_px, cx + curr_r_px, cy + curr_r_px, outline="yellow", width=2, tags="ovals")
        m.cnv_tile.tag_raise("ovals")
        m.btn_save_tile.config(state=tk.NORMAL)

    def __save_tile_height_data (m):
        if m.height_data is None:
            return
        h, w = m.height_data.shape
        rgba_array = np.zeros((h, w, 4), dtype=np.uint8)
        rgba_array[:, :, 0] = m.height_data
        rgba_array[:, :, 1] = m.height_data
        rgba_array[:, :, 2] = m.height_data
        rgba_array[:, :, 3] = m.alpha_data
        img = Image.fromarray(rgba_array, mode='RGBA')
        suffix = 0
        while True:
            output_path = os.path.join(m.save_path, "raw_mtn_hd%03d.png" % suffix)
            if not os.path.exists(output_path):
                break
            suffix += 1
        img.save(output_path)
        m.curr_page = "selection"
        m.__show_selection_page()

    #============================================================================================================================#

    def __init__ (m, width=800, height=600, tile_width=100, tile_height=100, save_path=None):
        m.width = width
        m.height = height
        m.tile_width = tile_width
        m.tile_height = tile_height
        m.save_path = save_path if save_path else "."
        noise_image_path = os.path.join(m.save_path, "noise_base.png")
        m.root = tk.Tk()
        m.root.title("Height Data Display")
        m.gen = HeightDataGenerator(m.tile_width, m.tile_height, noise_image_path)
        m.height_data = None
        m.alpha_data = m.gen.alpha
        m.r_slider_val = tk.IntVar(value=50)
        m.click_x, m.click_y = 800, 400
        m.noise_photo = None
        m.noise_gen_photo = None
        m.tile_photo = None
        m.r_horisontal, m.r_vertical = tile_width // 2, tile_height // 2
        m.curr_page = None
        
        m.btn_frame = tk.Frame(m.root)
        m.btn_frame.pack(side=tk.TOP, fill=tk.X)
        m.widgets = []

        #==> Noise generation step <=============================================================================================#
        m.btn_gen = tk.Button(m.btn_frame, text="Generate", command=m.__generate_noise_base)
        m.btn_gen.pack(side=tk.LEFT, padx=5, pady=5)
        m.btn_step2 = tk.Button(m.btn_frame, text="Save", command=m.__save_noise_base, state=tk.DISABLED)
        m.btn_step2.pack(side=tk.LEFT, padx=5, pady=5)
        m.cnv_gen = tk.Canvas(m.root, width=1600, height=800, bg="darkgray")
        m.cnv_gen.bind("<Button-1>", lambda e: m.__generate_noise_base())
        m.widgets.extend([m.btn_gen, m.btn_step2, m.cnv_gen])
        
        #==> Noise selection step <=============================================================================================#
        m.btn_step3 = tk.Button(m.btn_frame, text="Next", command=m.__move_on_to_crop_selection_page, state=tk.DISABLED)
        m.btn_step3.pack(side=tk.LEFT, padx=5, pady=5)
        m.cnv_sel = tk.Canvas(m.root, width=1600, height=800, bg="darkgray")
        m.cnv_sel.bind("<Button-1>", m.__select_noise)
        m.widgets.extend([m.btn_step3, m.cnv_sel])
       
        #==> Crop selection step <=============================================================================================#
        m.btn_back = tk.Button(m.btn_frame, text="Back", command=m.__on_delect_new_noise)
        m.btn_back.pack(side=tk.LEFT, padx=5, pady=5)
        m.btn_save_tile = tk.Button(m.btn_frame, text="Save", command=m.__save_tile_height_data, state=tk.DISABLED)
        m.btn_save_tile.pack(side=tk.LEFT, padx=5, pady=5)
        m.slider = tk.Scale(m.btn_frame, from_=0, to=100, orient=tk.HORIZONTAL, variable=m.r_slider_val)
        m.slider.configure(command=m.__on_r_change)
        m.cnv_tile = tk.Canvas(m.root, width=m.tile_width * 4, height=m.tile_height * 4, bg="darkgray")
        m.widgets.extend([m.btn_back, m.btn_save_tile, m.slider, m.cnv_tile])

        if not os.path.exists(noise_image_path):
            m.curr_page = "noise_gen"
            m.__show_noise_gen_page()
        else:
            m.curr_page = "selection"
            m.__show_selection_page()

    def run (m):
        m.root.mainloop()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    save_path = "../../img-content"
    display = HeightDataDisplay(width=1600, height=800, tile_width=100, tile_height=100, save_path=save_path)
    display.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
