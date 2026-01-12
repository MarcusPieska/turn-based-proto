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
import numpy as np
from PIL import Image

#================================================================================================================================#
#=> - Setup C library -
#================================================================================================================================#

lib_path = os.path.join(os.path.dirname(__file__), "img_mng.so")
if not os.path.exists(lib_path):
    print("Error: %s not found. Please compile img_mng.cpp first." % lib_path)
    sys.exit(1)

lib = ctypes.CDLL(lib_path)

class pt(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.flood_fill.restype = None
lib.flood_fill.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_ubyte), 
    pt
]

#================================================================================================================================#
#=> - Python flood fill function -
#================================================================================================================================#

def flood_fill_py (img, lim_col, fill_col, start_pt):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(lim_col, (list, tuple)):
        lim_col = np.array(lim_col, dtype=np.uint8)
    if isinstance(fill_col, (list, tuple)):
        fill_col = np.array(fill_col, dtype=np.uint8)
    if lim_col.size != ch:
        lim_col = np.array([lim_col[0]], dtype=np.uint8) if ch == 1 else np.array(lim_col[:ch], dtype=np.uint8)
    if fill_col.size != ch:
        fill_col = np.array([fill_col[0]], dtype=np.uint8) if ch == 1 else np.array(fill_col[:ch], dtype=np.uint8)
    sx, sy = int(start_pt[0]), int(start_pt[1])
    if sx < 0 or sx >= w or sy < 0 or sy >= h:
        return img
    if np.array_equal(img[sy, sx], lim_col) or np.array_equal(img[sy, sx], fill_col):
        return img
    stack = [(sy, sx)]
    while stack:
        y, x = stack.pop()
        if x < 0 or x >= w or y < 0 or y >= h:
            continue
        if np.array_equal(img[y, x], lim_col) or np.array_equal(img[y, x], fill_col):
            continue
        img[y, x] = fill_col
        stack.append((y - 1, x))
        stack.append((y + 1, x))
        stack.append((y, x - 1))
        stack.append((y, x + 1))
    if ch == 1:
        img = img[:, :, 0]
    return img

def flood_fill_numpy (img, lim_col, fill_col, start_pt):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(lim_col, (list, tuple)):
        lim_col = np.array(lim_col, dtype=np.uint8)
    if isinstance(fill_col, (list, tuple)):
        fill_col = np.array(fill_col, dtype=np.uint8)
    if lim_col.size != ch:
        lim_col = np.array([lim_col[0]], dtype=np.uint8) if ch == 1 else np.array(lim_col[:ch], dtype=np.uint8)
    if fill_col.size != ch:
        fill_col = np.array([fill_col[0]], dtype=np.uint8) if ch == 1 else np.array(fill_col[:ch], dtype=np.uint8)
    sx, sy = int(start_pt[0]), int(start_pt[1])
    if sx < 0 or sx >= w or sy < 0 or sy >= h:
        return img
    if np.array_equal(img[sy, sx], lim_col) or np.array_equal(img[sy, sx], fill_col):
        return img
    stack = [(sy, sx)]
    lim_mask = np.all(img == lim_col, axis=2) if ch > 1 else (img[:, :, 0] == lim_col[0])
    fill_mask = np.all(img == fill_col, axis=2) if ch > 1 else (img[:, :, 0] == fill_col[0])
    while stack:
        y, x = stack.pop()
        if x < 0 or x >= w or y < 0 or y >= h:
            continue
        if lim_mask[y, x] or fill_mask[y, x]:
            continue
        img[y, x] = fill_col
        fill_mask[y, x] = True
        stack.append((y - 1, x))
        stack.append((y + 1, x))
        stack.append((y, x - 1))
        stack.append((y, x + 1))
    if ch == 1:
        img = img[:, :, 0]
    return img

#================================================================================================================================#
#=> - Helper function -
#================================================================================================================================#

def flood_fill_cpp (img, lim_col, fill_col, start_pt):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(lim_col, (list, tuple)):
        lim_col = np.array(lim_col, dtype=np.uint8)
    if isinstance(fill_col, (list, tuple)):
        fill_col = np.array(fill_col, dtype=np.uint8)
    if lim_col.size != ch:
        lim_col = np.array([lim_col[0]], dtype=np.uint8) if ch == 1 else np.array(lim_col[:ch], dtype=np.uint8)
    if fill_col.size != ch:
        fill_col = np.array([fill_col[0]], dtype=np.uint8) if ch == 1 else np.array(fill_col[:ch], dtype=np.uint8)
    sz = size(w, h)
    img_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lim_col_ptr = lim_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    fill_col_ptr = fill_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    start = pt(float(start_pt[0]), float(start_pt[1]))
    lib.flood_fill(img_ptr, sz, ch, lim_col_ptr, fill_col_ptr, start)
    if ch == 1:
        img = img[:, :, 0]
    return img

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_paths = []
    img_paths.append("../../../DEL_map_fill1.png")
    lim_col = np.array([0, 0, 0], dtype=np.uint8)
    fill_col = np.array([0, 0, 128], dtype=np.uint8)
    
    print ("\n======= C++ flood fill =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        out_img = flood_fill_cpp(img.copy(), lim_col, fill_col, (w / 2, h / 2))
        t1 = time.perf_counter()
        Image.fromarray(out_img).save(p.replace(".png", "_fill.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= NumPy flood fill =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        out_img = flood_fill_numpy(img.copy(), lim_col, fill_col, (w / 2, h / 2))
        t1 = time.perf_counter()
        Image.fromarray(out_img).save(p.replace(".png", "_fill.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= Python flood fill =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        out_img = flood_fill_py(img.copy(), lim_col, fill_col, (w / 2, h / 2))
        t1 = time.perf_counter()
        Image.fromarray(out_img).save(p.replace(".png", "_fill.png"))
        print("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
