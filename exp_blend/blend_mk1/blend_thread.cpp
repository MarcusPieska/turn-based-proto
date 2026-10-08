//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "blend_thread.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

//================================================================================================================================
//=> - Class: BlendMap -
//================================================================================================================================

BlendMap::BlendMap () : m_rows(0), m_cols(0) {}
BlendMap::~BlendMap () {}

bool BlendMap::ld (const std::string &path) {
    FILE *f = fopen (path.c_str (), "r");
    if (!f) return false;
    int rows = 0, cols = 0;
    if (fscanf (f, "%d %d", &rows, &cols) != 2 || rows <= 0 || cols <= 0) {
        fclose (f);
        return false;
    }
    std::vector<int> ter (rows * cols, 0);
    for (int i = 0; i < rows * cols; i++) {
        int v = 0;
        if (fscanf (f, "%d", &v) != 1 || v < 0 || v >= TERRAIN_ID_MAX) {
            fclose (f);
            return false;
        }
        ter[i] = v;
    }
    fclose (f);
    m_rows = rows;
    m_cols = cols;
    m_ter.swap (ter);
    return true;
}

bool BlendMap::from_win (const uint8_t *full, int full_w, int full_h, int r0, int c0, int win) {
    if (!full || win <= 0 || r0 < 0 || c0 < 0 || r0 + win > full_h || c0 + win > full_w) return false;
    m_rows = win;
    m_cols = win;
    m_ter.resize (win * win);
    for (int r = 0; r < win; r++) {
        for (int c = 0; c < win; c++) {
            m_ter[r * win + c] = (int)full[(r0 + r) * full_w + (c0 + c)];
        }
    }
    return true;
}

int BlendMap::rows () const { return m_rows; }
int BlendMap::cols () const { return m_cols; }
int BlendMap::get (int r, int c) const { return m_ter[r * m_cols + c]; }
int BlendMap::px_w () const { return m_cols * TILE_PX; }
int BlendMap::px_h () const { return m_rows * TILE_PX; }

//================================================================================================================================
//=> - Class: EdgeThread -
//================================================================================================================================

EdgeThread::EdgeThread () : m_rows(0), m_cols(0) {}
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

Vx EdgeThread::junc (int vr, int vc) const {
    return m_junc[vr * (m_cols + 1) + vc];
}

void EdgeThread::calc_juncs (const BlendMap &map) {
    m_junc.assign ((m_rows + 1) * (m_cols + 1), Vx ());
    static const int dxs[4] = { -1, 1, -1, 1 };
    static const int dys[4] = { -1, -1, 1, 1 };
    for (int vr = 0; vr <= m_rows; vr++) {
        for (int vc = 0; vc <= m_cols; vc++) {
            int gx = vc * TILE_PX;
            int gy = vr * TILE_PX;
            int off_x = 0, off_y = 0;
            if (vr > 0 && vr < m_rows && vc > 0 && vc < m_cols) {
                int t[4] = {
                    map.get (vr - 1, vc - 1), map.get (vr - 1, vc),
                    map.get (vr, vc - 1), map.get (vr, vc)
                };
                int cnt[TERRAIN_ID_MAX];
                for (int i = 0; i < TERRAIN_ID_MAX; i++) cnt[i] = 0;
                for (int i = 0; i < 4; i++) {
                    if (t[i] < 0 || t[i] >= TERRAIN_ID_MAX) continue;
                    cnt[t[i]]++;
                }
                int solo_ter = -1, maj_ter = -1;
                for (int i = 0; i < TERRAIN_ID_MAX; i++) {
                    if (cnt[i] == 1) solo_ter = i;
                    if (cnt[i] == 3) maj_ter = i;
                }
                if (solo_ter >= 0 && maj_ter >= 0) {
                    uint32_t s = seed (vr, vc, 17, 41);
                    if (pick (s, 100) < CORNER_PULL_PCT) {
                        int which = 0;
                        for (int i = 0; i < 4; i++) if (t[i] == solo_ter) which = i;
                        off_x = dxs[which] * CORNER_PULL_SUB;
                        off_y = dys[which] * CORNER_PULL_SUB;
                    }
                }
            }
            m_junc[vr * (m_cols + 1) + vc] = Vx (gx + off_x * SUBTILE_PX, gy + off_y * SUBTILE_PX);
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

void EdgeThread::walk_h (const BlendMap &map, int r, int c) {
    Vx a = junc (r + 1, c);
    Vx b = junc (r + 1, c + 1);
    std::vector<int> off;
    drunk (off, seed (r, c, r + 1, c));
    EdgePath ep;
    ep.kind = EDGE_H;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r + 1, c);
    ep.pts.reserve (SUBTILE_DIV + 1);
    for (int i = 0; i <= SUBTILE_DIV; i++) {
        float t = (float)i / (float)SUBTILE_DIV;
        int px = (int)std::lround ((1.f - t) * (float)a.x + t * (float)b.x);
        int py = (int)std::lround ((1.f - t) * (float)a.y + t * (float)b.y);
        py += off[i] * SUBTILE_PX;
        ep.pts.push_back (Vx (px, py));
    }
    m_paths.push_back (ep);
}

void EdgeThread::walk_v (const BlendMap &map, int r, int c) {
    Vx a = junc (r, c + 1);
    Vx b = junc (r + 1, c + 1);
    std::vector<int> off;
    drunk (off, seed (r, c, r, c + 1));
    EdgePath ep;
    ep.kind = EDGE_V;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r, c + 1);
    ep.pts.reserve (SUBTILE_DIV + 1);
    for (int i = 0; i <= SUBTILE_DIV; i++) {
        float t = (float)i / (float)SUBTILE_DIV;
        int px = (int)std::lround ((1.f - t) * (float)a.x + t * (float)b.x);
        int py = (int)std::lround ((1.f - t) * (float)a.y + t * (float)b.y);
        px += off[i] * SUBTILE_PX;
        ep.pts.push_back (Vx (px, py));
    }
    m_paths.push_back (ep);
}

void EdgeThread::run (const BlendMap &map) {
    m_paths.clear ();
    m_rows = map.rows ();
    m_cols = map.cols ();
    calc_juncs (map);
    for (int r = 0; r < m_rows; r++) {
        for (int c = 0; c < m_cols; c++) {
            if (c + 1 < m_cols && map.get (r, c) != map.get (r, c + 1)) walk_v (map, r, c);
            if (r + 1 < m_rows && map.get (r, c) != map.get (r + 1, c)) walk_h (map, r, c);
        }
    }
}

int EdgeThread::n_path () const { return (int)m_paths.size (); }
const EdgePath& EdgeThread::path (int i) const { return m_paths[i]; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
