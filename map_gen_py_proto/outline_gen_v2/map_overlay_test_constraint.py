#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import time
import ctypes
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

lib.distance_overlay.restype = None
lib.distance_overlay.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_int)
]

lib.distance_mask.restype = None
lib.distance_mask.argtypes = [
    ctypes.POINTER(ctypes.c_int), 
    size, 
    ctypes.c_int, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte)
]

#================================================================================================================================#
#=> - Python mask function -
#================================================================================================================================#

def distance_mask_py (dist, lower, upper):
    if not isinstance(dist, np.ndarray):
        dist = np.array(dist, dtype=np.int32)
    h, w = dist.shape
    mask = np.zeros((h, w), dtype=np.uint8)
    mask[(dist >= lower) & (dist <= upper)] = 255
    return mask

def distance_mask_numpy (dist, lower, upper):
    if not isinstance(dist, np.ndarray):
        dist = np.array(dist, dtype=np.int32)
    h, w = dist.shape
    mask = np.zeros((h, w), dtype=np.uint8)
    mask[(dist >= lower) & (dist <= upper)] = 255
    return mask

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

def distance_mask_cpp (dist, lower, upper):
    if not isinstance(dist, np.ndarray):
        dist = np.array(dist, dtype=np.int32)
    h, w = dist.shape
    sz = size(w, h)
    dist_ptr = dist.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
    mask_out = np.zeros((h, w), dtype=np.uint8)
    mask_ptr = mask_out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.distance_mask(dist_ptr, sz, lower, upper, mask_ptr)
    return mask_out

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_paths, dist_images = [], []
    img_paths.append("../../../DEL_map_dist.png")
    bg_col = np.array([255, 255, 255], dtype=np.uint8)
    constraints = []
    constraints.append((5, 15))
    constraints.append((10, 20))
    constraints.append((15, 25))
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        dist_images.append(distance_overlay_cpp(img, bg_col))
    
    print ("\n======= C++ distance mask =======")
    for idx, dist in enumerate(dist_images):
        p = img_paths[idx]
        for i, (lower, upper) in enumerate(constraints):
            t0 = time.perf_counter()
            mask = distance_mask_cpp(dist, lower, upper)
            t1 = time.perf_counter()
            mask_rgb = np.stack([mask, mask, mask], axis=2)
            mask_rgb[dist == 0] = [0, 0, 128]
            Image.fromarray(mask_rgb, mode='RGB').save(p.replace(".png", "_mask_%d_%d_cpp.png" % (lower, upper)))
            print("%s [%d-%d]: %.3f ms" % (p.split("/")[-1], lower, upper, (t1 - t0) * 1000))
    
    print ("\n======= NumPy distance mask =======")
    for idx, dist in enumerate(dist_images):
        p = img_paths[idx]
        for i, (lower, upper) in enumerate(constraints):
            t0 = time.perf_counter()
            mask = distance_mask_numpy(dist, lower, upper)
            t1 = time.perf_counter()
            mask_rgb = np.stack([mask, mask, mask], axis=2)
            mask_rgb[dist == 0] = [0, 0, 128]
            Image.fromarray(mask_rgb, mode='RGB').save(p.replace(".png", "_mask_%d_%d_numpy.png" % (lower, upper)))
            print("%s [%d-%d]: %.3f ms" % (p.split("/")[-1], lower, upper, (t1 - t0) * 1000))
    
    print ("\n======= Python distance mask =======")
    for idx, dist in enumerate(dist_images):
        p = img_paths[idx]
        for i, (lower, upper) in enumerate(constraints):
            t0 = time.perf_counter()
            mask = distance_mask_py(dist, lower, upper)
            t1 = time.perf_counter()
            mask_rgb = np.stack([mask, mask, mask], axis=2)
            mask_rgb[dist == 0] = [0, 0, 128]
            Image.fromarray(mask_rgb, mode='RGB').save(p.replace(".png", "_mask_%d_%d_py.png" % (lower, upper)))
            print("%s [%d-%d]: %.3f ms" % (p.split("/")[-1], lower, upper, (t1 - t0) * 1000))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
