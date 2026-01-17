#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageTk

from dialog_get_ints import DialogGetInts
from dialog_new_cont import DialogNewCont
from dialog_coastal_mtn import DialogCoastalMtn
from dialog_climate import DialogClimate
from multi_canvas import CycleSelectCanvas

#================================================================================================================================#
#=> - MapEditor class -
#================================================================================================================================#

class MapEditor:

    COLOR_OCEAN = (32, 26, 120)
    COLOR_GRASSLAND = (121, 189, 36)
    COLOR_PLAINS = (222, 199, 89)
    COLOR_DESERT = (244, 203, 141)
    COLOR_TUNDRA = (248, 251, 252)
    COLOR_MOUNTAIN = (100, 50, 25)

    DEFAULT_COLOR_BG = COLOR_OCEAN
    DEFAULT_COLOR_FILL = COLOR_GRASSLAND

    #==> Helper <================================================================================================================#

    def __print (m, msg):
        print("*** " + msg)

    def __update_current_image (m, img):
        m.cycle_canvas.update_current_image(img)

    def __upsize_map_width (m, old_w, new_w):
        current_img = m.cycle_canvas.get_current_image()
        if current_img is None:
            return
        dw = new_w - old_w
        add_left = dw // 2
        add_right = dw - add_left
        new_img = Image.new("RGB", (new_w, m.map_h), m.DEFAULT_COLOR_BG)
        new_img.paste(current_img, (add_left, 0))
        m.__update_current_image(new_img)
        m.map_w = new_w
        m.margin_left += add_left
        m.margin_right += add_right

    def __upsize_map_height (m, old_h, new_h):
        current_img = m.cycle_canvas.get_current_image()
        if current_img is None:
            return
        dh = new_h - old_h
        add_top = dh // 2
        add_bottom = dh - add_top
        new_img = Image.new("RGB", (m.map_w, new_h), m.DEFAULT_COLOR_BG)
        new_img.paste(current_img, (0, add_top))
        m.__update_current_image(new_img)
        m.map_h = new_h
        m.margin_top += add_top
        m.margin_bottom += add_bottom

    def __downsize_map_width (m, old_w, new_w):
        current_img = m.cycle_canvas.get_current_image()
        if current_img is None:
            return
        dw = old_w - new_w
        total_margin = m.margin_left + m.margin_right
        remove_left_margin = int(round(dw * m.margin_left / total_margin))
        remove_right_margin = dw - remove_left_margin
        new_img = current_img.crop((remove_left_margin, 0, m.map_w - remove_right_margin, m.map_h))
        m.__update_current_image(new_img)
        m.map_w = new_w
        m.margin_left -= remove_left_margin
        m.margin_right -= remove_right_margin

    def __downsize_map_height (m, old_h, new_h):
        current_img = m.cycle_canvas.get_current_image()
        if current_img is None:
            return
        dh = old_h - new_h
        total_margin = m.margin_top + m.margin_bottom
        remove_top_margin = int(round(dh * m.margin_top / total_margin))
        remove_bottom_margin = dh - remove_top_margin
        new_img = current_img.crop((0, remove_top_margin, m.map_w, m.map_h - remove_bottom_margin))
        m.__update_current_image(new_img)
        m.map_h = new_h
        m.margin_top -= remove_top_margin
        m.margin_bottom -= remove_bottom_margin

    def __get_default_map (m):
        return Image.new("RGB", (m.map_w, m.map_h), m.DEFAULT_COLOR_BG)

    #==> Callbacks <=============================================================================================================#

    def __load_cb (m):
        if not os.path.exists(m.project_path):
            m.__print("Project path does not exist: %s" % m.project_path)
            return
        file_list = sorted([f for f in os.listdir(m.project_path) if f.startswith("cont") and f.endswith(".png")])
        if not file_list:
            m.__print("No continent images found in: %s" % m.project_path)
            return
        m.cycle_canvas.images = []
        m.cycle_canvas.image_paths = []
        m.cycle_canvas.checkboxes = []
        for widget in m.cycle_canvas.checkbox_frame.winfo_children():
            widget.destroy()
        for i,filename in enumerate(file_list):
            filepath = os.path.join(m.project_path, filename)
            img = Image.open(filepath)
            if i == 0:
                m.map_w, m.map_h = img.width, img.height
            m.cycle_canvas.add_image(img, filepath)
        if len(m.cycle_canvas.images) > 0:
            m.cycle_canvas.set_current_index(0)
            m.showing_default_img = False
            m.__print("Loaded %d images from: %s" % (len(file_list), m.project_path))

    def __save_cb (m):
        if not os.path.exists(m.project_path):
            os.makedirs(m.project_path)
        images = m.cycle_canvas.get_all_images()
        saved_files = []
        for i, img in enumerate(images, start=1):
            filename = "cont%03d.png" % i
            filepath = os.path.join(m.project_path, filename)
            saved_files.append(filepath)
            img.save(filepath)
            m.__print("Saved: %s" % filepath)
        m.__print("Saved %d images" % len(images))
        all_png_files = [os.path.join(m.project_path, f) for f in os.listdir(m.project_path) if f.endswith(".png")]
        for png_file in all_png_files:
            if png_file not in saved_files:
                os.remove(png_file)
                m.__print("Removed: %s" % png_file)
        
    def __new_cont_cb (m):
        dialog = DialogNewCont(m.root, m.map_w, m.map_h, m.zoom, m.DEFAULT_COLOR_BG, m.DEFAULT_COLOR_FILL, scale=0.5)
        new_img = dialog.get_image()
        if new_img is not None:
            new_w, new_h = new_img.size
            if new_w <= m.map_w and new_h <= m.map_h:
                base_img = Image.new("RGB", (m.map_w, m.map_h), m.DEFAULT_COLOR_BG)
                base_img.paste(new_img, ((m.map_w - new_w) // 2, (m.map_h - new_h) // 2))
                if m.showing_default_img:
                    m.cycle_canvas.update_current_image(base_img)
                else:
                    m.cycle_canvas.add_image(base_img, None)
                m.showing_default_img = False
                m.__print("New continent added: %d x %d" % (new_w, new_h))

    def __resize_cb (m):
        min_w, max_w = m.cnv_w - (m.margin_left + m.margin_right), m.cnv_w
        min_h, max_h = m.cnv_h - (m.margin_top + m.margin_bottom), m.cnv_h
        old_w, old_h = m.map_w, m.map_h
        values = DialogGetInts(m.root, [(min_w, m.map_w, max_w), (min_h, m.map_h, max_h)]).get_values()
        new_w, new_h = values[0], values[1]
        if new_w > m.map_w:
            m.__upsize_map_width(m.map_w, new_w)
        elif new_w < m.map_w:
            m.__downsize_map_width(m.map_w, new_w)
        if new_h > m.map_h:
            m.__upsize_map_height(m.map_h, new_h)
        elif new_h < m.map_h:
            m.__downsize_map_height(m.map_h, new_h)
        m.__print("Map resized to: %d x %d" % (m.map_w, m.map_h))

    def __coastal_mtn_cb (m):
        current_img = m.cycle_canvas.get_current_image()
        if current_img is None:
            return
        dialog = DialogCoastalMtn(m.root, current_img, m.DEFAULT_COLOR_BG, m.DEFAULT_COLOR_FILL, m.COLOR_MOUNTAIN)
        new_img = dialog.get_image()
        if new_img is not None:
            m.__update_current_image(new_img)
            m.__print("Coastal mountain added")

    def __climate_cb (m):
        current_img = m.cycle_canvas.get_current_image()
        if current_img is None:
            return
        climate_colors = [m.COLOR_GRASSLAND, m.COLOR_PLAINS, m.COLOR_DESERT]
        ignore_colors = [m.COLOR_OCEAN, m.COLOR_MOUNTAIN]
        dialog = DialogClimate(m.root, current_img, m.COLOR_OCEAN, m.COLOR_MOUNTAIN, climate_colors, ignore_colors)
        new_img = dialog.get_image()
        if new_img is not None:
            m.__update_current_image(new_img)
            m.__print("Climate applied")

    def __setup_ui (m):
        m.frm_btn = tk.Frame(m.root)
        m.frm_btn.pack(pady=5, anchor='w')
        
        m.btn_load = tk.Button(m.frm_btn, text="Load", width=m.btn_w, height=m.btn_h, command=m.__load_cb)
        m.btn_load.grid(row=0, column=0, padx=5, pady=5)
        m.btn_save = tk.Button(m.frm_btn, text="Save", width=m.btn_w, height=m.btn_h, command=m.__save_cb)
        m.btn_save.grid(row=0, column=1, padx=5, pady=5)
        
        m.btn_resize = tk.Button(m.frm_btn, text="Resize", width=m.btn_w, height=m.btn_h, command=m.__resize_cb)
        m.btn_resize.grid(row=1, column=0, padx=5, pady=5)
        m.btn_new_cont = tk.Button(m.frm_btn, text="New cont.", width=m.btn_w, height=m.btn_h, command=m.__new_cont_cb)
        m.btn_new_cont.grid(row=1, column=1, padx=5, pady=5)
        m.btn_coastal_mtn = tk.Button(m.frm_btn, text="Coastal mtn.", width=m.btn_w, height=m.btn_h, command=m.__coastal_mtn_cb)
        m.btn_coastal_mtn.grid(row=1, column=2, padx=5, pady=5)
        m.btn_climate = tk.Button(m.frm_btn, text="Climate", width=m.btn_w, height=m.btn_h, command=m.__climate_cb)
        m.btn_climate.grid(row=1, column=3, padx=5, pady=5)
        
        m.cycle_canvas = CycleSelectCanvas(m.root, m.cnv_w, m.cnv_h, m.DEFAULT_COLOR_BG)
        m.cycle_canvas.get_widget().pack(pady=5)

        default_img = m.__get_default_map()
        m.cycle_canvas.add_image(default_img, None)

    #==> Initialization <=========================================================================================================#

    def __init__ (m, project_path):
        m.project_path = project_path
        m.showing_default_img = True
        m.root = tk.Tk()
        m.root.title("Map Editor")
        m.btn_w, m.btn_h = 10, 1
        m.cnv_w, m.cnv_h = 1600, 800
        m.map_w, m.map_h = 200, 200
        m.zoom = 1
        m.margin_left, m.margin_right = m.cnv_w//2, m.cnv_w//2
        m.margin_top, m.margin_bottom = m.cnv_h//2, m.cnv_h//2
        m.__setup_ui()

    #==> Public Methods <=========================================================================================================#

    def load_image_on_canvas (m, filepath):
        img = Image.open(filepath)
        m.cycle_canvas.add_image(img, filepath)

    def run (m):
        m.root.mainloop()

#================================================================================================================================#
#=> - Main entry point -
#================================================================================================================================#

if __name__ == "__main__":
    project_path = "../../rts-proto-map/first-test/"
    editor = MapEditor(project_path)
    editor.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
