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

#================================================================================================================================#
#=> - Class: HeightDataDraw
#================================================================================================================================#

class HeightDataDraw:

    def __load_file (m, idx):
        img = Image.open(m.file_list[idx])
        img_array = np.array(img)
        if img_array.shape[2] == 4:
            m.current_img = img_array[:, :, 0]
            m.current_alpha = img_array[:, :, 3]
        else:
            m.current_img = img_array
            m.current_alpha = np.ones_like(img_array, dtype=np.uint8) * 255

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

    def __init__ (m, height_scale=1.0, save_path=None):
        m.height_scale = height_scale
        m.save_path = save_path
        m.file_list = []
        m.flip_options = [(False, False), (True, False), (False, True), (True, True)]
        m.current_idx = 0
        m.current_img, m.current_alpha = None, None
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
            if v_flip:
                m.current_img = m.current_img[:, ::-1]
            if h_flip:
                m.current_img = m.current_img[::-1, :]
            if m.mesh is not None:
                m.plt.remove_actor(m.mesh)
            grid = m.__create_mesh(m.current_img, m.current_alpha, m.height_scale)
            m.mesh = m.plt.add_mesh(grid, scalars="height", cmap="gray", show_edges=False, opacity=1.0, show_scalar_bar=False)
            m.plt.set_background("darkgray")

            # Never touch these lines!
            m.plt.camera.azimuth = 45
            m.plt.camera.elevation = -5
            bounds = m.mesh.bounds
            m.plt.camera.parallel_scale = 50 
            m.plt.camera.focal_point = ((bounds[0] + bounds[1]) / 2, bounds[2] + m.plt.camera.parallel_scale, 0)
            current_focal_point = list(m.plt.camera.focal_point)
            current_focal_point[1] -= 50 
            m.plt.camera.focal_point = current_focal_point

            output_path = m.file_list[m.current_idx].replace(".png", "_pv%d.png" % idx)
            was_off_screen = m.plt.off_screen
            m.plt.off_screen = True
            m.plt.render()
            m.plt.screenshot(output_path, transparent_background=True)
            m.plt.off_screen = was_off_screen

    def run (m):
        m.plt.show()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

if __name__ == "__main__":
    save_path = "../../img-content"
    draw = HeightDataDraw(height_scale=0.15, save_path=save_path)
    draw.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
