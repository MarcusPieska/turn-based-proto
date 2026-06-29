#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

MAP_W = 1000
MAP_H = 1000

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
TERR_PATH = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm"
TRACE_PATH = os.path.join(THIS_DIR, "explore_distant_test.trace")
OUT_DIR = "/home/w/Projects/simple-map-gen/explore-distant-test"

UNEXPLORED = (24, 24, 32)
PLAYER_N = 2

#================================================================================================================================#
#=> - PPM -
#================================================================================================================================#

def ppm_read_token (f):
    tok = b""
    while True:
        c = f.read(1)
        if not c:
            break
        if c in b" \t\r\n":
            if tok:
                break
            if c == b"#":
                while c not in (b"\n", b""):
                    c = f.read(1)
            continue
        if c == b"#":
            while c not in (b"\n", b""):
                c = f.read(1)
            continue
        tok += c
    if not tok:
        return None
    return tok.decode("ascii")

def load_ppm_rgb (path):
    with open(path, "rb") as f:
        if f.read(2) != b"P6":
            raise ValueError("not P6 ppm: " + path)
        w = int(ppm_read_token(f))
        h = int(ppm_read_token(f))
        maxv = int(ppm_read_token(f))
        if maxv != 255:
            raise ValueError("expected maxval 255: " + path)
        raw = f.read(w * h * 3)
        if len(raw) != w * h * 3:
            raise ValueError("short ppm body: " + path)
    return w, h, raw

def write_ppm_rgb (path, w, h, rgb):
    with open(path, "wb") as f:
        f.write(b"P6\n")
        f.write(("%d %d\n" % (w, h)).encode("ascii"))
        f.write(b"255\n")
        f.write(rgb)

#================================================================================================================================#
#=> - Trace -
#================================================================================================================================#

def parse_trace (path):
    explored = {0: set(), 1: set()}
    snapshots = []
    with open(path, "r", encoding="utf-8") as ptr:
        for line in ptr:
            line = line.strip()
            if line == "":
                continue
            if line.startswith("EXPLORE_DISCOVER:"):
                parts = line.split(":")
                if len(parts) != 4:
                    continue
                x = int(parts[1])
                y = int(parts[2])
                p = int(parts[3])
                explored[p].add((x, y))
            elif line.startswith("NEW_TURN:"):
                snapshots.append({0: set(explored[0]), 1: set(explored[1])})
    snapshots.append({0: set(explored[0]), 1: set(explored[1])})
    if len(snapshots) == 0:
        snapshots.append({0: set(), 1: set()})
    return snapshots

#================================================================================================================================#
#=> - Compose -
#================================================================================================================================#

def compose_player_frame (base_rgb, explored):
    out = bytearray(len(base_rgb))
    for y in range(MAP_H):
        for x in range(MAP_W):
            i = (y * MAP_W + x) * 3
            if (x, y) not in explored:
                out[i] = UNEXPLORED[0]
                out[i + 1] = UNEXPLORED[1]
                out[i + 2] = UNEXPLORED[2]
                continue
            out[i] = base_rgb[i]
            out[i + 1] = base_rgb[i + 1]
            out[i + 2] = base_rgb[i + 2]
    return bytes(out)

#================================================================================================================================#
#=> - Export -
#================================================================================================================================#

def export_turn_images (terr_rgb, snapshots, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    turn_n = len(snapshots)
    file_n = 0
    for player in range(PLAYER_N):
        for turn in range(turn_n):
            frame = compose_player_frame(terr_rgb, snapshots[turn][player])
            name = "%03d_%04d_explore.ppm" % (player, turn)
            path = os.path.join(out_dir, name)
            write_ppm_rgb(path, MAP_W, MAP_H, frame)
            file_n += 1
    return file_n

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    os.chdir(THIS_DIR)
    if not os.path.isfile(TRACE_PATH):
        print("WARN: missing", TRACE_PATH)
        sys.exit(1)
    if not os.path.isfile(TERR_PATH):
        print("WARN: missing", TERR_PATH)
        sys.exit(1)
    w, h, terr_rgb = load_ppm_rgb(TERR_PATH)
    if w != MAP_W or h != MAP_H:
        print("WARN: terrain size %dx%d" % (w, h))
        sys.exit(1)
    snapshots = parse_trace(TRACE_PATH)
    n = export_turn_images(terr_rgb, snapshots, OUT_DIR)
    print("OK: wrote %d images (%d players) to %s" % (n, PLAYER_N, OUT_DIR))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
