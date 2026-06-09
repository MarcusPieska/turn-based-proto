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

LATT_ROWS = 50
LATT_COLS = 50
DOT_R = 4
JITTER_FRAC = 0.5
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

TERR_OCEAN = 1
TERR_SEA = 2
TERR_COASTAL = 3
TERR_PLAINS = 4
TERR_HILLS = 5
WATER_SET = {TERR_OCEAN, TERR_SEA, TERR_COASTAL}
LAND_SET = {TERR_PLAINS, TERR_HILLS}
PICK_N = 255
U16_INF = 65535
PROB_LO = 20
PROB_HI = 255
ADJ_R_IN = 10
ADJ_R_OUT = 20
MAX_PICK_TRIES = 500000

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

def save_gray_ppm(path, gray, w, h):
    buf = bytearray(w * h * 3)
    for i, g in enumerate(gray):
        j = i * 3
        buf[j] = g
        buf[j + 1] = g
        buf[j + 2] = g
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

def is_water(cls, water_set):
    return cls in water_set

def is_land(cls, water_set):
    return cls not in water_set

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
#=> - Water distance -
#================================================================================================================================#

def adj_water(w, h, classes, water_set, i):
    wi = w
    px = i % wi
    py = i // wi
    if px > 0 and is_water(classes[i - 1], water_set):
        return True
    if px + 1 < w and is_water(classes[i + 1], water_set):
        return True
    if py > 0 and is_water(classes[i - wi], water_set):
        return True
    if py + 1 < h and is_water(classes[i + wi], water_set):
        return True
    return False

def bfs_land_dist_to_water(classes, w, h, water_set):
    n = w * h
    dist = [U16_INF] * n
    q = []
    max_d = 0
    for i in range(n):
        if not is_land(classes[i], water_set):
            continue
        if not adj_water(w, h, classes, water_set, i):
            continue
        dist[i] = 1
        q.append(i)
        if max_d < 1:
            max_d = 1
    qh = 0
    while qh < len(q):
        i = q[qh]
        qh += 1
        cur = dist[i]
        if cur >= U16_INF - 1:
            continue
        nxt = cur + 1
        wi = w
        px = i % wi
        py = i // wi
        nbs = []
        if px > 0:
            nbs.append(i - 1)
        if px + 1 < w:
            nbs.append(i + 1)
        if py > 0:
            nbs.append(i - wi)
        if py + 1 < h:
            nbs.append(i + wi)
        for j in nbs:
            if not is_land(classes[j], water_set):
                continue
            if dist[j] != U16_INF:
                continue
            dist[j] = nxt
            if nxt > max_d:
                max_d = nxt
            q.append(j)
    return dist, max_d

def flip_norm_land_gray(d, max_d):
    if d <= 0 or d >= U16_INF:
        return 0
    if d == 1:
        return 255
    if max_d <= 1:
        return 255
    flip = max_d - d + 1
    return 1 + (flip - 1) * 254 // (max_d - 1)

def mk_water_dist_gray(classes, dist, w, h, max_d, water_set):
    gray = [0] * (w * h)
    for i in range(w * h):
        if is_water(classes[i], water_set):
            gray[i] = 0
            continue
        if not is_land(classes[i], water_set):
            gray[i] = 0
            continue
        gray[i] = flip_norm_land_gray(dist[i], max_d)
    return gray

def mk_pick_sel(classes, dist, w, h, max_d, water_set):
    sel = [0] * (w * h)
    for i in range(w * h):
        if is_water(classes[i], water_set):
            continue
        if not is_land(classes[i], water_set):
            continue
        d = dist[i]
        if d <= 0 or d >= U16_INF:
            continue
        if max_d <= 1:
            sel[i] = 1
        else:
            sel[i] = 1 + (d - 1) * 254 // (max_d - 1)
    return sel

#================================================================================================================================#
#=> - Offset kernel -
#================================================================================================================================#

def mk_adj_kern(r_in, r_out):
    rad = int(math.ceil(r_out))
    sz = rad * 2 + 1
    kern = []
    for dy in range(-rad, rad + 1):
        row = []
        for dx in range(-rad, rad + 1):
            r = math.sqrt(float(dx * dx + dy * dy))
            if r < r_in:
                row.append(0)
            elif r <= r_out:
                t = (r - r_in) / (r_out - r_in)
                row.append(int(t * 255.0 + 0.5))
            else:
                row.append(-1)
        kern.append(row)
    return kern, rad

def save_kern_ppm(path, kern, rad):
    sz = rad * 2 + 1
    gray = []
    for row in kern:
        for v in row:
            gray.append(0 if v < 0 else v)
    save_gray_ppm(path, gray, sz, sz)

def stamp_adj(sel, w, h, cx, cy, kern, rad):
    px = int(cx)
    py = int(cy)
    for dy in range(-rad, rad + 1):
        yy = py + dy
        if yy < 0 or yy >= h:
            continue
        kr = dy + rad
        for dx in range(-rad, rad + 1):
            xx = px + dx
            if xx < 0 or xx >= w:
                continue
            nv = kern[kr][dx + rad]
            if nv < 0:
                continue
            i = yy * w + xx
            if nv == 0:
                sel[i] = 0
            elif nv > sel[i]:
                sel[i] = nv

#================================================================================================================================#
#=> - Prob pick -
#================================================================================================================================#

def pick_prob_water(sel, kept, w, h, pick_n, kern, rad):
    picked = []
    picked_set = set()
    tries = 0
    while len(picked) < pick_n and tries < MAX_PICK_TRIES:
        tries += 1
        pt = random.choice(kept)
        if pt in picked_set:
            continue
        px = int(pt[0])
        py = int(pt[1])
        if px < 0 or py < 0 or px >= w or py >= h:
            continue
        prob = random.randint(PROB_LO, PROB_HI)
        val = sel[py * w + px]
        if val <= 0:
            continue
        if val < prob:
            picked.append(pt)
            picked_set.add(pt)
            stamp_adj(sel, w, h, px, py, kern, rad)
    return picked, tries

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

def run_mk2(map_path, rows, cols, rad, land_set, water_set, pick_n):
    here = os.path.dirname(os.path.abspath(__file__))
    loaded = load_terrain_ppm(map_path)
    if loaded is None:
        print("FAILED to load map: %s" % map_path)
        sys.exit(1)
    w, h, classes = loaded
    t0 = time.perf_counter()
    dist, max_d = bfs_land_dist_to_water(classes, w, h, water_set)
    t1 = time.perf_counter()
    gray = mk_water_dist_gray(classes, dist, w, h, max_d, water_set)
    dist_path = os.path.join(here, "DEL_mk2_00_water_dist.ppm")
    save_gray_ppm(dist_path, gray, w, h)
    kern, kern_rad = mk_adj_kern(ADJ_R_IN, ADJ_R_OUT)
    kern_path = os.path.join(here, "DEL_mk2_01_offset_matrix.ppm")
    save_kern_ppm(kern_path, kern, kern_rad)
    pts = basic_dist(rows, cols, w, h)
    kept = filt_land_pts(pts, classes, w, h, land_set)
    sel = mk_pick_sel(classes, dist, w, h, max_d, water_set)
    t2 = time.perf_counter()
    picked, tries = pick_prob_water(sel, kept, w, h, pick_n, kern, kern_rad)
    t3 = time.perf_counter()
    prob_path = os.path.join(here, "DEL_mk2_02_prob_pick.ppm")
    save_land_dist_img(prob_path, classes, picked, w, h, rad, land_set, MID_GRAY, WHITE)
    print("MAP:    %s  (%d x %d)" % (map_path, w, h))
    print("DIST:   max u16=%d  bfs %.4f s" % (max_d, t1 - t0))
    print("SAVED:  %s" % dist_path)
    print("SAVED:  %s  (%dx%d kernel)" % (kern_path, kern_rad * 2 + 1, kern_rad * 2 + 1))
    print("PTS:    %d total, %d land, %d picked (%d tries)" % (len(pts), len(kept), len(picked), tries))
    print("PICK:   prob [%d..%d], adj r<%d=0 r%d..%d ramp, %.4f s" % (PROB_LO, PROB_HI, ADJ_R_IN, ADJ_R_IN, ADJ_R_OUT, t3 - t2))
    print("SAVED:  %s" % prob_path)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    run_mk2(MAP_PATH, LATT_ROWS, LATT_COLS, DOT_R, LAND_SET, WATER_SET, PICK_N)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
