#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir (os.path.dirname (os.path.abspath (__file__)))
sys.path.insert (0, os.getcwd ())

import math
import random
from PIL import Image, ImageDraw

from tool_forrest_pattern_gen import PineTreeMaker, BroadleafTreeMaker

#================================================================================================================================#
#=> - Diamond tile forest packer -
#================================================================================================================================#

DIA_W = 100
DIA_H = 50
PAD_TOP = 28
PAD_SIDE = 24
PAD_BOT = 10
OUT_DIR = "/home/w/Projects/img-content/forest_tile"
N_VAR = 5
PINE_N = 70
BROAD_N = 36

def canv_size ():
    return DIA_W + 2 * PAD_SIDE, DIA_H + PAD_TOP + PAD_BOT

def dia_ctr ():
    cw, ch = canv_size ()
    return cw // 2, PAD_TOP + DIA_H // 2

def in_dia (x, y, cx, cy):
    hw = DIA_W // 2
    hh = DIA_H // 2
    return abs (x - cx) * hh + abs (y - cy) * hw <= hw * hh

def sample_foot (cx, cy, rng):
    hw = DIA_W // 2
    hh = DIA_H // 2
    for _ in range (8000):
        x = rng.randint (cx - hw, cx + hw)
        y = rng.randint (cy - hh, cy + hh)
        if in_dia (x, y, cx, cy):
            return x, y
    return cx, cy

def blend_px (dst, x, y, r, g, b, a):
    if a <= 0:
        return
    if x < 0 or y < 0 or x >= dst.width or y >= dst.height:
        return
    er, eg, eb, ea = dst.getpixel ((x, y))
    if ea == 0:
        dst.putpixel ((x, y), (r, g, b, a))
        return
    sa = a / 255.0
    da = ea / 255.0
    oa = sa + da * (1.0 - sa)
    if oa <= 0:
        return
    nr = int ((r * sa + er * da * (1.0 - sa)) / oa)
    ng = int ((g * sa + eg * da * (1.0 - sa)) / oa)
    nb = int ((b * sa + eb * da * (1.0 - sa)) / oa)
    dst.putpixel ((x, y), (nr, ng, nb, int (oa * 255)))

def paste_rgba (dst, src, x0, y0):
    sp = src.load ()
    for y in range (src.height):
        for x in range (src.width):
            r, g, b, a = sp[x, y]
            blend_px (dst, x0 + x, y0 + y, r, g, b, a)

def paste_shadow (dst, shadow, x0, y0, mask):
    sp = shadow.load ()
    for y in range (shadow.height):
        for x in range (shadow.width):
            r, g, b, a = sp[x, y]
            if a <= 0:
                continue
            px, py = x0 + x, y0 + y
            if px < 0 or py < 0 or px >= dst.width or py >= dst.height:
                continue
            key = (px, py)
            if a <= mask.get (key, 0):
                continue
            blend_px (dst, px, py, r, g, b, a)
            mask[key] = a

def pack_tile (maker, tree_n, seed):
    rng = random.Random (seed)
    random.seed (seed)
    cw, ch = canv_size ()
    cx, cy = dia_ctr ()
    out = Image.new ("RGBA", (cw, ch), (0, 0, 0, 0))
    feet = [sample_foot (cx, cy, rng) for _ in range (tree_n)]
    feet.sort (key=lambda p: p[1])
    trees = []
    shadows = []
    for x, y in feet:
        color = maker.getRandomTreeColor ()
        decal = maker.generateTreeDecal (color)
        shadow = maker.generateTreeShadow (decal.height)
        trees.append ((decal, x, y))
        if shadow is not None:
            shadows.append ((shadow, x + 4, y))
    mask = {}
    for shadow, x, y in shadows:
        paste_shadow (out, shadow, x - shadow.width // 2, y - shadow.height // 2, mask)
    for decal, x, y in trees:
        paste_rgba (out, decal, x - decal.width // 2, y - decal.height + 1)
    return out

def draw_guide (img):
    cx, cy = dia_ctr ()
    d = ImageDraw.Draw (img)
    pts = [
        (cx, cy - DIA_H // 2),
        (cx + DIA_W // 2, cy),
        (cx, cy + DIA_H // 2),
        (cx - DIA_W // 2, cy),
    ]
    d.polygon (pts, outline=(255, 0, 255, 180))

def save_set (tag, maker, tree_n):
    os.makedirs (OUT_DIR, exist_ok=True)
    paths = []
    for i in range (1, N_VAR + 1):
        path = os.path.join (OUT_DIR, "%s_%02d.png" % (tag, i))
        if os.path.exists (path):
            raise RuntimeError ("refusing to overwrite: %s" % path)
        img = pack_tile (maker, tree_n, seed=1000 * (1 if tag == "pine" else 2) + i)
        img.save (path)
        paths.append (path)
        print ("wrote %s  %dx%d" % (path, img.width, img.height))
    guide = pack_tile (maker, tree_n, seed=9000 + (1 if tag == "pine" else 2))
    draw_guide (guide)
    gpath = os.path.join (OUT_DIR, "%s_guide.png" % tag)
    if os.path.exists (gpath):
        raise RuntimeError ("refusing to overwrite: %s" % gpath)
    guide.save (gpath)
    print ("wrote %s (diamond guide)" % gpath)
    return paths

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    protected = [
        "/home/w/Projects/img-content/texture-grassland3-pine%02d.png" % i for i in range (1, 6)
    ] + [
        "/home/w/Projects/img-content/texture-grassland3-broadleaf%02d.png" % i for i in range (1, 6)
    ]
    for p in protected:
        if not os.path.isfile (p):
            print ("warning: expected source sprite missing: %s" % p)
    if not os.path.isdir (OUT_DIR):
        os.makedirs (OUT_DIR, exist_ok=True)
    existing = [f for f in os.listdir (OUT_DIR) if f.endswith (".png")] if os.path.isdir (OUT_DIR) else []
    if existing:
        print ("*** forest_tile already has files; remove them first or pick a fresh folder")
        print ("    %s" % OUT_DIR)
        for f in sorted (existing)[:8]:
            print ("    %s" % f)
        sys.exit (1)
    save_set ("pine", PineTreeMaker (), PINE_N)
    save_set ("broadleaf", BroadleafTreeMaker (), BROAD_N)
    print ("done  out=%s  (original grassland3 sprites untouched)" % OUT_DIR)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
