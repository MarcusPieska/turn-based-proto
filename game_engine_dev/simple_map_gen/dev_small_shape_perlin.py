#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

#================================================================================================================================#
#=> - PerlinNoise -
#================================================================================================================================#

_GRAD2 = (
    (1.0, 1.0), (-1.0, 1.0), (1.0, -1.0), (-1.0, -1.0),
    (1.0, 0.0), (-1.0, 0.0), (1.0, 0.0), (-1.0, 0.0),
    (0.0, 1.0), (0.0, -1.0), (0.0, 1.0), (0.0, -1.0),
    (1.0, 1.0), (0.0, -1.0), (-1.0, 1.0), (0.0, -1.0),
)

def _fade(t):
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)

def _lerp(a, b, t):
    return a + t * (b - a)

def _fast_floor(x):
    i = int(x)
    return i - (1 if x < float(i) else 0)

def _grad2(h, x, y):
    g = _GRAD2[h & 15]
    return g[0] * x + g[1] * y

class PerlinNoise:
    def __init__(self, seed):
        perm = list(range(256))
        for i in range(255, 0, -1):
            seed = (seed * 1664525 + 1013904223) & 0xFFFFFFFF
            j = seed % (i + 1)
            perm[i], perm[j] = perm[j], perm[i]
        self._perm = perm + perm

    def noise2(self, x, y):
        xi0 = _fast_floor(x)
        yi0 = _fast_floor(y)
        xi = xi0 & 255
        yi = yi0 & 255
        xf = x - float(xi0)
        yf = y - float(yi0)
        u = _fade(xf)
        v = _fade(yf)
        p = self._perm
        aa = p[xi + p[yi]]
        ba = p[xi + 1 + p[yi]]
        ab = p[xi + p[yi + 1]]
        bb = p[xi + 1 + p[yi + 1]]
        x1 = _lerp(_grad2(aa, xf, yf), _grad2(ba, xf - 1.0, yf), u)
        x2 = _lerp(_grad2(ab, xf, yf - 1.0), _grad2(bb, xf - 1.0, yf - 1.0), u)
        return _lerp(x1, x2, v)

#================================================================================================================================#
#=> - FBM field -
#================================================================================================================================#

OCTAVES_DEF = 9
PERSISTENCE_DEF = 0.5
SAMPLE_SCALE = 8.0

def fbm_field(w, h, seed, frequency=5.0, lacunarity=2.0, octaves=OCTAVES_DEF, persistence=PERSISTENCE_DEF):
    gen = PerlinNoise(seed)
    invw = 1.0 / float(w)
    invh = 1.0 / float(h)
    k = frequency * SAMPLE_SCALE
    lacp = 1.0
    amp = 1.0
    dcx = []
    dcy = []
    amps = []
    for _ in range(max(1, octaves)):
        dcx.append(invw * k * lacp)
        dcy.append(invh * k * lacp)
        amps.append(amp)
        lacp *= lacunarity
        amp *= persistence
    out = [[0.0] * w for _ in range(h)]
    for y in range(h):
        cy = float(y)
        for x in range(w):
            cx = float(x)
            s = 0.0
            for o in range(len(dcx)):
                s += gen.noise2(cx * dcx[o], cy * dcy[o]) * amps[o]
            out[y][x] = s
    lo = min(out[y][x] for y in range(h) for x in range(w))
    hi = max(out[y][x] for y in range(h) for x in range(w))
    span = hi - lo + 1e-12
    for y in range(h):
        for x in range(w):
            out[y][x] = (out[y][x] - lo) / span
    return out

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
