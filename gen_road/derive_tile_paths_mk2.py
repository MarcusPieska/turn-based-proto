#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import tkinter as tk
import random

from get_bezier_line import get_line_pixels, draw_road

#================================================================================================================================#
#=> - Global Args -
#================================================================================================================================#

GRID_NUM_COLS = 16
GRID_NUM_ROWS = 16
TILE_CENTER_DOT_RADIUS = 3

even_row_offsets = {
    "n":  (0, -2),
    "ne": (0, -1),
    "e":  (1, 0),
    "se": (0, 1),
    "s":  (0, 2),
    "sw": (-1, 1),
    "w":  (-1, 0),
    "nw": (-1, -1),
}

odd_row_offsets = {
    "n":  (0, -2),
    "ne": (1, -1),
    "e":  (1, 0),
    "se": (1, 1),
    "s":  (0, 2),
    "sw": (0, 1),
    "w":  (-1, 0),
    "nw": (0, -1),
}

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def get_adjacent_tiles (row, col, angles, num_rows=GRID_NUM_ROWS, num_cols=GRID_NUM_COLS):
    offsets = odd_row_offsets if row % 2 == 1 else even_row_offsets
    neighbors, selected_angles = [], []
    for (dx, dy), angle in zip(offsets.values(), angles):
        nc = col + dx
        nr = row + dy
        if 0 <= nc < num_cols and 0 <= nr < num_rows:
            neighbors.append((nr, nc))
            selected_angles.append(angle)
    return neighbors, selected_angles

#================================================================================================================================#
#=> - Class: TileCropper (grid viewer only)
#================================================================================================================================#

class TileCropper:

    def __toggleGrid (m):
        m.show_grid = not m.show_grid
        m.__displayScene ()

    def __toggleCenters (m):
        m.show_centers = not m.show_centers
        m.__displayScene ()

    def __toggleOffsetCenters (m):
        m.show_offset_centers = not m.show_offset_centers
        m.__displayScene ()

    def __toggleLinks (m):
        m.show_links = not m.show_links
        m.__displayScene ()

    def __toggleLinkOffsets (m):
        m.use_offset_links = not m.use_offset_links
        m.__displayScene ()

    def __onClick (m, event):
        click_x, click_y = event.x, event.y
        half_height = m.tile_height // 2
        half_width = m.tile_width // 2
        best_row = None
        best_col = None
        best_dist2 = None
        for ty in range(m.num_rows):
            row_offset = 0 if ty % 2 == 0 else half_width
            cy = m.min_y + ty * half_height + half_height
            for tx in range(m.num_cols):
                cx = m.min_x + tx * m.tile_width + row_offset + half_width
                dx = cx - click_x
                dy = cy - click_y
                d2 = dx * dx + dy * dy
                if (best_dist2 is None) or (d2 < best_dist2):
                    best_dist2 = d2
                    best_row = ty
                    best_col = tx
        if best_row is not None and best_col is not None:
            m.active_tiles[best_row][best_col] = True
        m.__displayScene ()

    def __onRightClick (m, event):
        click_x, click_y = event.x, event.y
        half_height = m.tile_height // 2
        half_width = m.tile_width // 2
        best_row = None
        best_col = None
        best_dist2 = None
        for ty in range(m.num_rows):
            row_offset = 0 if ty % 2 == 0 else half_width
            cy = m.min_y + ty * half_height + half_height
            for tx in range(m.num_cols):
                cx = m.min_x + tx * m.tile_width + row_offset + half_width
                dx = cx - click_x
                dy = cy - click_y
                d2 = dx * dx + dy * dy
                if (best_dist2 is None) or (d2 < best_dist2):
                    best_dist2 = d2
                    best_row = ty
                    best_col = tx
        if best_row is not None and best_col is not None:
            m.active_tiles[best_row][best_col] = False
        m.__displayScene ()

    def __drawGrid (m, cnv, width, height, tile_w, tile_h, zoom=1.0):
        half_w = int((tile_w // 2) * zoom)
        half_h = int((tile_h // 2) * zoom)

        half_width = tile_w // 2
        half_height = tile_h // 2

        m.tile_polys = []
        for tx in range(m.num_cols):
            for ty in range(m.num_rows):
                row_offset = 0 if ty % 2 == 0 else half_width
                cy = m.min_y + ty * half_height + half_height
                cx = m.min_x + tx * tile_w + row_offset + half_width
                top = (cx, cy - half_h)
                right = (cx + half_w, cy)
                bottom = (cx, cy + half_h)
                left = (cx - half_w, cy)
                m.tile_polys.append([top, right, bottom, left])
                if m.show_grid:
                    cnv.create_line(top[0], top[1], right[0], right[1], fill="gray", width=1, tags="grid")
                    cnv.create_line(right[0], right[1], bottom[0], bottom[1], fill="gray", width=1, tags="grid")
                    cnv.create_line(bottom[0], bottom[1], left[0], left[1], fill="gray", width=1, tags="grid")
                    cnv.create_line(left[0], left[1], top[0], top[1], fill="gray", width=1, tags="grid")

                if m.show_centers:
                    r = TILE_CENTER_DOT_RADIUS
                    cnv.create_oval(cx - r, cy - r, cx + r, cy + r, fill="gray", outline="", tags="centers")

                if m.show_offset_centers:
                    off_dx, off_dy = m.offset_matrix[ty][tx]
                    ox = cx + off_dx
                    oy = cy + off_dy
                    r = TILE_CENTER_DOT_RADIUS
                    cnv.create_oval(ox - r, oy - r, ox + r, oy + r, fill="gray", outline="", tags="offset_centers") 

                if m.show_links:
                    neighbors, selected_angles = get_adjacent_tiles(ty, tx, m.angles, m.num_rows, m.num_cols)
                    for (nr, nc), (ang0, ang1) in zip(neighbors, selected_angles):
                        if not m.active_tiles[ty][tx] or not m.active_tiles[nr][nc]:
                            continue
                        if nr < ty or (nr == ty and nc <= tx):
                            continue
                        row_offset_n = 0 if nr % 2 == 0 else half_width
                        cy_n = m.min_y + nr * half_height + half_height
                        cx_n = m.min_x + nc * tile_w + row_offset_n + half_width
                        if m.use_offset_links:
                            off_dx_a, off_dy_a = m.offset_matrix[ty][tx]
                            off_dx_b, off_dy_b = m.offset_matrix[nr][nc]
                            ax = cx + off_dx_a
                            ay = cy + off_dy_a
                            bx = cx_n + off_dx_b
                            by = cy_n + off_dy_b
                        else:
                            ax = cx
                            ay = cy
                            bx = cx_n
                            by = cy_n
                        pixels = get_line_pixels(ax, ay, bx, by, ang0, ang1)
                        inner_c, outer_c = "darkgoldenrod", "saddlebrown"
                        draw_road(cnv, pixels, inner_r=1, outer_r=2, inner_color=inner_c, outer_color=outer_c, tag="links")

    def __displayScene (m):
        m.canv_main.delete("all")

        m.__drawGrid(m.canv_main, m.canv_w, m.canv_h, m.tile_width, m.tile_height, 1.0)
        m.canv_main.tag_raise("grid")
        m.canv_main.tag_raise("centers")
        m.canv_main.tag_raise("offset_centers")
        m.canv_main.tag_raise("links")

    def __init__ (m, root, tile_width, tile_height):
        m.root = root
        m.tile_width = tile_width
        m.tile_height = tile_height
        xo, yo = 20, 10
        m.offsets = [(0, -yo), (xo, 0), (0, yo), (-xo, 0)]
        
        m.angles = []

        m.angles.append([90, -90])
        m.angles.append([-26.57+180, -26.57])
        m.angles.append([0, 180])
        m.angles.append([26.57, 26.57+180])

        m.angles.append([90, -90])
        m.angles.append([-26.57+180, -26.57])
        m.angles.append([0, 180])
        m.angles.append([26.57, 26.57+180])

        num_tiles = GRID_NUM_COLS * GRID_NUM_ROWS
        num_offset_types = len(m.offsets)
        assert num_tiles % num_offset_types == 0, "*** Error: num_tiles must be divisible by number of offsets"
        count_per_offset = num_tiles // num_offset_types
        flat_offsets = []
        for off in m.offsets:
            flat_offsets.extend([off] * count_per_offset)
        for i in range(len(flat_offsets)):
            rand_xo = random.randint(-5, 5)
            rand_yo = random.randint(-5, 5)
            flat_offsets[i] = (flat_offsets[i][0] + rand_xo, flat_offsets[i][1] + rand_yo)
        random.shuffle(flat_offsets)

        m.offset_matrix = []
        idx = 0
        for ty in range(GRID_NUM_ROWS):
            row = []
            for tx in range(GRID_NUM_COLS):
                row.append(flat_offsets[idx])
                idx += 1
            m.offset_matrix.append(row)

        m.active_tiles = []
        for ty in range(GRID_NUM_ROWS):
            row = []
            for tx in range(GRID_NUM_COLS):
                row.append(False)
            m.active_tiles.append(row)

        m.show_grid = True
        m.show_centers = True
        m.show_offset_centers = True
        m.show_links = True
        m.use_offset_links = False

        m.num_cols = GRID_NUM_COLS
        m.num_rows = GRID_NUM_ROWS
        m.tile_polys = []

        m.min_x = tile_width
        m.min_y = tile_height
        half_height = tile_height // 2
        grid_height = (m.num_rows + 1) * half_height
        m.canv_w = m.min_x * 2 + m.num_cols * tile_width
        m.canv_h = m.min_y * 2 + grid_height

        m.top_frame = tk.Frame(root)
        m.top_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        m.grid_btn = tk.Button(m.top_frame, text="Toggle Grid", command=m.__toggleGrid, width=15)
        m.grid_btn.pack(side=tk.LEFT, padx=5)
        m.centers_btn = tk.Button(m.top_frame, text="Toggle Centers", command=m.__toggleCenters, width=15)
        m.centers_btn.pack(side=tk.LEFT, padx=5)
        m.offset_centers_btn = tk.Button(m.top_frame, text="Toggle Offset Centers", command=m.__toggleOffsetCenters, width=18)
        m.offset_centers_btn.pack(side=tk.LEFT, padx=5)
        m.links_btn = tk.Button(m.top_frame, text="Toggle Links", command=m.__toggleLinks, width=15)
        m.links_btn.pack(side=tk.LEFT, padx=5)
        m.link_offsets_btn = tk.Button(m.top_frame, text="Toggle Link Offsets", command=m.__toggleLinkOffsets, width=18)
        m.link_offsets_btn.pack(side=tk.LEFT, padx=5)

        m.canv_frame = tk.Frame(root)
        m.canv_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=5, pady=5)
        m.canv_main = tk.Canvas(m.canv_frame, width=m.canv_w, height=m.canv_h, bg="white")
        m.canv_main.pack(side=tk.LEFT, padx=5)

        m.canv_main.bind("<Button-1>", m.__onClick)
        m.canv_main.bind("<Button-3>", m.__onRightClick)

        m.__displayScene ()

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = tk.Tk()
    root.title("Tile Cropper - Grid Viewer")
    tile_width = 100
    tile_height = 50
    editor = TileCropper (root, tile_width, tile_height)
    root.mainloop ()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#