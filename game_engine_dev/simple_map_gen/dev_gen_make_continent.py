#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
from collections import deque

import numpy as np
from PIL import Image

#================================================================================================================================#
#=> - PNG byte matrix -
#================================================================================================================================#

def load_png_matrix(path):
    img = Image.open(path).convert("L")
    return np.asarray(img, dtype=np.uint8)

def save_png_matrix(matrix_u8, path):
    os.makedirs(os.path.dirname(os.path.abspath(path)) or ".", exist_ok=True)
    Image.fromarray(matrix_u8, mode="L").save(path)

BYTE_SEA = 255
BYTE_LAND = 128

#================================================================================================================================#
#=> - Noise / sampling -
#================================================================================================================================#

def upscale_bilinear(coarse, out_h, out_w):
    gh, gw = coarse.shape
    ys = np.linspace(0.0, gh - 1.0, out_h, dtype=np.float64)
    xs = np.linspace(0.0, gw - 1.0, out_w, dtype=np.float64)
    yi = np.floor(ys).astype(np.int32).reshape(out_h, 1)
    xi = np.floor(xs).astype(np.int32).reshape(1, out_w)
    yi2 = np.clip(yi + 1, 0, gh - 1)
    xi2 = np.clip(xi + 1, 0, gw - 1)
    wy = (ys - yi.reshape(-1)).reshape(out_h, 1)
    wx = (xs - xi.reshape(-1)).reshape(1, out_w)
    z00 = coarse[yi, xi]
    z01 = coarse[yi, xi2]
    z10 = coarse[yi2, xi]
    z11 = coarse[yi2, xi2]
    return (1.0 - wy) * ((1.0 - wx) * z00 + wx * z01) + wy * ((1.0 - wx) * z10 + wx * z11)

def fbm_field(h, w, rng, octaves, base_period):
    acc = np.zeros((h, w), dtype=np.float64)
    tw = 0.0
    for i in range(max(1, octaves)):
        period = max(4, base_period >> i)
        gw = max(2, w // period + 1)
        gh = max(2, h // period + 1)
        coarse = rng.random((gh, gw))
        layer = upscale_bilinear(coarse, h, w)
        wi = 0.5 ** i
        acc += wi * layer
        tw += wi
    return acc / (tw + 1e-12)

def norm01(a):
    lo = float(np.min(a))
    hi = float(np.max(a))
    return (a - lo) / (hi - lo + 1e-12)

def radial_envelope(h, w, cx, cy, sx, sy, roundness):
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float64)
    d = np.sqrt(((xx - cx) / (sx + 1e-9)) ** 2 + ((yy - cy) / (sy + 1e-9)) ** 2)
    d = d / (float(np.max(d)) + 1e-12)
    return np.clip(1.0 - np.power(d, roundness), 0.0, 1.0)

def flood_component(mask, sx, sy):
    h, w = mask.shape
    out = np.zeros_like(mask, dtype=bool)
    if sx < 0 or sy < 0 or sx >= w or sy >= h or not mask[sy, sx]:
        return out
    q = deque([(sx, sy)])
    out[sy, sx] = True
    while q:
        x, y = q.popleft()
        if x + 1 < w and mask[y, x + 1] and not out[y, x + 1]:
            out[y, x + 1] = True
            q.append((x + 1, y))
        if x - 1 >= 0 and mask[y, x - 1] and not out[y, x - 1]:
            out[y, x - 1] = True
            q.append((x - 1, y))
        if y + 1 < h and mask[y + 1, x] and not out[y + 1, x]:
            out[y + 1, x] = True
            q.append((x, y + 1))
        if y - 1 >= 0 and mask[y - 1, x] and not out[y - 1, x]:
            out[y - 1, x] = True
            q.append((x, y - 1))
    return out

def mask_touches_border(m):
    if m.shape[0] < 2 or m.shape[1] < 2:
        return True
    return bool(np.any(m[0]) or np.any(m[-1]) or np.any(m[:, 0]) or np.any(m[:, -1]))

def build_combined_centered(params):
    rng = np.random.default_rng(np.random.SeedSequence([params.seed]))
    h, w = params.height, params.width
    cx = 0.5 * (w - 1)
    cy = 0.5 * (h - 1)
    sx = 0.5 * w
    sy = 0.5 * h
    env = radial_envelope(h, w, cx, cy, sx, sy, params.roundness)
    nz = fbm_field(h, w, rng, params.noise_octaves, params.noise_base_period)
    env = norm01(env)
    nz = norm01(nz)
    rw = params.radial_weight
    nw = params.noise_weight
    tw = rw + nw
    rw, nw = rw / tw, nw / tw
    combined = rw * env + nw * nz
    return norm01(combined)

def largest_centered_continent_no_border(combined):
    h, w = combined.shape
    ax = int(round(0.5 * (w - 1)))
    ay = int(round(0.5 * (h - 1)))
    lo = 0.0
    hi = 1.0
    best = np.zeros_like(combined, dtype=bool)
    for _ in range(56):
        mid = 0.5 * (lo + hi)
        mask = combined >= mid
        if not mask[ay, ax]:
            hi = mid
            continue
        comp = flood_component(mask, ax, ay)
        if not comp.any():
            hi = mid
            continue
        if mask_touches_border(comp):
            lo = mid
        else:
            best = comp
            hi = mid
    return best

def make_continent(params):
    combined = build_combined_centered(params)
    continent = largest_centered_continent_no_border(combined)
    if not continent.any():
        return np.full((params.height, params.width), BYTE_SEA, dtype=np.uint8)
    return np.where(continent, BYTE_LAND, BYTE_SEA).astype(np.uint8)

#================================================================================================================================#
#=> - Params -
#================================================================================================================================#

class ContinentParams(object):
    __slots__ = (
        "seed",
        "width",
        "height",
        "roundness",
        "noise_octaves",
        "noise_base_period",
        "radial_weight",
        "noise_weight",
    )

    def __init__(m):
        m.seed = 1
        m.width = 512
        m.height = 512
        m.roundness = 2.35
        m.noise_octaves = 4
        m.noise_base_period = 56
        m.radial_weight = 0.82
        m.noise_weight = 0.18

#================================================================================================================================#
#=> - Test helpers -
#================================================================================================================================#

def make_and_save_continent_png(params, out_png):
    mat = make_continent(params)
    save_png_matrix(mat, out_png)
    frac = float(np.mean(mat == BYTE_LAND))
    ok = int(frac > 0.0)
    print("wrote %s land_frac=%.4f kept=%d seed=%d size=%dx%d" % (out_png, frac, ok, params.seed, params.width, params.height))
    return frac

def params_round_medium():
    params = ContinentParams()
    params.seed = 41
    params.width = 1000
    params.height = 1000
    params.roundness = 2.45
    params.noise_octaves = 4
    params.noise_base_period = 64
    params.radial_weight = 0.84
    params.noise_weight = 0.16
    return params

def params_coarser_outline():
    params = ContinentParams()
    params.seed = 109
    params.width = 1000
    params.height = 1000
    params.roundness = 2.1
    params.noise_octaves = 5
    params.noise_base_period = 42
    params.radial_weight = 0.78
    params.noise_weight = 0.22
    return params

def run_smoke_outputs():
    here = os.path.dirname(os.path.abspath(__file__))
    make_and_save_continent_png(params_round_medium(), os.path.join(here, "out_continent_round.png"))
    make_and_save_continent_png(params_coarser_outline(), os.path.join(here, "out_continent_coarse.png"))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    run_smoke_outputs()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
