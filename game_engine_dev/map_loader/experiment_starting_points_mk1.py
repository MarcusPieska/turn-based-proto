#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import math
import os
import random
import sys
import time

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Config -
#================================================================================================================================#

IMG_W = 1000
IMG_H = 1000
LATT_ROWS = 50
LATT_COLS = 50
DOT_R = 4
JITTER_FRAC = 0.5
OUT_N = 10
GRAY = (160, 160, 160)
MID_GRAY = (128, 128, 128)
WHITE = (255, 255, 255)
BLACK = (0, 0, 0)

MAP_PATH = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm"

TERR_ROWS = [
    (0, 255, 255, 255),
    (1, 14, 52, 112),
    (2, 38, 102, 188),
    (3, 118, 182, 242),
    (4, 34, 112, 48),
    (5, 50, 140, 78),
    (6, 76, 48, 30),
]

TERR_PLAINS = 4
TERR_HILLS = 5
LAND_SET = {TERR_PLAINS, TERR_HILLS}
PICK_N = 255

#================================================================================================================================#
#=> - Distribution -
#================================================================================================================================#

def basic_dist(rows, cols, w, h):
    dx = float(w) / float(cols)
    dy = float(h) / float(rows)
    jx = dx * JITTER_FRAC
    jy = dy * JITTER_FRAC
    pts = []
    for row in range(rows):
        for col in range(cols):
            bx = (float(col) + 0.5) * dx
            by = (float(row) + 0.5) * dy
            ox = random.uniform(-jx, jx)
            oy = random.uniform(-jy, jy)
            pts.append((bx + ox, by + oy))
    return pts

#================================================================================================================================#
#=> - Image -
#================================================================================================================================#

def mk_canvas(w, h, rgb):
    r, g, b = rgb
    return bytearray([r, g, b] * (w * h))

def set_px(buf, w, h, x, y, rgb):
    if x < 0 or y < 0 or x >= w or y >= h:
        return
    i = (y * w + x) * 3
    buf[i] = rgb[0]
    buf[i + 1] = rgb[1]
    buf[i + 2] = rgb[2]

def draw_dot(buf, w, h, cx, cy, rad, rgb):
    r2 = rad * rad
    x0 = max(0, int(math.floor(cx - rad)))
    x1 = min(w - 1, int(math.ceil(cx + rad)))
    y0 = max(0, int(math.floor(cy - rad)))
    y1 = min(h - 1, int(math.ceil(cy + rad)))
    for y in range(y0, y1 + 1):
        dy = float(y) + 0.5 - cy
        for x in range(x0, x1 + 1):
            dx = float(x) + 0.5 - cx
            if dx * dx + dy * dy <= r2:
                set_px(buf, w, h, x, y, rgb)

def draw_pts(buf, w, h, pts, rad, rgb):
    for x, y in pts:
        draw_dot(buf, w, h, x, y, rad, rgb)

def save_ppm(path, buf, w, h):
    hdr = "P6\n%d %d\n255\n" % (w, h)
    with open(path, "wb") as ptr:
        ptr.write(hdr.encode("ascii"))
        ptr.write(buf)

def save_dist_img(path, pts, w, h, rad):
    buf = mk_canvas(w, h, GRAY)
    draw_pts(buf, w, h, pts, rad, BLACK)
    save_ppm(path, buf, w, h)

#================================================================================================================================#
#=> - Terrain -
#================================================================================================================================#

def class_from_rgb(r, g, b):
    for cls, tr, tg, tb in TERR_ROWS:
        if tr == r and tg == g and tb == b:
            return cls
    return None

def load_terrain_ppm(path):
    with open(path, "rb") as ptr:
        magic = ptr.read(2)
        if magic != b"P6":
            return None
        line = b""
        while True:
            ch = ptr.read(1)
            if not ch:
                return None
            if ch == b"#":
                while ch and ch != b"\n":
                    ch = ptr.read(1)
                continue
            if ch in b" \t\r\n":
                continue
            line = ch
            break
        while True:
            ch = ptr.read(1)
            if not ch:
                return None
            if ch in b" \t\r\n":
                break
            line += ch
        parts = line.decode("ascii").split()
        while len(parts) < 3:
            ch = ptr.read(1)
            if not ch:
                return None
            if ch in b" \t\r\n":
                continue
            if ch == b"#":
                while ch and ch != b"\n":
                    ch = ptr.read(1)
                continue
            tok = ch
            while True:
                ch = ptr.read(1)
                if not ch or ch in b" \t\r\n":
                    break
                tok += ch
            parts.append(tok.decode("ascii"))
        w = int(parts[0])
        h = int(parts[1])
        maxv = int(parts[2])
        if maxv != 255:
            return None
        while True:
            ch = ptr.read(1)
            if not ch:
                return None
            if ch not in b" \t\r\n":
                ptr.seek(ptr.tell() - 1)
                break
        rgb = ptr.read(w * h * 3)
        if len(rgb) != w * h * 3:
            return None
    classes = []
    for i in range(w * h):
        j = i * 3
        cls = class_from_rgb(rgb[j], rgb[j + 1], rgb[j + 2])
        if cls is None:
            return None
        classes.append(cls)
    return w, h, classes

def terr_at(classes, w, h, x, y):
    px = int(x)
    py = int(y)
    if px < 0 or py < 0 or px >= w or py >= h:
        return None
    return classes[py * w + px]

def filt_land_pts(pts, classes, w, h, land_set):
    kept = []
    for x, y in pts:
        cls = terr_at(classes, w, h, x, y)
        if cls in land_set:
            kept.append((x, y))
    return kept

def mk_land_bg(classes, w, h, land_set, land_rgb, oth_rgb):
    buf = mk_canvas(w, h, oth_rgb)
    for y in range(h):
        row = y * w
        for x in range(w):
            cls = classes[row + x]
            if cls in land_set:
                set_px(buf, w, h, x, y, land_rgb)
    return buf

def save_land_dist_img(path, classes, pts, w, h, rad, land_set, land_rgb, oth_rgb):
    buf = mk_land_bg(classes, w, h, land_set, land_rgb, oth_rgb)
    draw_pts(buf, w, h, pts, rad, BLACK)
    save_ppm(path, buf, w, h)

#================================================================================================================================#
#=> - Spaced pick -
#================================================================================================================================#

def dist_sq(a, b):
    dx = a[0] - b[0]
    dy = a[1] - b[1]
    return dx * dx + dy * dy

def min_dist_sq(pt, picked):
    if not picked:
        return float("inf")
    best = float("inf")
    for q in picked:
        d = dist_sq(pt, q)
        if d < best:
            best = d
    return best

def part_pts(pts, k):
    n = len(pts)
    if k <= 0 or n == 0:
        return []
    segs = []
    for i in range(k):
        a = (i * n) // k
        b = ((i + 1) * n) // k
        segs.append(pts[a:b])
    return segs

def pick_spaced(pts, k):
    if k <= 0 or not pts:
        return []
    k = min(k, len(pts))
    segs = part_pts(pts, k)
    picked = []
    for seg in segs:
        if not seg:
            continue
        best_pt = None
        best_d = -1.0
        for p in seg:
            d = min_dist_sq(p, picked)
            if d > best_d:
                best_d = d
                best_pt = p
        if best_pt is not None:
            picked.append(best_pt)
    return picked

#================================================================================================================================#
#=> - Run -
#================================================================================================================================#

def run_basic_dist(out_n, w, h, rows, cols, rad):
    here = os.path.dirname(os.path.abspath(__file__))
    for i in range(out_n):
        pts = basic_dist(rows, cols, w, h)
        name = "DEL_basic_dist_%02d.ppm" % i
        path = os.path.join(here, name)
        save_dist_img(path, pts, w, h, rad)
        print("SAVED: %s  (%d points)" % (path, len(pts)))

def run_basic_dist_land(map_path, rows, cols, rad, land_set):
    here = os.path.dirname(os.path.abspath(__file__))
    loaded = load_terrain_ppm(map_path)
    if loaded is None:
        print("FAILED to load map: %s" % map_path)
        sys.exit(1)
    w, h, classes = loaded
    pts = basic_dist(rows, cols, w, h)
    kept = filt_land_pts(pts, classes, w, h, land_set)
    name = "DEL_basic_dist_land.ppm"
    path = os.path.join(here, name)
    save_land_dist_img(path, classes, kept, w, h, rad, land_set, MID_GRAY, WHITE)
    print("MAP:   %s  (%d x %d)" % (map_path, w, h))
    print("PTS:   %d total, %d kept on plains/hills" % (len(pts), len(kept)))
    print("SAVED: %s" % path)

def run_basic_dist_pick(map_path, rows, cols, rad, land_set, pick_n):
    here = os.path.dirname(os.path.abspath(__file__))
    loaded = load_terrain_ppm(map_path)
    if loaded is None:
        print("FAILED to load map: %s" % map_path)
        sys.exit(1)
    w, h, classes = loaded
    pts = basic_dist(rows, cols, w, h)
    kept = filt_land_pts(pts, classes, w, h, land_set)
    t0 = time.perf_counter()
    picked = pick_spaced(kept, pick_n)
    t1 = time.perf_counter()
    name = "DEL_basic_dist_pick.ppm"
    path = os.path.join(here, name)
    save_land_dist_img(path, classes, picked, w, h, rad, land_set, MID_GRAY, WHITE)
    print("MAP:   %s  (%d x %d)" % (map_path, w, h))
    print("PTS:   %d total, %d land, %d picked" % (len(pts), len(kept), len(picked)))
    print("PICK:  %d segments, spaced max-min selection" % pick_n)
    print("TIME:  pick step %.4f s" % (t1 - t0))
    print("SAVED: %s" % path)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    #run_basic_dist(OUT_N, IMG_W, IMG_H, LATT_ROWS, LATT_COLS, DOT_R)
    #run_basic_dist_land(MAP_PATH, LATT_ROWS, LATT_COLS, DOT_R, LAND_SET)
    run_basic_dist_pick(MAP_PATH, LATT_ROWS, LATT_COLS, DOT_R, LAND_SET, PICK_N)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
