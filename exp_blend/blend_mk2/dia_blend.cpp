//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_blend.h"

#include <cstdio>
#include <cmath>
#include <algorithm>

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

BlendMap::BlendMap () : m_rows(0), m_cols(0) {}
BlendMap::~BlendMap () {}

bool BlendMap::from_win (const uint8_t *full, int full_w, int full_h, int r0, int c0, int win) {
    if (!full || win <= 0 || r0 < 0 || c0 < 0 || r0 + win > full_h || c0 + win > full_w) return false;
    m_rows = win;
    m_cols = win;
    m_ter.resize (win * win);
    for (int r = 0; r < win; r++) {
        for (int c = 0; c < win; c++) m_ter[r * win + c] = (int)full[(r0 + r) * full_w + (c0 + c)];
    }
    return true;
}

int BlendMap::rows () const { return m_rows; }
int BlendMap::cols () const { return m_cols; }
int BlendMap::get (int r, int c) const { return m_ter[r * m_cols + c]; }

//================================================================================================================================
//=> - Class: EdgeThread -
//================================================================================================================================

EdgeThread::EdgeThread () : m_rows(0), m_cols(0), m_jrows(0), m_jcols(0) {}
EdgeThread::~EdgeThread () {}

uint32_t EdgeThread::seed (int a, int b, int c, int d) const {
    uint32_t s = 2166136261u;
    int v[4] = { a, b, c, d };
    for (int i = 0; i < 4; i++) {
        s ^= (uint32_t)v[i] + 0x9e3779b9u + (s << 6) + (s >> 2);
        s *= 16777619u;
    }
    return s ? s : 1u;
}

int EdgeThread::pick (uint32_t &s, int n) const {
    s = s * 1664525u + 1013904223u;
    return (int)(s % (uint32_t)n);
}

Vx EdgeThread::junc (int jr, int jc) const {
    return m_junc[(jr + 1) * m_jcols + (jc + 1)];
}

// 1 1
// 0 0

void EdgeThread::calc_juncs (const BlendMap &map, const DiaGrid &grid) {
    m_jrows = m_rows + 2;
    m_jcols = m_cols + 2;
    m_junc.assign (m_jrows * m_jcols, Vx ());
    static const int pdx[4] = { -1, 1, -1, 1 };
    static const int pdy[4] = { -1, -1, 1, 1 };
    for (int jr = -1; jr < m_rows; jr++) {
        for (int jc = -1; jc < m_cols; jc++) {
            Vx geo;
            if (jr >= 0 && jc >= 0 && jr < m_rows && jc < m_cols) {
                Vx ctr = grid.center (jr, jc);
                geo = Vx (ctr.x + TILE_HALF_W, ctr.y);
            } else if (jr + 1 >= 0 && jc + 1 >= 0 && jr + 1 < m_rows && jc + 1 < m_cols) {
                Vx ctr = grid.center (jr + 1, jc + 1);
                geo = Vx (ctr.x - TILE_HALF_W, ctr.y);
            } else if (jr + 1 >= 0 && jc >= 0 && jr + 1 < m_rows && jc < m_cols) {
                Vx ctr = grid.center (jr + 1, jc);
                geo = Vx (ctr.x, ctr.y - TILE_HALF_H);
            } else if (jr >= 0 && jc + 1 >= 0 && jr < m_rows && jc + 1 < m_cols) {
                Vx ctr = grid.center (jr, jc + 1);
                geo = Vx (ctr.x, ctr.y + TILE_HALF_H);
            } else {
                continue;
            }
            int off_x = 0, off_y = 0;
            if (jr >= 0 && jc >= 0 && jr + 1 < m_rows && jc + 1 < m_cols) {
                int t[4] = {
                    map.get (jr, jc), map.get (jr, jc + 1),
                    map.get (jr + 1, jc), map.get (jr + 1, jc + 1)
                };
                int cnt[TERRAIN_ID_MAX];
                for (int i = 0; i < TERRAIN_ID_MAX; i++) cnt[i] = 0;
                for (int i = 0; i < 4; i++) {
                    if (t[i] < 0 || t[i] >= TERRAIN_ID_MAX) continue;
                    cnt[t[i]]++;
                }
                int n1 = 0, n2 = 0, n3 = 0;
                for (int i = 0; i < TERRAIN_ID_MAX; i++) {
                    if (cnt[i] == 1) n1++;
                    if (cnt[i] == 2) n2++;
                    if (cnt[i] == 3) n3++;
                }
                int cand[4];
                int nc = 0;
                if (n3 == 1 && n1 == 1) {
                    for (int i = 0; i < 4; i++) if (cnt[t[i]] == 1) cand[nc++] = i;
                } else if (n2 == 1 && n1 == 2) {
                    for (int i = 0; i < 4; i++) if (cnt[t[i]] == 1) cand[nc++] = i;
                } else if (n1 == 4) {
                    for (int i = 0; i < 4; i++) cand[nc++] = i;
                }
                int sx = TILE_HALF_W / SUBTILE_DIV;
                int sy = TILE_HALF_H / SUBTILE_DIV;
                if (nc > 0) {
                    uint32_t s = seed (jr, jc, 17, 41);
                    if (pick (s, 100) < CORNER_PULL_PCT) {
                        int which = cand[pick (s, nc)];
                        off_x = pdx[which] * CORNER_PULL_SUB * sx;
                        off_y = pdy[which] * CORNER_PULL_SUB * sy;
                        std::printf ("pull corner jr=%d jc=%d toward tile%d off=(%d,%d) t=[%d %d / %d %d]\n",
                                     jr, jc, which, off_x, off_y, t[0], t[1], t[2], t[3]);
                    }
                } else if (t[0] == t[1] && t[2] == t[3] && t[0] != t[2]) {
                    uint32_t s = seed (jr, jc, 23, 59);
                    if (pick (s, 100) < STRIPE_PULL_PCT) {
                        off_x = 0;
                        off_y = (pick (s, 2) == 0 ? -1 : 1) * STRIPE_PULL_SUB * sy;
                        std::printf ("dent stripe-H jr=%d jc=%d off=(%d,%d) t=[%d %d / %d %d]\n",
                                     jr, jc, off_x, off_y, t[0], t[1], t[2], t[3]);
                    }
                } else if (t[0] == t[2] && t[1] == t[3] && t[0] != t[1]) {
                    uint32_t s = seed (jr, jc, 29, 71);
                    if (pick (s, 100) < STRIPE_PULL_PCT) {
                        off_x = (pick (s, 2) == 0 ? -1 : 1) * STRIPE_PULL_SUB * sx;
                        off_y = 0;
                        std::printf ("dent stripe-V jr=%d jc=%d off=(%d,%d) t=[%d %d / %d %d]\n",
                                     jr, jc, off_x, off_y, t[0], t[1], t[2], t[3]);
                    }
                }
            }
            m_junc[(jr + 1) * m_jcols + (jc + 1)] = Vx (geo.x + off_x, geo.y + off_y);
        }
    }
}

void EdgeThread::drunk (std::vector<int> &off, uint32_t s) const {
    const int n = SUBTILE_DIV;
    const int max_dev = EDGE_MAX_DEV;
    off.assign (n + 1, 0);
    int y = 0;
    for (int x = 1; x < n; x++) {
        int rem = n - x;
        int lo = y - 1;
        int hi = y + 1;
        if (lo < -max_dev) lo = -max_dev;
        if (hi > max_dev) hi = max_dev;
        if (lo < -rem) lo = -rem;
        if (hi > rem) hi = rem;
        int choices[8];
        int nc = 0;
        for (int ny = lo; ny <= hi; ny++) {
            if (std::abs (ny - y) <= 1) choices[nc++] = ny;
        }
        if (nc <= 0) choices[nc++] = y;
        y = choices[pick (s, nc)];
        off[x] = y;
    }
    off[0] = 0;
    off[n] = 0;
}

void EdgeThread::walk_ne (const BlendMap &map, const DiaGrid &grid, int r, int c) {
    Vx dia[4];
    grid.diamond (r, c, dia);
    Vx ja = junc (r - 1, c);
    Vx jb = junc (r, c);
    std::vector<int> off;
    drunk (off, seed (r, c, r, c + 1));
    EdgePath ep;
    ep.kind = EDGE_NE;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r, c + 1);
    ep.a = dia[0];
    ep.b = dia[1];
    float abx = (float)(ep.b.x - ep.a.x);
    float aby = (float)(ep.b.y - ep.a.y);
    float plen = std::sqrt (abx * abx + aby * aby);
    float step = plen / (float)SUBTILE_DIV;
    float pux = 0.f, puy = 0.f;
    if (plen > 1.f) { pux = -aby / plen; puy = abx / plen; }
    ep.pts.reserve (SUBTILE_DIV + 1);
    for (int i = 0; i <= SUBTILE_DIV; i++) {
        float t = (float)i / (float)SUBTILE_DIV;
        float px = (1.f - t) * (float)ja.x + t * (float)jb.x;
        float py = (1.f - t) * (float)ja.y + t * (float)jb.y;
        px += (float)off[i] * pux * step;
        py += (float)off[i] * puy * step;
        ep.pts.push_back (Vx ((int)std::lround (px), (int)std::lround (py)));
    }
    m_paths.push_back (ep);
}

void EdgeThread::walk_se (const BlendMap &map, const DiaGrid &grid, int r, int c) {
    Vx dia[4];
    grid.diamond (r, c, dia);
    Vx ja = junc (r, c);
    Vx jb = junc (r, c - 1);
    std::vector<int> off;
    drunk (off, seed (r, c, r + 1, c));
    EdgePath ep;
    ep.kind = EDGE_SE;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r + 1, c);
    ep.a = dia[1];
    ep.b = dia[2];
    float abx = (float)(ep.b.x - ep.a.x);
    float aby = (float)(ep.b.y - ep.a.y);
    float plen = std::sqrt (abx * abx + aby * aby);
    float step = plen / (float)SUBTILE_DIV;
    float pux = 0.f, puy = 0.f;
    if (plen > 1.f) { pux = -aby / plen; puy = abx / plen; }
    ep.pts.reserve (SUBTILE_DIV + 1);
    for (int i = 0; i <= SUBTILE_DIV; i++) {
        float t = (float)i / (float)SUBTILE_DIV;
        float px = (1.f - t) * (float)ja.x + t * (float)jb.x;
        float py = (1.f - t) * (float)ja.y + t * (float)jb.y;
        px += (float)off[i] * pux * step;
        py += (float)off[i] * puy * step;
        ep.pts.push_back (Vx ((int)std::lround (px), (int)std::lround (py)));
    }
    m_paths.push_back (ep);
}

void EdgeThread::run (const BlendMap &map, const DiaGrid &grid) {
    m_paths.clear ();
    m_rows = map.rows ();
    m_cols = map.cols ();
    calc_juncs (map, grid);
    for (int r = 0; r < m_rows; r++) {
        for (int c = 0; c < m_cols; c++) {
            if (c + 1 < m_cols && map.get (r, c) != map.get (r, c + 1)) walk_ne (map, grid, r, c);
            if (r + 1 < m_rows && map.get (r, c) != map.get (r + 1, c)) walk_se (map, grid, r, c);
        }
    }
}

int EdgeThread::n_path () const { return (int)m_paths.size (); }
const EdgePath& EdgeThread::path (int i) const { return m_paths[i]; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
