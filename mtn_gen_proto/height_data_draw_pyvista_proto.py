#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import numpy as np
import pyvista as pv
from PIL import Image
import glob
import math
import random

#================================================================================================================================#
#=> - Class: HeightDataDraw
#================================================================================================================================#

class HeightDataDraw:

    def __apply_oval_boundary_alpha (m, img_array, radius_range_from_boundary=5):
        h, w = img_array.shape[:2]
        cx, cy = w / 2.0, h / 2.0
        outer_r = min(cx, cy)
        result = img_array.copy().astype(np.float32)
        for y in range(h):
            for x in range(w):
                dx = (x - cx) / outer_r
                dy = (y - cy) / outer_r
                dist_from_center = math.sqrt(dx * dx + dy * dy)
                if dist_from_center <= 1.0:
                    dist_from_boundary = (1.0 - dist_from_center) * outer_r
                    factor = min(1.0, dist_from_boundary / (radius_range_from_boundary + int(random.random() - 0.5) * 20))
                    if len(img_array.shape) == 2:
                        result[y, x] = result[y, x] * factor + (random.random() - 0.5) * 0.4
                    else:
                        result[y, x, :] = result[y, x, :] * factor
        return result.astype(np.uint8)

    def __slope_lower_fraction (m, img_array, fraction=0.5):
        h, w = img_array.shape[:2]
        lower_start = int(h * (1.0 - fraction))
        result = img_array.copy().astype(np.float32)
        range_height = h - lower_start
        for y in range(lower_start, h):
            factor = 1.0 - (y - lower_start) / (range_height - 1)
            for x in range(w):
                if img_array[y, x] != 0:
                    result[y, x] = result[y, x] * factor * factor * random.uniform(0.95, 1.05)
        return result.astype(np.uint8)

    def __load_file (m, idx):
        img_array = np.array(Image.open(m.file_list[idx]))
        m.current_img = img_array[:, :, 0]
        m.current_alpha = img_array[:, :, 3]
        m.current_img = m.__apply_oval_boundary_alpha(m.current_img, radius_range_from_boundary=20)

    def __create_mesh (m, height_data, alpha_data, height_scale=1.0):
        h, w = height_data.shape
        x = np.arange(w, dtype=np.float32) - w / 2.0
        y = np.arange(h, dtype=np.float32) - h / 2.0
        X, Y = np.meshgrid(x, y)
        Z = height_data.astype(np.float32) * height_scale * 1.2
        grid = pv.StructuredGrid(X, Y, Z)
        height_flat = height_data.flatten(order="F").astype(np.float32)
        grid["height"] = height_flat
        alpha_flat = alpha_data.flatten(order="F").astype(np.float32)
        grid["alpha"] = alpha_flat
        grid = grid.threshold(0.5, scalars="alpha")
        return grid

    def __init__ (m, height_scale=1.0, save_path=None, filename=None):
        m.height_scale = height_scale
        m.save_path = save_path
        m.file_list = []
        m.flip_options = [(False, False), (True, False), (False, True), (True, True)]
        m.current_idx = 0
        m.current_img, m.current_alpha = None, None
        if filename is not None:
            m.file_list.append(os.path.join(m.save_path, filename))
        else:
            for file in os.listdir(m.save_path):
                file_name = file.split("/")[-1]
                if file.endswith(".png") and "pv" not in file_name and file_name.startswith("raw_mtn_hd"):
                    m.file_list.append(os.path.join(m.save_path, file))
        for file in sorted(m.file_list):
            m.__load_file(m.current_idx)
            m.plt = pv.Plotter(window_size=(m.current_img.shape[1], m.current_img.shape[0]), off_screen=True)
            m.plt.disable_anti_aliasing()
            m.plt.enable_parallel_projection()
            m.mesh = None
            m.update_display()
            m.current_idx += 1

    def update_display (m):
        for idx, (v_flip, h_flip) in enumerate(m.flip_options):
            img = m.current_img.copy()
            if v_flip: img = img[:, ::-1]
            if h_flip: img = img[::-1, :]
            if m.mesh is not None:
                m.plt.remove_actor(m.mesh)
            img = m.__slope_lower_fraction(img, fraction=0.25)
            grid = m.__create_mesh(img, m.current_alpha, m.height_scale * (img.shape[1] // 100))
            m.mesh = m.plt.add_mesh(grid, scalars="height", cmap="gray", show_edges=False, opacity=1.0, show_scalar_bar=False)
            m.plt.set_background("darkgray")

            # Never touch these lines!
            m.plt.camera.azimuth = 45
            m.plt.camera.elevation = -4 #/ (math.ceil(m.current_img.shape[1] // 100))
            bounds = m.mesh.bounds
            m.plt.camera.parallel_scale = img.shape[1] // 2 
            m.plt.camera.focal_point = ((bounds[0] + bounds[1]) / 2, bounds[2] + m.plt.camera.parallel_scale, 0)
            current_focal_point = list(m.plt.camera.focal_point)
            current_focal_point[1] -= img.shape[1] // 2  
            m.plt.camera.focal_point = current_focal_point

            m.plt.off_screen = True
            m.plt.render()
            m.plt.screenshot(m.file_list[m.current_idx].replace(".png", "_pv%d.png" % idx), transparent_background=True)
            m.plt.off_screen = m.plt.off_screen

    def run (m):
        m.plt.show()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    save_path = "../../img-content"
    filename = "raw_mtn_hd006.png"
    filename = None
    draw = HeightDataDraw(height_scale=0.30, save_path=save_path, filename=filename)
    draw.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
