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

lib_path = os.path.join(os.path.dirname(__file__), "img_mng.so")
if not os.path.exists(lib_path):
    print("Error: %s not found. Please compile img_mng.cpp first." % lib_path)
    sys.exit(1)

lib = ctypes.CDLL(lib_path)

class pt(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

class margins(ctypes.Structure):
    _fields_ = [("left", ctypes.c_int), ("right", ctypes.c_int), ("top", ctypes.c_int), ("bottom", ctypes.c_int)]

lib.find_margins.restype = None
lib.find_margins.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(margins)
]

#================================================================================================================================#
#=> - Python margin finder function -
#================================================================================================================================#

def find_margins_py (img, bg_col):
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
    left, right, top, bottom = w, -1, h, -1
    for y in range(h):
        for x in range(w):
            if not np.array_equal(img[y, x], bg_col):
                if x < left: left = x
                if x > right: right = x
                if y < top: top = y
                if y > bottom: bottom = y
    right = w - 1 - right
    bottom = h - 1 - bottom
    return [left, right, top, bottom]

def find_margins_numpy (img, bg_col):
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
    non_bg = ~bg_mask
    if not np.any(non_bg):
        return [w, -1, h, -1]
    y_coords, x_coords = np.where(non_bg)
    left = int(np.min(x_coords))
    right = int(np.max(x_coords))
    top = int(np.min(y_coords))
    bottom = int(np.max(y_coords))
    right = w - 1 - right
    bottom = h - 1 - bottom
    return [left, right, top, bottom]

#================================================================================================================================#
#=> - Helper function -
#================================================================================================================================#

def find_margins_cpp (img, bg_col):
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
    m = margins()
    lib.find_margins(img_ptr, sz, ch, bg_col_ptr, ctypes.byref(m))
    return [m.left, m.right, m.top, m.bottom]

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_paths = []
    img_paths.append("../../../DEL_map_margin1.png")
    img_paths.append("../../../DEL_map_margin2.png")
    img_paths.append("../../../DEL_map_margin3.png")
    img_paths.append("../../../DEL_map_margin4.png")
    bg_col = np.array([255, 255, 255], dtype=np.uint8)
    
    print ("\n======= C++ find margins =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        m = find_margins_cpp(img, bg_col)
        t1 = time.perf_counter()
        print("%s: %.3f ms - left:%d right:%d top:%d bottom:%d" % (p.split("/")[-1], (t1 - t0) * 1000, m[0], m[1], m[2], m[3]))
    
    print ("\n======= NumPy find margins =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        m = find_margins_numpy(img, bg_col)
        t1 = time.perf_counter()
        print("%s: %.3f ms - left:%d right:%d top:%d bottom:%d" % (p.split("/")[-1], (t1 - t0) * 1000, m[0], m[1], m[2], m[3]))
    
    print ("\n======= Python find margins =======")
    for p in img_paths:
        img = np.array(Image.open(p).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        m = find_margins_py(img, bg_col)
        t1 = time.perf_counter()
        print("%s: %.3f ms - left:%d right:%d top:%d bottom:%d" % (p.split("/")[-1], (t1 - t0) * 1000, m[0], m[1], m[2], m[3]))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
