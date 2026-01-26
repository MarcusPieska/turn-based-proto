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
import numpy as np
import random
import bisect

#================================================================================================================================#
#=> - Class: Sector -
#================================================================================================================================#

class Sector:

    def __init__ (m, sector_id):
        m.sector_id = sector_id
        m.central_point = None
        m.coordinates = []
        m.color = None
        m.adjacent_sectors = []
        m.is_coastal = False
        m.river_system_id = -1
        m.connected_sectors = []

    def get_sector_id (m): 
        return m.sector_id

    def set_sector_id (m, sector_id):
         m.sector_id = sector_id

    def get_central_point (m): 
        return m.central_point

    def set_central_point (m, central_point): 
        m.central_point = central_point

    def get_coordinates (m): 
        return m.coordinates

    def set_coordinates (m, coordinates): 
        m.coordinates = coordinates

    def add_coordinate (m, coord): 
        m.coordinates.append(coord)

    def get_color (m): 
        return m.color

    def set_color (m, color): 
        m.color = color

    def get_adjacent_sectors (m): 
        return m.adjacent_sectors

    def set_adjacent_sectors (m, adjacent_sectors): 
        m.adjacent_sectors = adjacent_sectors

    def add_adjacent_sector (m, sector_id): 
        if sector_id not in m.adjacent_sectors: 
            m.adjacent_sectors.append(sector_id)

    def get_is_coastal (m): 
        return m.is_coastal

    def set_is_coastal (m, is_coastal): 
        m.is_coastal = is_coastal

    def get_river_system_id (m): 
        return m.river_system_id

    def set_river_system_id (m, river_system_id): 
        m.river_system_id = river_system_id

    def get_connected_sectors (m): 
        return m.connected_sectors

    def set_connected_sectors (m, connected_sectors): 
        m.connected_sectors = connected_sectors

    def add_connected_sector (m, sector_id):
        if sector_id not in m.connected_sectors: 
            m.connected_sectors.append(sector_id)

#================================================================================================================================#
#=> - Class: DialogGetSectors -
#================================================================================================================================#

class DialogGetSectors:

    def __update_button_states (m):
        m.btn_back.config(state=tk.NORMAL if m.current_stage > 0 else tk.DISABLED)
        m.btn_points.config(state=tk.NORMAL if m.current_stage == 0 else tk.DISABLED)
        m.btn_sectors.config(state=tk.NORMAL if m.current_stage == 1 else tk.DISABLED)
        m.btn_rivers.config(state=tk.NORMAL if m.current_stage == 2 else tk.DISABLED)
        m.btn_toggle.config(state=tk.NORMAL if m.current_stage == 3 else tk.DISABLED)

    def __on_back (m):
        if m.current_stage > 0: m.current_stage -= 1; m.__update_button_states()

    def __display_image (m, img):
        if img is None: return
        photo = ImageTk.PhotoImage(img)
        m.canvas.delete("all")
        m.canvas.create_image(img.size[0] // 2, img.size[1] // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo

    def __on_points (m):
        if m.img_start is None or m.image is None: return
        m.sectors = []
        img_orig_array = np.array(m.image)
        img_zoomed_array = np.array(m.img_start)
        orig_h, orig_w = img_orig_array.shape[:2]
        zoomed_h, zoomed_w = img_zoomed_array.shape[:2]
        zoom = m.canvas_zoom
        color_ocean_arr = np.array(m.color_ocean)
        color_mtn_arr = np.array(m.color_mtn)
        lattice_spacing = 20
        sector_id = 0
        for lattice_y in range(0, zoomed_h, lattice_spacing):
            for lattice_x in range(0, zoomed_w, lattice_spacing):
                offset_x, offset_y = random.randint(-5, 5), random.randint(-5, 5)
                x_zoomed, y_zoomed = lattice_x + offset_x, lattice_y + offset_y
                if x_zoomed < 0 or x_zoomed >= zoomed_w or y_zoomed < 0 or y_zoomed >= zoomed_h: continue
                x_orig, y_orig = x_zoomed // zoom, y_zoomed // zoom
                if x_orig >= orig_w or y_orig >= orig_h: continue
                pixel_color = img_orig_array[y_orig, x_orig]
                if img_orig_array.ndim == 3:
                    is_ocean = np.array_equal(pixel_color, color_ocean_arr)
                    is_mtn = np.array_equal(pixel_color, color_mtn_arr)
                else:
                    is_ocean = pixel_color == color_ocean_arr[0]
                    is_mtn = pixel_color == color_mtn_arr[0]
                if is_ocean or is_mtn: continue
                sector = Sector(sector_id); sector.set_central_point((x_zoomed, y_zoomed))
                m.sectors.append(sector); sector_id += 1
        m.img_points = m.img_start.copy()
        draw = ImageDraw.Draw(m.img_points)
        for sector in m.sectors:
            x, y = sector.get_central_point()
            draw.ellipse([x - 3, y - 3, x + 3, y + 3], fill=(255, 0, 0), outline=(255, 0, 0))
        m.__display_image(m.img_points)
        m.current_stage = 1
        m.__update_button_states()

    def __note_adjacency (m, sector_id_1, sector_id_2):
        if sector_id_1 != sector_id_2:
            m.sectors[sector_id_1].add_adjacent_sector(sector_id_2); m.sectors[sector_id_2].add_adjacent_sector(sector_id_1)
            m.sectors[sector_id_1].add_connected_sector(sector_id_2); m.sectors[sector_id_2].add_connected_sector(sector_id_1)

    def __on_sectors (m):
        if m.img_points is None or len(m.sectors) == 0: return
        img_array = np.array(m.img_points)
        img_h, img_w = img_array.shape[:2]
        color_ocean_arr = np.array(m.color_ocean)
        color_mtn_arr = np.array(m.color_mtn)
        is_3d = img_array.ndim == 3
        if is_3d:
            is_ocean_mask = np.all(img_array == color_ocean_arr, axis=2)
            is_mtn_mask = np.all(img_array == color_mtn_arr, axis=2)
        else:
            is_ocean_mask = (img_array == color_ocean_arr[0])
            is_mtn_mask = (img_array == color_mtn_arr[0])
        valid_mask = ~(is_ocean_mask | is_mtn_mask)
        claimed = np.zeros((img_h, img_w), dtype=bool)
        pixel_owner = np.full((img_h, img_w), -1, dtype=np.int32)
        queues = [[] for _ in range(len(m.sectors))]
        queue_sets = [set() for _ in range(len(m.sectors))]
        for i, sector in enumerate(m.sectors):
            px, py = sector.get_central_point()
            if 0 <= px < img_w and 0 <= py < img_h and valid_mask[py, px]:
                claimed[py, px] = True
                pixel_owner[py, px] = i
                sector.add_coordinate((px, py))
                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    nx, ny = px + dx, py + dy
                    if 0 <= nx < img_w and 0 <= ny < img_h and not claimed[ny, nx] and valid_mask[ny, nx]:
                        dist_sq = (nx - px) ** 2 + (ny - py) ** 2
                        if (nx, ny) not in queue_sets[i]:
                            bisect.insort(queues[i], (dist_sq, nx, ny))
                            queue_sets[i].add((nx, ny))
        active_indices = list(range(len(m.sectors)))
        iteration = 0
        total_claimed = sum(len(sector.get_coordinates()) for sector in m.sectors)
        range_width = 3
        while len(active_indices) > 0:
            iteration += 1
            min_dist_sq_in_queues = None
            for i in active_indices:
                if len(queues[i]) == 0:
                    continue
                for dist_sq, x, y in queues[i]:
                    if not claimed[y, x]:
                        if min_dist_sq_in_queues is None or dist_sq < min_dist_sq_in_queues:
                            min_dist_sq_in_queues = dist_sq
                        break
            if min_dist_sq_in_queues is None:
                break
            min_dist_sq = min_dist_sq_in_queues
            min_dist = int(np.sqrt(min_dist_sq))
            max_dist = min_dist + range_width
            max_dist_sq = max_dist ** 2
            new_active_indices = []
            pixels_claimed_this_iteration = 0
            for i in active_indices:
                if len(queues[i]) == 0:
                    continue
                px, py = m.sectors[i].get_central_point()
                while len(queues[i]) > 0:
                    dist_sq, x, y = queues[i][0]
                    if claimed[y, x]:
                        queues[i].pop(0)
                        queue_sets[i].discard((x, y))
                        continue
                    if dist_sq < min_dist_sq or dist_sq > max_dist_sq:
                        break
                    queues[i].pop(0)
                    queue_sets[i].discard((x, y))
                    if not valid_mask[y, x]:
                        continue
                    claimed[y, x] = True
                    pixel_owner[y, x] = i
                    m.sectors[i].add_coordinate((x, y))
                    pixels_claimed_this_iteration += 1
                    for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < img_w and 0 <= ny < img_h:
                            if not valid_mask[ny, nx]:
                                continue
                            if claimed[ny, nx]:
                                owner_id = pixel_owner[ny, nx]
                                if owner_id != -1 and owner_id != i:
                                    m.__note_adjacency(i, owner_id)
                            else:
                                if (nx, ny) not in queue_sets[i]:
                                    neighbor_dist_sq = (nx - px) ** 2 + (ny - py) ** 2
                                    bisect.insort(queues[i], (neighbor_dist_sq, nx, ny))
                                    queue_sets[i].add((nx, ny))
                    break
                if len(queues[i]) > 0:
                    new_active_indices.append(i)
            total_claimed += pixels_claimed_this_iteration
            min_dist = int(np.sqrt(min_dist_sq))
            max_dist = int(np.sqrt(max_dist_sq))
            if not pixels_claimed_this_iteration:
                break
            active_indices = new_active_indices
        m.img_sectors = m.img_points.copy()
        img_sectors_array = np.array(m.img_sectors)
        used_colors = []
        for sector in m.sectors:
            sector_pixels = sector.get_coordinates()
            if len(sector_pixels) == 0:
                continue
            while True:
                color_r, color_g, color_b = random.randint(50, 200), random.randint(50, 200), random.randint(50, 200)
                sector_color, color_tuple = np.array([color_r, color_g, color_b]), (color_r, color_g, color_b)
                if color_tuple not in used_colors: used_colors.append(color_tuple); break
            sector.set_color(sector_color)
            for x, y in sector_pixels:
                if img_sectors_array.ndim == 3:
                    img_sectors_array[y, x] = sector_color
                else:
                    img_sectors_array[y, x] = sector_color[0]
        m.img_sectors = Image.fromarray(img_sectors_array)
        m.__display_image(m.img_sectors)
        m.current_stage = 2
        m.__update_button_states()

    def __on_toggle (m):
        if m.current_stage != 3: return
        m.show_river_system_view = not m.show_river_system_view
        m.__display_image(m.img_rivers_by_system if m.show_river_system_view else m.img_rivers)

    def __on_rivers (m):
        if m.img_sectors is None or len(m.sectors) == 0: return
        img_array = np.array(m.img_sectors)
        img_h, img_w = img_array.shape[:2]
        color_ocean_arr = np.array(m.color_ocean)
        is_3d = img_array.ndim == 3
        if is_3d:
            is_ocean_mask = np.all(img_array == color_ocean_arr, axis=2)
        else:
            is_ocean_mask = (img_array == color_ocean_arr[0])
        coastal_sectors = []
        for sector in m.sectors:
            sector_pixels = sector.get_coordinates()
            if len(sector_pixels) == 0:
                continue
            is_coastal = False
            for x, y in sector_pixels:
                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < img_w and 0 <= ny < img_h:
                        if is_ocean_mask[ny, nx]:
                            is_coastal = True
                            break
                if is_coastal:
                    break
            if is_coastal:
                sector.set_is_coastal(True)
                sector.set_river_system_id(sector.get_sector_id())
                coastal_sectors.append(sector.get_sector_id())
        if len(coastal_sectors) == 0: return
        river_system_colors = {}
        for coastal_id in coastal_sectors:
            color_r, color_g, color_b = random.randint(50, 200), random.randint(50, 200), random.randint(50, 200)
            river_system_colors[coastal_id] = np.array([color_r, color_g, color_b])
        claimed = set(coastal_sectors)
        queue = list(coastal_sectors)
        river_connections = set()
        while len(queue) > 0:
            new_queue = []
            for sector_idx in queue:
                sector = m.sectors[sector_idx]
                claimed_any = False
                for adj_sector_id in sector.get_connected_sectors():
                    if adj_sector_id not in claimed:
                        claimed.add(adj_sector_id)
                        adj_sector = m.sectors[adj_sector_id]
                        adj_sector.set_river_system_id(sector.get_river_system_id())
                        river_connections.add((sector_idx, adj_sector_id))
                        new_queue.append(adj_sector_id)
                        claimed_any = True
                if claimed_any:
                    new_queue.append(sector_idx)
            queue = new_queue
        m.img_rivers = m.img_sectors.copy()
        draw = ImageDraw.Draw(m.img_rivers)
        for i, j in river_connections:
            px1, py1 = m.sectors[i].get_central_point()
            px2, py2 = m.sectors[j].get_central_point()
            draw.line([(px1, py1), (px2, py2)], fill=(0, 100, 255), width=2)
        m.img_rivers_by_system = m.img_sectors.copy()
        img_rivers_by_system_array = np.array(m.img_rivers_by_system)
        for sector in m.sectors:
            sector_pixels = sector.get_coordinates()
            river_system_id = sector.get_river_system_id()
            if river_system_id != -1 and river_system_id in river_system_colors:
                system_color = river_system_colors[river_system_id]
                for x, y in sector_pixels:
                    if img_rivers_by_system_array.ndim == 3:
                        img_rivers_by_system_array[y, x] = system_color
                    else:
                        img_rivers_by_system_array[y, x] = system_color[0]
        m.img_rivers_by_system = Image.fromarray(img_rivers_by_system_array)
        draw_rivers = ImageDraw.Draw(m.img_rivers_by_system)
        for i, j in river_connections:
            px1, py1 = m.sectors[i].get_central_point()
            px2, py2 = m.sectors[j].get_central_point()
            draw_rivers.line([(px1, py1), (px2, py2)], fill=(0, 100, 255), width=2)
        m.__display_image(m.img_rivers)
        m.current_stage = 3
        m.show_river_system_view = False
        m.__update_button_states()

    def __display_start_image (m):
        if m.image is None: return
        img_w, img_h = m.image.size
        zoom = 2
        new_w, new_h = img_w * zoom, img_h * zoom
        m.img_start = m.image.resize((new_w, new_h), Image.Resampling.NEAREST)
        m.canvas.config(width=new_w, height=new_h)
        photo = ImageTk.PhotoImage(m.img_start)
        m.canvas.delete("all")
        m.canvas.create_image(new_w // 2, new_h // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo
        m.canvas_zoom = zoom
        m.canvas_offset_x = 0
        m.canvas_offset_y = 0

    def __setup_ui (m):
        m.frm_main = tk.Frame(m.dialog, padx=10, pady=10)
        m.frm_main.pack()
        btn_frame = tk.Frame(m.frm_main)
        btn_frame.pack(pady=5)
        m.btn_back = tk.Button(btn_frame, text="Back", width=10, command=m.__on_back)
        m.btn_back.pack(side=tk.LEFT, padx=5)
        m.btn_points = tk.Button(btn_frame, text="Points", width=10, command=m.__on_points)
        m.btn_points.pack(side=tk.LEFT, padx=5)
        m.btn_sectors = tk.Button(btn_frame, text="Sectors", width=10, command=m.__on_sectors)
        m.btn_sectors.pack(side=tk.LEFT, padx=5)
        m.btn_rivers = tk.Button(btn_frame, text="Rivers", width=10, command=m.__on_rivers)
        m.btn_rivers.pack(side=tk.LEFT, padx=5)
        m.btn_toggle = tk.Button(btn_frame, text="Toggle", width=10, command=m.__on_toggle)
        m.btn_toggle.pack(side=tk.LEFT, padx=5)
        m.canvas = tk.Canvas(m.frm_main, width=800, height=600, bg="white")
        m.canvas.pack(pady=5)

    def __init__ (m, parent, img_path, color_ocean, color_mtn, color_grass):
        m.parent = parent
        m.img_path = img_path
        m.color_ocean = color_ocean
        m.color_mtn = color_mtn
        m.color_grass = color_grass
        m.current_stage = 0

        m.img_start = None
        m.img_points = None
        m.img_sectors = None
        m.img_rivers = None
        m.img_rivers_by_system = None

        m.sectors = []
        m.show_river_system_view = False
        
        m.image = Image.open(img_path) if os.path.exists(img_path) else None
        
        m.dialog = tk.Toplevel(parent)
        m.dialog.title("Get Sectors")
        m.dialog.transient(parent)
        m.dialog.grab_set()
        m.__setup_ui()
        m.__update_button_states()
        m.__display_start_image()
        m.dialog.wait_window()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    color_ocean = (32, 26, 120)
    color_mtn = (100, 50, 25)
    color_grass = (121, 189, 36)
    img_path = "/home/w/Projects/rts-proto-map/first-test/cont001.png"
    root = tk.Tk()
    app = DialogGetSectors(root, img_path, color_ocean, color_mtn, color_grass)
    root.mainloop()

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
