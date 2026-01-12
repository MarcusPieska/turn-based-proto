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

lib.rotate_image.restype = None
lib.rotate_image.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    pt, 
    ctypes.c_double
]

#================================================================================================================================#
#=> - Python rotation function -
#================================================================================================================================#

def rotate_image_py (img, def_col, cx, cy, rot_deg):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(def_col, (list, tuple)):
        def_col = np.array(def_col, dtype=np.uint8)
    if def_col.size != ch:
        def_col = np.array([def_col[0]], dtype=np.uint8) if ch == 1 else np.array(def_col[:ch], dtype=np.uint8)
    out = np.zeros_like(img)
    angle_rad = math.radians(rot_deg)
    cos_a = math.cos(angle_rad)
    sin_a = math.sin(angle_rad)
    for y in range(h):
        for x in range(w):
            dx = x - cx
            dy = y - cy
            src_x = cx + dx * cos_a + dy * sin_a
            src_y = cy - dx * sin_a + dy * cos_a
            x0 = int(math.floor(src_x))
            y0 = int(math.floor(src_y))
            x1 = x0 + 1
            y1 = y0 + 1
            if x0 < 0 or y0 < 0 or x1 >= w or y1 >= h:
                out[y, x] = def_col
            else:
                fx = src_x - x0
                fy = src_y - y0
                for c in range(ch):
                    v00 = img[y0, x0, c]
                    v10 = img[y0, x1, c]
                    v01 = img[y1, x0, c]
                    v11 = img[y1, x1, c]
                    v0 = v00 * (1.0 - fx) + v10 * fx
                    v1 = v01 * (1.0 - fx) + v11 * fx
                    out[y, x, c] = int(v0 * (1.0 - fy) + v1 * fy)
    if ch == 1:
        out = out[:, :, 0]
    return out

def rotate_image_numpy (img, def_col, cx, cy, rot_deg):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(def_col, (list, tuple)):
        def_col = np.array(def_col, dtype=np.uint8)
    if def_col.size != ch:
        def_col = np.array([def_col[0]], dtype=np.uint8) if ch == 1 else np.array(def_col[:ch], dtype=np.uint8)
    angle_rad = math.radians(rot_deg)
    cos_a = math.cos(angle_rad)
    sin_a = math.sin(angle_rad)
    y_coords, x_coords = np.meshgrid(np.arange(h), np.arange(w), indexing='ij')
    dx = x_coords - cx
    dy = y_coords - cy
    src_x = cx + dx * cos_a + dy * sin_a
    src_y = cy - dx * sin_a + dy * cos_a
    x0 = np.floor(src_x).astype(np.int32)
    y0 = np.floor(src_y).astype(np.int32)
    x1 = x0 + 1
    y1 = y0 + 1
    fx = src_x - x0
    fy = src_y - y0
    valid = (x0 >= 0) & (y0 >= 0) & (x1 < w) & (y1 < h)
    out = np.tile(def_col[np.newaxis, np.newaxis, :], (h, w, 1))
    if np.any(valid):
        x0v = x0[valid]
        y0v = y0[valid]
        x1v = x1[valid]
        y1v = y1[valid]
        fxv = fx[valid]
        fyv = fy[valid]
        v00 = img[y0v, x0v, :].astype(np.float32)
        v10 = img[y0v, x1v, :].astype(np.float32)
        v01 = img[y1v, x0v, :].astype(np.float32)
        v11 = img[y1v, x1v, :].astype(np.float32)
        v0 = v00 * (1.0 - fxv[:, np.newaxis]) + v10 * fxv[:, np.newaxis]
        v1 = v01 * (1.0 - fxv[:, np.newaxis]) + v11 * fxv[:, np.newaxis]
        out[valid, :] = (v0 * (1.0 - fyv[:, np.newaxis]) + v1 * fyv[:, np.newaxis]).astype(np.uint8)
    if ch == 1:
        out = out[:, :, 0]
    return out

#================================================================================================================================#
#=> - Helper function -
#================================================================================================================================#

def rotate_image_cpp (img, def_col, cx, cy, rot_deg):
    if not isinstance(img, np.ndarray):
        img = np.array(img, dtype=np.uint8)
    if img.ndim == 2:
        h, w = img.shape
        ch = 1
        img = img[:, :, np.newaxis]
    else:
        h, w, ch = img.shape
    if isinstance(def_col, (list, tuple)):
        def_col = np.array(def_col, dtype=np.uint8)
    if def_col.size != ch:
        def_col = np.array([def_col[0]], dtype=np.uint8) if ch == 1 else np.array(def_col[:ch], dtype=np.uint8)
    out = np.zeros_like(img)
    sz = size(w, h)
    cntr = pt(float(cx), float(cy))
    in_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    out_ptr = out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    col_ptr = def_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.rotate_image(in_ptr, out_ptr, col_ptr, sz, ch, cntr, rot_deg)
    if ch == 1:
        out = out[:, :, 0]
    return out

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_paths = []
    img_paths.append("../../../DEL_map_800x600.png")
    img_paths.append("../../../DEL_map_1200x600.png")
    img_paths.append("../../../DEL_map_2000x1200.png")
    def_col = np.array ([128, 128, 128], dtype=np.uint8)
    rot_deg = 45.0
    
    print ("\n======= C++ rotation =======")
    for p in img_paths:
        img = np.array (Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter ()
        out_img = rotate_image_cpp (img, def_col, w / 2.0, h / 2.0, rot_deg)
        t1 = time.perf_counter ()
        Image.fromarray(out_img).save (p.replace(".png", "_rot.png"))
        print ("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= NumPy rotation =======")
    for p in img_paths:
        img = np.array (Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter ()
        out_img = rotate_image_numpy (img, def_col, w / 2.0, h / 2.0, rot_deg)
        t1 = time.perf_counter ()
        Image.fromarray(out_img).save (p.replace(".png", "_rot.png"))
        print ("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= Python rotation =======")
    for p in img_paths:
        img = np.array (Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter ()
        out_img = rotate_image_py (img, def_col, w / 2.0, h / 2.0, rot_deg)
        t1 = time.perf_counter ()
        Image.fromarray(out_img).save (p.replace(".png", "_rot.png"))
        print ("%s: %.3f ms" % (p.split("/")[-1], (t1 - t0) * 1000))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
