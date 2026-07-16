#!/usr/bin/env python3

import sys

sys.dont_write_bytecode = True

#=================================================================================================================================
#=> - Config -
#=================================================================================================================================

RANGE_HI = 20

#=================================================================================================================================
#=> - Offset calc -
#=================================================================================================================================

def min_range (dx, dy):
    d2 = dx * dx + dy * dy
    if d2 == 0:
        return 0
    r = 1
    while r * r <= d2:
        r += 1
    return r

def build_table ():
    out = []
    for dy in range(-RANGE_HI, RANGE_HI + 1):
        for dx in range(-RANGE_HI, RANGE_HI + 1):
            r = min_range(dx, dy)
            if r <= RANGE_HI:
                out.append((r, dx, dy))
    out.sort(key=lambda t: (t[0], t[2], t[1]))
    return out

def cumul_counts (rows):
    cnt = [0] * (RANGE_HI + 1)
    for r, dx, dy in rows:
        cnt[r] += 1
    for i in range(1, RANGE_HI + 1):
        cnt[i] += cnt[i - 1]
    return cnt

def emit_brd (rows):
    body = ",".join("{%d,%d}" % (dx, dy) for r, dx, dy in rows)
    return "static const i8 k_brd[][2] = {%s};" % body

def emit_cnt (cnt):
    body = ",".join(str(n) for n in cnt)
    return "static const u16 k_brd_cnt[] = {%s};" % body

#=================================================================================================================================
#=> - main -
#=================================================================================================================================

def main ():
    rows = build_table()
    cnt = cumul_counts(rows)
    print("// City border offsets: {dx, dy}, tile in range N iff dist < N.")
    print("// Sorted by min_r, then dy, then dx. k_brd_cnt[N] = #entries with min_r <= N.")
    print("// Offset count = %d." % len(rows))
    print()
    print(emit_brd(rows))
    print(emit_cnt(cnt))

if __name__ == "__main__":
    main()

#=================================================================================================================================
#=> - End of file -
#=================================================================================================================================
