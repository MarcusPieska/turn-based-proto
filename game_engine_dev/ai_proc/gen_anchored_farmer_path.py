#!/usr/bin/env python3

import math
import os

#================================================================================================================================
#=> - Constants -
#================================================================================================================================

# First k_brd_cnt[4] entries from city/circular_tile_areas.cpp
CELLS = [
    (0, 0), (-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1),
    (-1, -2), (0, -2), (1, -2), (-2, -1), (2, -1), (-2, 0), (2, 0), (-2, 1), (2, 1),
    (-1, 2), (0, 2), (1, 2), (-1, -3), (0, -3), (1, -3), (-2, -2), (2, -2),
    (-3, -1), (3, -1), (-3, 0), (3, 0), (-3, 1), (3, 1), (-2, 2), (2, 2),
    (-1, 3), (0, 3), (1, 3),
]

DIRS = [(0, -1), (1, -1), (1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1)]
R = 3
SIDE = 2 * R + 1
N = SIDE * SIDE

#================================================================================================================================
#=> - Functions -
#================================================================================================================================

def cheb(p):
    return max(abs(p[0]), abs(p[1]))

def spiral_key(p):
    return (cheb(p), math.atan2(p[0], -p[1]))

def flat(dx, dy):
    return (dx + R) + (dy + R) * SIDE

def solve(cells):
    s = set(cells)

    def neighbors(p):
        x, y = p
        return [(x + dx, y + dy) for dx, dy in DIRS if (x + dx, y + dy) in s]

    path = [(0, 0)]
    used = {(0, 0)}

    def dfs():
        if len(path) == len(cells):
            return True
        cand = [q for q in neighbors(path[-1]) if q not in used]
        cand.sort(key=spiral_key)
        for q in cand:
            used.add(q)
            path.append(q)
            if dfs():
                return True
            path.pop()
            used.remove(q)
        return False

    if not dfs():
        raise SystemExit("no Hamiltonian path")
    if set(path) != s:
        raise SystemExit("path miss")
    for i in range(len(path) - 1):
        a, b = path[i], path[i + 1]
        if max(abs(a[0] - b[0]), abs(a[1] - b[1])) != 1:
            raise SystemExit("non-adjacent step")
    return path

def emit_row(vals):
    return "    " + ", ".join(str(v) for v in vals) + ","

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    path = solve(CELLS)
    sx = [0] * N
    sy = [0] * N
    for i, (dx, dy) in enumerate(path):
        ndx, ndy = path[(i + 1) % len(path)]
        j = flat(dx, dy)
        sx[j] = ndx - dx
        sy[j] = ndy - dy
        if sx[j] == 0 and sy[j] == 0:
            raise SystemExit("zero step on path")
    out = os.path.join(os.path.dirname(__file__), "anchored_farmer_path.inc")
    lines = [
        f"static const i8 k_map_r = {R};",
        f"static const u16 k_map_side = {SIDE};",
        f"static const u16 k_map_n = {N};",
        "static const i8 k_step_x[] = {",
        emit_row(sx),
        "};",
        "static const i8 k_step_y[] = {",
        emit_row(sy),
        "};",
        "",
    ]
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"wrote {out} path_n={len(path)} map_n={N}")

if __name__ == "__main__":
    main()

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
