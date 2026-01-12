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

class size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_int), ("height", ctypes.c_int)]

lib.shift_image.restype = None
lib.shift_image.argtypes = [
    ctypes.POINTER(ctypes.c_ubyte), 
    ctypes.POINTER(ctypes.c_ubyte), 
    size, 
    ctypes.c_int, 
    ctypes.c_int, 
    ctypes.c_int, 
    ctypes.POINTER(ctypes.c_ubyte)
]

#================================================================================================================================#
#=> - Python shift function -
#================================================================================================================================#

def shift_image_cpp (img, dx, dy, def_col):
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
    in_ptr = img.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    out_ptr = out.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    col_ptr = def_col.ctypes.data_as(ctypes.POINTER(ctypes.c_ubyte))
    lib.shift_image(in_ptr, out_ptr, sz, ch, dx, dy, col_ptr)
    if ch == 1:
        out = out[:, :, 0]
    return out

def shift_image_py (img, dx, dy, def_col):
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
    out = np.tile(def_col[np.newaxis, np.newaxis, :], (h, w, 1))
    src_y0 = max(0, -dy)
    src_y1 = min(h, h - dy)
    src_x0 = max(0, -dx)
    src_x1 = min(w, w - dx)
    dst_y0 = max(0, dy)
    dst_y1 = min(h, h + dy)
    dst_x0 = max(0, dx)
    dst_x1 = min(w, w + dx)
    if src_y1 > src_y0 and src_x1 > src_x0:
        out[dst_y0:dst_y1, dst_x0:dst_x1] = img[src_y0:src_y1, src_x0:src_x1]
    if ch == 1:
        out = out[:, :, 0]
    return out

def shift_image_numpy (img, dx, dy, def_col):
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
    out = np.tile(def_col[np.newaxis, np.newaxis, :], (h, w, 1))
    src_y0 = max(0, -dy)
    src_y1 = min(h, h - dy)
    src_x0 = max(0, -dx)
    src_x1 = min(w, w - dx)
    dst_y0 = max(0, dy)
    dst_y1 = min(h, h + dy)
    dst_x0 = max(0, dx)
    dst_x1 = min(w, w + dx)
    if src_y1 > src_y0 and src_x1 > src_x0:
        out[dst_y0:dst_y1, dst_x0:dst_x1] = img[src_y0:src_y1, src_x0:src_x1]
    if ch == 1:
        out = out[:, :, 0]
    return out

#================================================================================================================================#
#=> - Main test -
#================================================================================================================================#

if __name__ == "__main__":
    img_path = "../../../DEL_map_margin4.png"
    def_col = np.array([128, 128, 128], dtype=np.uint8)
    shifts = []
    shifts.append((-281, 0))
    shifts.append((3282, 0))
    shifts.append((0, -3474))
    shifts.append((0, 218))

    print ("\n======= C++ shift =======")
    for i, margin in enumerate(shifts):
        img = np.array(Image.open(img_path).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        out_img = shift_image_cpp(img, margin[0], margin[1], def_col)
        t1 = time.perf_counter()
        Image.fromarray(out_img).save(img_path.replace(".png", "_shift%d_cpp.png" % i))
        print("%s: %.3f ms" % (img_path.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= NumPy shift =======")
    for i, margin in enumerate(shifts):
        img = np.array(Image.open(img_path).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        out_img = shift_image_numpy(img, margin[0], margin[1], def_col)
        t1 = time.perf_counter()
        Image.fromarray(out_img).save(img_path.replace(".png", "_shift%d_numpy.png" % i))
        print("%s: %.3f ms" % (img_path.split("/")[-1], (t1 - t0) * 1000))
    
    print ("\n======= Python shift =======")
    for i, margin in enumerate(shifts):
        img = np.array(Image.open(img_path).convert("RGB"), dtype=np.uint8)
        h, w = img.shape[:2]
        t0 = time.perf_counter()
        out_img = shift_image_py(img, margin[0], margin[1], def_col)
        t1 = time.perf_counter()
        Image.fromarray(out_img).save(img_path.replace(".png", "_shift%d_py.png" % i))
        print("%s: %.3f ms" % (img_path.split("/")[-1], (t1 - t0) * 1000))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
