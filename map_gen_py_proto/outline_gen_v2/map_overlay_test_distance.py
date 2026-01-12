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
import ctypes
from collections import deque
import numpy as np
from PIL import Image

#================================================================================================================================#
#=> - Setup C library -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "map_overlays.so")
if not os.path.exists(lib_path):
    print("Error: %s not found. Please compile map_overlays.cpp first." % lib_path)
    sys.exit(1)

lib = ctypes.CDLL(lib_path)

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.distance_overlay_brute_force.restype = None
lib.distance_overlay_brute_force.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_int)
]

lib.distance_overlay.restype = None
lib.distance_overlay.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_int)
]

#================================================================================================================================#
#=> - Python distance overlay function -
#================================================================================================================================#

def distance_overlay_py (img, bg_col):
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
    dist_out = np.full((h, w), -1, dtype=np.int32)
    q = deque()
    for y in range(h):
        for x in range(w):
            if np.array_equal(img[y, x], bg_col):
                dist_out[y, x] = 0
                dx = [-1, 1, 0, 0]
                dy = [0, 0, -1, 1]
                for d in range(4):
                    nx = x + dx[d]
                    ny = y + dy[d]
                    if nx >= 0 and nx < w and ny >= 0 and ny < h:
                        if not np.array_equal(img[ny, nx], bg_col) and dist_out[ny, nx] == -1:
                            dist_out[ny, nx] = 1
                            q.append((nx, ny))
    while q:
        x, y = q.popleft()
        curr_dist = dist_out[y, x]
        dx = [-1, 1, 0, 0]
        dy = [0, 0, -1, 1]
        for d in range(4):
            nx = x + dx[d]
            ny = y + dy[d]
            if nx >= 0 and nx < w and ny >= 0 and ny < h:
                if not np.array_equal(img[ny, nx], bg_col) and dist_out[ny, nx] == -1:
                    dist_out[ny, nx] = curr_dist + 1
                    q.append((nx, ny))
    return dist_out

def distance_overlay_numpy (img, bg_col):
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
    bg_mask = np.all(img == bg_col, axis=2) if ch > 1 else (img[:, :, 0] == bg_col[0])
    dist_out = np.full((h, w), -1, dtype=np.int32)
    q = deque()
    for y in range(h):
        for x in range(w):
            if bg_mask[y, x]:
                dist_out[y, x] = 0
                dx = [-1, 1, 0, 0]
                dy = [0, 0, -1, 1]
                for d in range(4):
                    nx = x + dx[d]
                    ny = y + dy[d]
                    if nx >= 0 and nx < w and ny >= 0 and ny < h:
                        if not bg_mask[ny, nx] and dist_out[ny, nx] == -1:
                            dist_out[ny, nx] = 1
                            q.append((nx, ny))
    while q:
        x, y = q.popleft()
        curr_dist = dist_out[y, x]
        dx = [-1, 1, 0, 0]
        dy = [0, 0, -1, 1]
        for d in range(4):
            nx = x + dx[d]
            ny = y + dy[d]
            if nx >= 0 and nx < w and ny >= 0 and ny < h:
                if not bg_mask[ny, nx] and dist_out[ny, nx] == -1:
                    dist_out[ny, nx] = curr_dist + 1
                    q.append((nx, ny))
    return dist_out

#================================================================================================================================#
#=> - Helper function -
#================================================================================================================================#

def distance_overlay_brute_force_cpp (img, bg_col):
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
    lib.distance_overlay_brute_force(img_ptr, sz, ch, bg_col_ptr, dist_ptr)
    return dist_out

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

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_paths = []
    img_paths.append("../../../DEL_map_dist.png")
    bg_col = np.array([255, 255, 255], dtype=np.uint8)
    
    def save_dist_image(dist, path):
        dist_max = dist.max()
        if dist_max == 0:
            out_img = np.zeros((dist.shape[0], dist.shape[1], 3), dtype=np.uint8)
            out_img[dist == 0] = [0, 0, 128]
        else:
            dist_norm = (dist * 255.0 / dist_max).astype(np.uint8)
            out_img = np.stack([dist_norm, dist_norm, dist_norm], axis=2)
            out_img[dist == 0] = [0, 0, 128]
        Image.fromarray(out_img, mode='RGB').save(path)
    
    print ("\n======= C++ distance overlay brute force =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        dist = distance_overlay_brute_force_cpp(img, bg_col)
        t1 = time.perf_counter()
        save_dist_image(dist, p.replace(".png", "_dist_cpp_brute_force.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))

    print ("\n======= C++ distance overlay =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        dist = distance_overlay_cpp(img, bg_col)
        t1 = time.perf_counter()
        save_dist_image(dist, p.replace(".png", "_dist_cpp.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= NumPy distance overlay =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        dist = distance_overlay_numpy(img, bg_col)
        t1 = time.perf_counter()
        save_dist_image(dist, p.replace(".png", "_dist_numpy.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= Python distance overlay =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        dist = distance_overlay_py(img, bg_col)
        t1 = time.perf_counter()
        save_dist_image(dist, p.replace(".png", "_dist_py.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
