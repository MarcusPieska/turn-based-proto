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
#=> - Class: DialogGetSectors -
#================================================================================================================================#

class DialogGetSectors:

    def __update_button_states (m):
        m.btn_back.config(state=tk.NORMAL if m.current_stage > 0 else tk.DISABLED)
        m.btn_points.config(state=tk.NORMAL if m.current_stage == 0 else tk.DISABLED)
        m.btn_sectors.config(state=tk.NORMAL if m.current_stage == 1 else tk.DISABLED)
        m.btn_connect.config(state=tk.NORMAL if m.current_stage == 2 else tk.DISABLED)
        m.btn_rivers.config(state=tk.NORMAL if m.current_stage == 3 else tk.DISABLED)

    def __on_back (m):
        if m.current_stage > 0:
            m.current_stage -= 1
            m.__update_button_states()

    def __display_image (m, img):
        if img is None:
            return
        photo = ImageTk.PhotoImage(img)
        m.canvas.delete("all")
        m.canvas.create_image(img.size[0] // 2, img.size[1] // 2, anchor=tk.CENTER, image=photo)
        m.canvas.image = photo

    def __on_points (m):
        if m.img_start is None or m.image is None:
            return
        m.points = []
        img_orig_array = np.array(m.image)
        img_zoomed_array = np.array(m.img_start)
        orig_h, orig_w = img_orig_array.shape[:2]
        zoomed_h, zoomed_w = img_zoomed_array.shape[:2]
        zoom = m.canvas_zoom
        color_ocean_arr = np.array(m.color_ocean)
        color_mtn_arr = np.array(m.color_mtn)
        color_grass_arr = np.array(m.color_grass)
        for y_zoomed in range(zoomed_h):
            for x_zoomed in range(zoomed_w):
                x_orig = x_zoomed // zoom
                y_orig = y_zoomed // zoom
                if x_orig >= orig_w or y_orig >= orig_h:
                    continue
                pixel_color = img_orig_array[y_orig, x_orig]
                if img_orig_array.ndim == 3:
                    is_ocean = np.array_equal(pixel_color, color_ocean_arr)
                    is_mtn = np.array_equal(pixel_color, color_mtn_arr)
                else:
                    is_ocean = pixel_color == color_ocean_arr[0]
                    is_mtn = pixel_color == color_mtn_arr[0]
                if not is_ocean and not is_mtn:
                    if img_zoomed_array.ndim == 3:
                        img_zoomed_array[y_zoomed, x_zoomed] = color_grass_arr
                    else:
                        img_zoomed_array[y_zoomed, x_zoomed] = color_grass_arr[0]
        m.img_start = Image.fromarray(img_zoomed_array)
        min_dist = 20
        consecutive_failures = 0
        max_failures = 200
        while consecutive_failures < max_failures:
            x_zoomed = random.randint(0, zoomed_w - 1)
            y_zoomed = random.randint(0, zoomed_h - 1)
            x_orig = x_zoomed // zoom
            y_orig = y_zoomed // zoom
            if x_orig >= orig_w or y_orig >= orig_h:
                consecutive_failures += 1
                continue
            pixel_color = img_orig_array[y_orig, x_orig]
            if img_orig_array.ndim == 3:
                is_ocean = np.array_equal(pixel_color, color_ocean_arr)
                is_mtn = np.array_equal(pixel_color, color_mtn_arr)
            else:
                is_ocean = pixel_color == color_ocean_arr[0]
                is_mtn = pixel_color == color_mtn_arr[0]
            if is_ocean or is_mtn:
                consecutive_failures += 1
                continue
            too_close = False
            for px, py in m.points:
                dist = np.sqrt((x_zoomed - px) ** 2 + (y_zoomed - py) ** 2)
                if dist < min_dist:
                    too_close = True
                    break
            if too_close:
                consecutive_failures += 1
                continue
            m.points.append((x_zoomed, y_zoomed))
            consecutive_failures = 0
        m.img_points = m.img_start.copy()
        draw = ImageDraw.Draw(m.img_points)
        for x, y in m.points:
            draw.ellipse([x - 3, y - 3, x + 3, y + 3], fill=(255, 0, 0), outline=(255, 0, 0))
        m.__display_image(m.img_points)
        m.current_stage = 1
        m.__update_button_states()

    def __on_sectors (m):
        if m.img_points is None or len(m.points) == 0:
            return
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
        m.sectors = [[] for _ in range(len(m.points))]
        queues = [[] for _ in range(len(m.points))]
        queue_sets = [set() for _ in range(len(m.points))]
        for i, (px, py) in enumerate(m.points):
            if 0 <= px < img_w and 0 <= py < img_h and valid_mask[py, px]:
                claimed[py, px] = True
                m.sectors[i].append((px, py))
                for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    nx, ny = px + dx, py + dy
                    if 0 <= nx < img_w and 0 <= ny < img_h and not claimed[ny, nx] and valid_mask[ny, nx]:
                        dist_sq = (nx - px) ** 2 + (ny - py) ** 2
                        if (nx, ny) not in queue_sets[i]:
                            bisect.insort(queues[i], (dist_sq, nx, ny))
                            queue_sets[i].add((nx, ny))
        active_indices = list(range(len(m.points)))
        iteration = 0
        total_claimed = sum(len(sector) for sector in m.sectors)
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
                px, py = m.points[i]
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
                    m.sectors[i].append((x, y))
                    pixels_claimed_this_iteration += 1
                    for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < img_w and 0 <= ny < img_h and not claimed[ny, nx] and valid_mask[ny, nx]:
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
        for sector_pixels in m.sectors:
            if len(sector_pixels) == 0:
                continue
            color_r = random.randint(50, 200)
            color_g = random.randint(50, 200)
            color_b = random.randint(50, 200)
            sector_color = np.array([color_r, color_g, color_b])
            for x, y in sector_pixels:
                if img_sectors_array.ndim == 3:
                    img_sectors_array[y, x] = sector_color
                else:
                    img_sectors_array[y, x] = sector_color[0]
        m.img_sectors = Image.fromarray(img_sectors_array)
        m.__display_image(m.img_sectors)
        m.current_stage = 2
        m.__update_button_states()

    def __count_color_transitions (m, x1, y1, x2, y2, img_array, blocked_mask):
        dx = abs(x2 - x1)
        dy = abs(y2 - y1)
        sx = 1 if x1 < x2 else -1
        sy = 1 if y1 < y2 else -1
        err = dx - dy
        x, y = x1, y1
        transitions = 0
        prev_color = None
        is_3d = img_array.ndim == 3
        while True:
            if blocked_mask[y, x]:
                return -1
            if is_3d:
                current_color = tuple(img_array[y, x])
            else:
                current_color = img_array[y, x]
            if prev_color is not None and current_color != prev_color:
                transitions += 1
            prev_color = current_color
            if x == x2 and y == y2:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x += sx
            if e2 < dx:
                err += dx
                y += sy
        return transitions

    def __on_connect (m):
        if m.img_sectors is None or len(m.points) < 2:
            return
        img_array = np.array(m.img_sectors)
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
        blocked_mask = is_ocean_mask | is_mtn_mask
        num_points = len(m.points)
        distance_table = np.zeros((num_points, num_points))
        for i in range(num_points):
            px1, py1 = m.points[i]
            for j in range(i + 1, num_points):
                px2, py2 = m.points[j]
                dist_sq = (px1 - px2) ** 2 + (py1 - py2) ** 2
                distance_table[i, j] = dist_sq
                distance_table[j, i] = dist_sq
        m.connections = []
        for i in range(num_points):
            distances = [(distance_table[i, j], j) for j in range(num_points) if i != j]
            distances.sort()
            closest_5 = distances[:5]
            px1, py1 = m.points[i]
            for dist_sq, j in closest_5:
                px2, py2 = m.points[j]
                transitions = m.__count_color_transitions(px1, py1, px2, py2, img_array, blocked_mask)
                if transitions == 1:
                    if (i, j) not in m.connections and (j, i) not in m.connections:
                        m.connections.append((i, j))
        m.img_connections = m.img_sectors.copy()
        draw = ImageDraw.Draw(m.img_connections)
        for i, j in m.connections:
            px1, py1 = m.points[i]
            px2, py2 = m.points[j]
            draw.line([(px1, py1), (px2, py2)], fill=(255, 255, 255), width=2)
        m.__display_image(m.img_connections)
        m.current_stage = 3
        m.__update_button_states()

    def __on_rivers (m):
        if m.img_sectors is None or len(m.sectors) == 0 or len(m.connections) == 0:
            return
        img_array = np.array(m.img_sectors)
        img_h, img_w = img_array.shape[:2]
        color_ocean_arr = np.array(m.color_ocean)
        is_3d = img_array.ndim == 3
        if is_3d:
            is_ocean_mask = np.all(img_array == color_ocean_arr, axis=2)
        else:
            is_ocean_mask = (img_array == color_ocean_arr[0])
        coastal_sectors = []
        for sector_idx, sector_pixels in enumerate(m.sectors):
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
                coastal_sectors.append(sector_idx)
        if len(coastal_sectors) == 0:
            return
        claimed = set(coastal_sectors)
        queue = list(coastal_sectors)
        m.rivers = []
        sector_adjacency = {}
        for i, j in m.connections:
            if i not in sector_adjacency:
                sector_adjacency[i] = []
            if j not in sector_adjacency:
                sector_adjacency[j] = []
            sector_adjacency[i].append(j)
            sector_adjacency[j].append(i)
        while len(queue) > 0:
            new_queue = []
            for sector_idx in queue:
                if sector_idx not in sector_adjacency:
                    continue
                claimed_any = False
                for adj_sector in sector_adjacency[sector_idx]:
                    if adj_sector not in claimed:
                        claimed.add(adj_sector)
                        m.rivers.append((sector_idx, adj_sector))
                        new_queue.append(adj_sector)
                        claimed_any = True
                if claimed_any:
                    new_queue.append(sector_idx)
            queue = new_queue
        m.img_rivers = m.img_sectors.copy()
        draw = ImageDraw.Draw(m.img_rivers)
        for i, j in m.rivers:
            px1, py1 = m.points[i]
            px2, py2 = m.points[j]
            draw.line([(px1, py1), (px2, py2)], fill=(0, 100, 255), width=2)
        m.__display_image(m.img_rivers)
        m.current_stage = 4
        m.__update_button_states()

    def __display_start_image (m):
        if m.image is None:
            return
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
        m.btn_connect = tk.Button(btn_frame, text="Connect", width=10, command=m.__on_connect)
        m.btn_connect.pack(side=tk.LEFT, padx=5)
        m.btn_rivers = tk.Button(btn_frame, text="Rivers", width=10, command=m.__on_rivers)
        m.btn_rivers.pack(side=tk.LEFT, padx=5)
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
        m.img_connections = None
        m.img_rivers = None

        m.points = []
        m.sectors = []
        m.connections = []
        m.rivers = []
        
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
