#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import math
from collections import deque

from dev_small_shape_perlin import fbm_field

#================================================================================================================================#
#=> - Blob mask -
#================================================================================================================================#

def _norm_dist(gx, gy, cx, cy, lim_x, lim_y):
    dx = (gx - cx) / max(float(lim_x), 1e-9)
    dy = (gy - cy) / max(float(lim_y), 1e-9)
    return math.sqrt(dx * dx + dy * dy)

def _radial_grad(nd, power):
    if nd >= 1.0:
        return 0.0
    t = 1.0 - nd
    return t ** power

def _flood_mask(mask, sx, sy):
    h = len(mask)
    w = len(mask[0]) if h > 0 else 0
    if sx < 0 or sy < 0 or sx >= w or sy >= h or not mask[sy][sx]:
        return mask
    out = [row[:] for row in mask]
    q = deque([(sx, sy)])
    out[sy][sx] = True
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx = x + dx
            ny = y + dy
            if nx < 0 or ny < 0 or nx >= w or ny >= h:
                continue
            if out[ny][nx]:
                continue
            out[ny][nx] = True
            q.append((nx, ny))
    return out

def _fill_core(mask, cx, cy, lim_x, lim_y, core_nd):
    h = len(mask)
    w = len(mask[0]) if h > 0 else 0
    out = [row[:] for row in mask]
    q = deque()
    if 0 <= cx < w and 0 <= cy < h:
        out[cy][cx] = True
        q.append((cx, cy))
    while q:
        x, y = q.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx = x + dx
            ny = y + dy
            if nx < 0 or ny < 0 or nx >= w or ny >= h:
                continue
            if out[ny][nx]:
                continue
            nd = _norm_dist(nx, ny, cx, cy, lim_x, lim_y)
            if nd > 1.0:
                continue
            if nd <= core_nd or mask[ny][nx]:
                out[ny][nx] = True
                q.append((nx, ny))
    return out

def blob_mask(cells_w, cells_h, cx, cy, lim_x, lim_y, seed):
    noise = fbm_field(
        cells_w, cells_h, seed,
        frequency=2.5, lacunarity=2.0, octaves=4, persistence=0.4,
    )
    raw = [[False] * cells_w for _ in range(cells_h)]
    for gy in range(cells_h):
        for gx in range(cells_w):
            nd = _norm_dist(gx, gy, cx, cy, lim_x, lim_y)
            if nd > 1.05:
                continue
            radial = _radial_grad(nd, 1.6)
            n = noise[gy][gx]
            score = radial * (0.5 + 0.5 * n)
            if score >= 0.36:
                raw[gy][gx] = True
    conn = _flood_mask(raw, cx, cy)
    return _fill_core(conn, cx, cy, lim_x, lim_y, 0.72)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
