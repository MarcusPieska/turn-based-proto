#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
os.chdir (os.path.dirname (os.path.abspath (__file__)))
sys.path.insert (0, os.getcwd ())

import random
from PIL import Image, ImageDraw

from tool_forrest_pattern_gen import PineTreeMaker, BroadleafTreeMaker

#================================================================================================================================#
#=> - Subtile diamond forest packer (10 x 5) -
#================================================================================================================================#

DIA_W = 10
DIA_H = 5
PAD_TOP = 14
PAD_SIDE = 10
PAD_BOT = 4
OUT_DIR = "/home/w/Projects/img-content/forest_subtile"
N_VAR = 5
PINE_N_LO, PINE_N_HI = 3, 6
BROAD_N_LO, BROAD_N_HI = 1, 2

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
    for _ in range (4000):
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
            shadows.append ((shadow, x + 1, y))
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

def save_set (tag, maker, n_lo, n_hi):
    os.makedirs (OUT_DIR, exist_ok=True)
    paths = []
    base = 3000 if tag == "pine" else 4000
    for i in range (1, N_VAR + 1):
        path = os.path.join (OUT_DIR, "%s_%02d.png" % (tag, i))
        if os.path.exists (path):
            raise RuntimeError ("refusing to overwrite: %s" % path)
        rng = random.Random (base + i)
        tree_n = rng.randint (n_lo, n_hi)
        img = pack_tile (maker, tree_n, seed=base + i * 17)
        img.save (path)
        paths.append (path)
        print ("wrote %s  %dx%d  trees=%d  tree_px=%dx%d" % (
            path, img.width, img.height, tree_n, maker.TREE_PX_WIDTH, maker.TREE_PX_HEIGHT))
    guide = pack_tile (maker, n_hi, seed=base + 900)
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
    if not os.path.isdir (OUT_DIR):
        os.makedirs (OUT_DIR, exist_ok=True)
    existing = [f for f in os.listdir (OUT_DIR) if f.endswith (".png")]
    if existing:
        print ("*** forest_subtile already has files; remove them first")
        print ("    %s" % OUT_DIR)
        sys.exit (1)
    save_set ("pine", PineTreeMaker (), PINE_N_LO, PINE_N_HI)
    save_set ("broadleaf", BroadleafTreeMaker (), BROAD_N_LO, BROAD_N_HI)
    print ("done  out=%s" % OUT_DIR)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
