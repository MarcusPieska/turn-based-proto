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
from scipy import ndimage

SMOOTH_INTENSITY = 1.0
HIGH_OFFSET = -20
HEIGHT_THRESHOLD = 100
HEIGHT_THRESHOLD_LOW = 50

COLOR_BELOW_LOW = [0.47, 0.74, 0.14]
COLOR_BELOW = [0.7, 0.3, 0.2]
COLOR_ABOVE = [1.0, 1.0, 1.0]

COLOR_BELOW = [ x * 1.2 for x in COLOR_BELOW]

LIGHT_DIRECTION = [1.0, 0.5, 2.0]
LIGHT_DISTANCE_MULT = 20.0
LIGHT_INTENSITY = 1.2
LIGHT_COLOR = 'white'

ENABLE_RADIAL_COLOR_FACTOR = True
RADIAL_R1 = 1.0
RADIAL_R2 = 0.8
RADIAL_R3 = 0.6
RADIAL_R4 = 0.0
RADIAL_FACTOR_R1 = 0.0
RADIAL_FACTOR_R2 = 0.3
RADIAL_FACTOR_R3 = 0.8
RADIAL_FACTOR_R4 = 1.0

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

    def __smooth_height_data (m, height_data, alpha_data):
        global SMOOTH_INTENSITY
        sigma = SMOOTH_INTENSITY
        smoothed = ndimage.gaussian_filter(height_data.astype(np.float32), sigma=sigma)
        mask = alpha_data > 0
        smoothed = np.where(mask, smoothed, height_data)
        return smoothed.astype(np.uint8)

    def __apply_radial_color_factor (m, img, alpha_data):
        global ENABLE_RADIAL_COLOR_FACTOR, RADIAL_R1, RADIAL_R2, RADIAL_R3, RADIAL_R4
        global RADIAL_FACTOR_R1, RADIAL_FACTOR_R2, RADIAL_FACTOR_R3, RADIAL_FACTOR_R4
        if not ENABLE_RADIAL_COLOR_FACTOR:
            return img
        
        h, w = img.shape
        cx, cy = w / 2.0, h / 2.0
        outer_r = min(cx, cy)
        
        result = img.copy().astype(np.float32)
        
        for y in range(h):
            for x in range(w):
                if alpha_data[y, x] > 0:
                    dx = (x - cx) / outer_r
                    dy = (y - cy) / outer_r
                    r = math.sqrt(dx * dx + dy * dy)
                    
                    if r >= RADIAL_R2:
                        t = (r - RADIAL_R1) / (RADIAL_R2 - RADIAL_R1) if (RADIAL_R2 - RADIAL_R1) != 0 else 0
                        factor = RADIAL_FACTOR_R1 + (RADIAL_FACTOR_R2 - RADIAL_FACTOR_R1) * t
                    elif r >= RADIAL_R3:
                        t = (r - RADIAL_R2) / (RADIAL_R3 - RADIAL_R2) if (RADIAL_R3 - RADIAL_R2) != 0 else 0
                        factor = RADIAL_FACTOR_R2 + (RADIAL_FACTOR_R3 - RADIAL_FACTOR_R2) * t
                    else:
                        t = (r - RADIAL_R3) / (RADIAL_R4 - RADIAL_R3) if (RADIAL_R4 - RADIAL_R3) != 0 else 0
                        factor = RADIAL_FACTOR_R3 + (RADIAL_FACTOR_R4 - RADIAL_FACTOR_R3) * t
                    
                    factor = max(0.0, min(1.0, factor))
                    result[y, x] = result[y, x] * factor
        
        return result.astype(np.uint8)

    def __load_file (m, idx):
        img_array = np.array(Image.open(m.file_list[idx]))
        m.current_img = img_array[:, :, 0]
        m.current_alpha = img_array[:, :, 3]
        m.current_img = m.__apply_oval_boundary_alpha(m.current_img, radius_range_from_boundary=20)

    def __create_mesh (m, height_data, alpha_data, height_scale=1.0):
        global HEIGHT_THRESHOLD, HEIGHT_THRESHOLD_LOW, COLOR_BELOW, COLOR_BELOW_LOW, COLOR_ABOVE
        h, w = height_data.shape
        x = np.arange(w, dtype=np.float32) - w / 2.0
        y = np.arange(h, dtype=np.float32) - h / 2.0
        X, Y = np.meshgrid(x, y)
        Z = height_data.astype(np.float32) * height_scale * 0.45
        grid = pv.StructuredGrid(X, Y, Z)
        height_flat = height_data.flatten(order="F").astype(np.float32)
        grid["height"] = height_flat
        alpha_flat = alpha_data.flatten(order="F").astype(np.float32)
        grid["alpha"] = alpha_flat
        grid = grid.threshold(0.5, scalars="alpha")
        
        height_values = grid["height"]
        colors = np.zeros((len(height_values), 3))
        below_low_mask = height_values < HEIGHT_THRESHOLD_LOW
        below_mask = (height_values >= HEIGHT_THRESHOLD_LOW) & (height_values < HEIGHT_THRESHOLD)
        above_mask = height_values >= HEIGHT_THRESHOLD
        colors[below_low_mask] = COLOR_BELOW_LOW
        colors[below_mask] = COLOR_BELOW
        colors[above_mask] = COLOR_ABOVE
        grid["colors"] = colors
        
        return grid

    def __init__ (m, height_scale=1.0, save_path=None, filename=None, smooth=False):
        m.height_scale = height_scale
        m.save_path = save_path
        m.smooth = smooth
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
            alpha = m.current_alpha.copy()
            if v_flip: 
                img = img[:, ::-1]
                alpha = alpha[:, ::-1]
            if h_flip: 
                img = img[::-1, :]
                alpha = alpha[::-1, :]
            if m.mesh is not None:
                m.plt.remove_actor(m.mesh)
            #img = m.__slope_lower_fraction(img, fraction=0.25)
            if m.smooth:
                img = m.__smooth_height_data(img, alpha)
            img = m.__apply_radial_color_factor(img, alpha)
            
            global ENABLE_RADIAL_COLOR_FACTOR
            if ENABLE_RADIAL_COLOR_FACTOR and 0:
                debug_img = np.zeros((img.shape[0], img.shape[1], 4), dtype=np.uint8)
                debug_img[:, :, 0] = img
                debug_img[:, :, 1] = img
                debug_img[:, :, 2] = img
                debug_img[:, :, 3] = alpha
                debug_pil = Image.fromarray(debug_img, mode='RGBA')
                debug_path = m.file_list[m.current_idx].replace(".png", "_radial_debug_%d.png" % idx)
                debug_pil.save(debug_path)
            
            grid = m.__create_mesh(img, alpha, m.height_scale * (img.shape[1] // 100))
            m.mesh = m.plt.add_mesh(grid, scalars="colors", rgb=True, show_edges=False, opacity=1.0, show_scalar_bar=False)
            m.plt.set_background("darkgray")
            
            bounds = m.mesh.bounds
            center_x = (bounds[0] + bounds[1]) / 2
            center_y = (bounds[2] + bounds[3]) / 2
            center_z = (bounds[4] + bounds[5]) / 2
            light_distance = img.shape[1] * LIGHT_DISTANCE_MULT
            light_pos_x = center_x + LIGHT_DIRECTION[0] * light_distance
            light_pos_y = center_y + LIGHT_DIRECTION[1] * light_distance
            light_pos_z = center_z + LIGHT_DIRECTION[2] * light_distance
            light = pv.Light(
                position=(light_pos_x, light_pos_y, light_pos_z),
                focal_point=(center_x, center_y, center_z),
                color=LIGHT_COLOR,
                intensity=LIGHT_INTENSITY,
                light_type='scenelight'
            )
            renderer = m.plt.renderer
            renderer.RemoveAllLights()
            m.plt.add_light(light)

            # Never touch these lines!
            m.plt.camera.azimuth = 45
            m.plt.camera.elevation = -4 #/ (math.ceil(m.current_img.shape[1] // 100))
            m.plt.camera.parallel_scale = img.shape[1] // 2 
            m.plt.camera.focal_point = ((bounds[0] + bounds[1]) / 2, bounds[2] + m.plt.camera.parallel_scale, 0)
            current_focal_point = list(m.plt.camera.focal_point)
            current_focal_point[1] -= img.shape[1] // 2  
            current_focal_point[1] += HIGH_OFFSET
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
    smooth = "--smooth" in sys.argv
    save_path = "../../img-content"
    filename = "raw_mtn_hd000_cr.png"
    #filename = None
    draw = HeightDataDraw(height_scale=0.30, save_path=save_path, filename=filename, smooth=smooth)
    if filename is not None:
        draw.run()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
