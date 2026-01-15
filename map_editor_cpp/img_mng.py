#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())

import math
import numpy as np

#================================================================================================================================#
#=> - NumPy functions -
#================================================================================================================================#

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
#=> - End -
#================================================================================================================================#
