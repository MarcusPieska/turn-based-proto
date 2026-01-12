#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import time
import math
import random
import ctypes
import numpy as np
from PIL import Image
from line_gen import LineGenerator

#================================================================================================================================#
#=> - Setup C library -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "map_overlays.so")
if not os.path.exists(lib_path):
    print("Error: %s not found. Please compile map_overlays.cpp first." % lib_path)
    sys.exit(1)

lib = ctypes.CDLL(lib_path)

class pt(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.distance_overlay.restype = None
lib.distance_overlay.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_int)
]

lib.wander_up.restype = None
lib.wander_up.argtypes = [
    ctypes.POINTER(ctypes.c_int), 
    size, 
    pt, 
    ctypes.POINTER(pt)
]

lib.wander_up_radius.restype = ctypes.c_bool
lib.wander_up_radius.argtypes = [
    ctypes.POINTER(ctypes.c_int), 
    size, 
    pt, 
    ctypes.c_int, 
    ctypes.POINTER(pt)
]

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def distance_overlay_cpp (img, bg_col):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(bg_col, (list, tuple)):
        bg_col = np.array(bg_col, dtype=np.uint8)
    if bg_col.size != ch:
        bg_col = np.array([bg_col[0]], dtype=np.uint8) if ch == 1 else np.array(bg_col[:ch], dtype=np.uint8)
    sz = size(w, h)
    img_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    bg_col_ptr = bg_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    dist_out = np.zeros((h, w), dtype=np.int32)
    dist_ptr = dist_out.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    lib.distance_overlay(img_ptr, sz, ch, bg_col_ptr, dist_ptr)
    return dist_out

def wander_up_cpp (dist, start_pt):
    if not isinstance(dist, np.ndarray):
        dist = np.array(dist, dtype=np.int32)
    h, w = dist.shape
    sz = size(w, h)
    dist_ptr = dist.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    start = pt(float(start_pt[0]), float(start_pt[1]))
    end = pt()
    lib.wander_up(dist_ptr, sz, start, ctypes.byref(end))
    return (int(end.x), int(end.y))

def wander_up_radius_cpp (dist, start_pt, radius):
    if not isinstance(dist, np.ndarray):
        dist = np.array(dist, dtype=np.int32)
    h, w = dist.shape
    sz = size(w, h)
    dist_ptr = dist.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    start = pt(float(start_pt[0]), float(start_pt[1]))
    end = pt()
    found = lib.wander_up_radius(dist_ptr, sz, start, radius, ctypes.byref(end))
    return (int(end.x), int(end.y)), found

def generate_path_pixels (start_pt, end_pt, canvas_size):
    w, h = canvas_size
    dx = end_pt[0] - start_pt[0]
    dy = end_pt[1] - start_pt[1]
    total_dist = math.sqrt(dx * dx + dy * dy)
    if total_dist < 5:
        return [(int(start_pt[0]), int(start_pt[1])), (int(end_pt[0]), int(end_pt[1]))]
    angle = math.degrees(math.atan2(dy, dx))
    chunks = []
    remaining = total_dist
    while remaining > 0:
        chunk_size = min(random.randint(5, 15), remaining)
        chunks.append(chunk_size)
        remaining -= chunk_size
    if remaining > 0.1:
        chunks[-1] += remaining
    lg = LineGenerator()
    all_pixels = set()
    current_dist, bend_dir = 0, 1
    bend_factors = [2.5, 1.0, 1.5, 4.0, 1.5]
    peak_positions = [0.2, 0.5, 0.8]
    prev_end_px, prev_end_py = None, None
    for i, length in enumerate(chunks):
        t_start = current_dist / total_dist
        t_end = (current_dist + length) / total_dist
        seg_start_x = start_pt[0] + dx * t_start
        seg_start_y = start_pt[1] + dy * t_start
        seg_end_x = start_pt[0] + dx * t_end
        seg_end_y = start_pt[1] + dy * t_end
        bend = bend_factors[i % len(bend_factors)] * bend_dir
        peak = peak_positions[i % len(peak_positions)]
        pxs = lg.generateLinePixels(length, angle, bend_factor=bend, peak_position=peak, width=w, height=h, zoom=1.0)
        if len(pxs) == 0:
            current_dist += length
            bend_dir *= -1
            continue
        gen_start_px = pxs[0][0]
        gen_start_py = pxs[0][1]
        if prev_end_px is not None and prev_end_py is not None:
            offset_x = prev_end_px - gen_start_px
            offset_y = prev_end_py - (h - gen_start_py)
        else:
            offset_x = seg_start_x - gen_start_px
            offset_y = seg_start_y - (h - gen_start_py)
        gen_end_px = pxs[-1][0]
        gen_end_py = pxs[-1][1]
        if i == len(chunks) - 1:
            expected_end_x = gen_end_px + offset_x
            expected_end_y = h - gen_end_py + offset_y
            end_offset_x = int(end_pt[0]) - expected_end_x
            end_offset_y = int(end_pt[1]) - expected_end_y
            offset_x += end_offset_x
            offset_y += end_offset_y
        first_px_world = int(gen_start_px + offset_x)
        first_py_world = int(h - gen_start_py + offset_y)
        if prev_end_px is not None and prev_end_py is not None:
            if (first_px_world, first_py_world) != (prev_end_px, prev_end_py):
                lg_temp = LineGenerator()
                bridge_pxs = lg_temp._LineGenerator__getLineSegmentPixels(prev_end_px, prev_end_py, first_px_world, first_py_world, w, h)
                all_pixels.update(bridge_pxs)
        for px, py in pxs:
            px_world = int(px + offset_x)
            py_world = int(h - py + offset_y)
            all_pixels.add((px_world, py_world))
        last_px_world = int(gen_end_px + offset_x)
        last_py_world = int(h - gen_end_py + offset_y)
        all_pixels.add((last_px_world, last_py_world))
        prev_end_px = last_px_world
        prev_end_py = last_py_world
        current_dist += length
        bend_dir *= -1
    return list(all_pixels)

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_paths = []
    img_paths.append("../../../DEL_map_dist.png")
    bg_col = np.array([255, 255, 255], dtype=np.uint8)
    start_points = []
    start_points.append((118, 79))
    num_iterations = 10
    radius_set = [20,30,40, 100, 200]
    
    print ("\n======= C++ wander up radius iterative =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        dist = distance_overlay_cpp(img, bg_col)
        for i, start_pt in enumerate(start_points):
            t0 = time.perf_counter()
            points = [start_pt]
            current_pt = start_pt
            for iteration in range(num_iterations):
                for radius in radius_set:
                    next_pt, found = wander_up_radius_cpp(dist, current_pt, radius)
                    if found:
                        break
                if not found or next_pt == current_pt:
                    break
                points.append(next_pt)
                current_pt = next_pt
            path_pixels = []
            for j in range(len(points) - 1):
                segment_pixels = generate_path_pixels(points[j], points[j + 1], (w, h))
                path_pixels.extend(segment_pixels)
            t1 = time.perf_counter()
            out_img = np.zeros((h, w, 3), dtype=np.uint8)
            out_img[dist == 0] = [0, 0, 128]
            for px, py in path_pixels:
                if 0 <= px < w and 0 <= py < h:
                    out_img[py, px] = [255, 255, 255]
            Image.fromarray(out_img, mode='RGB').save(p.replace(".png", "_wander_up_%d.png" % i))
            print("%s: %d points, %.3f ms" % (p.split("/")[-1], len(points), (t1 - t0) * 1000))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
