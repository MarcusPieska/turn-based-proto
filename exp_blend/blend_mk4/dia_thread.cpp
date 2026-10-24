//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_thread.h"
#include "dia_pull.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <dirent.h>

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

int EdgeThread::idx (int jr, int jc) const {
    return (jr + 1) * m_jcols + (jc + 1);
}

Vx EdgeThread::junc (int jr, int jc) const {
    return m_junc[idx (jr, jc)];
}

Vx EdgeThread::junc2 (int jr, int jc) const {
    return m_junc2[idx (jr, jc)];
}

bool EdgeThread::dup (int jr, int jc) const {
    return m_dup[idx (jr, jc)] != 0;
}

bool EdgeThread::ld_bank (const std::string &dir) {
    m_bank.clear ();
    DIR *d = opendir (dir.c_str ());
    if (!d) return false;
    std::vector<int> ids;
    while (dirent *e = readdir (d)) {
        int id = -1;
        if (std::sscanf (e->d_name, "threading-%d.data", &id) == 1 && id >= 0) ids.push_back (id);
    }
    closedir (d);
    std::sort (ids.begin (), ids.end ());
    ids.erase (std::unique (ids.begin (), ids.end ()), ids.end ());
    for (int id : ids) {
        char path[512];
        std::snprintf (path, sizeof (path), "%s/threading-%03d.data", dir.c_str (), id);
        FILE *f = fopen (path, "r");
        if (!f) continue;
        std::vector<Vx> pts;
        char buf[256];
        while (fgets (buf, sizeof (buf), f)) {
            if (buf[0] == '#' || buf[0] == '\n' || buf[0] == '\0') continue;
            float x = 0, y = 0;
            if (std::sscanf (buf, "%f %f", &x, &y) == 2) pts.push_back (Vx ((int)std::lround (x), (int)std::lround (y)));
        }
        fclose (f);
        if (pts.size () >= 2) m_bank.push_back (pts);
    }
    return !m_bank.empty ();
}

int EdgeThread::n_bank () const { return (int)m_bank.size (); }

int EdgeThread::hash (int r, int c, int kind) const {
    if (m_bank.empty ()) return 0;
    uint32_t s = seed (r, c, kind, 91);
    return pick (s, (int)m_bank.size ());
}

void EdgeThread::calc_juncs (const BlendMap &map, const DiaGrid &grid) {
    m_jrows = m_rows + 2;
    m_jcols = m_cols + 2;
    int nv = m_jrows * m_jcols;
    m_junc.assign (nv, Vx ());
    m_junc2.assign (nv, Vx ());
    m_dup.assign (nv, 0);
    m_diag.assign (nv, 0);
    CornPull pull;
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
            int ox = 0, oy = 0, ox2 = 0, oy2 = 0;
            int i = idx (jr, jc);
            if (jr >= 0 && jc >= 0 && jr + 1 < m_rows && jc + 1 < m_cols) {
                int nw = map.get (jr, jc);
                int ne = map.get (jr, jc + 1);
                int sw = map.get (jr + 1, jc);
                int se = map.get (jr + 1, jc + 1);
                pull.bind (nw, ne, sw, se);
                uint32_t s = seed (jr, jc, 17, 41);
                bool do_dup = (pull.kind () == PULL_DUP);
                int diag = 0;
                if (pull.kind () == PULL_SOLO && pick (s, 100) < SOLO_DUP_PCT) {
                    do_dup = true;
                    int sq = pull.solo ();
                    diag = (sq == 0 || sq == 3) ? 1 : 0;
                }
                if (do_dup) {
                    if (pull.kind () == PULL_DUP) {
                        if (nw == se && ne == sw) diag = pick (s, 2);
                        else if (ne == sw || nw == se) diag = 0;
                        else diag = pick (s, 2);
                    }
                    int q0 = (diag == 0) ? 0 : 1;
                    int q1 = (diag == 0) ? 3 : 2;
                    int nch = CORNER_PULL_N;
                    Vx o0 = pull.into (q0, pick (s, nch));
                    Vx o1 = pull.into (q1, pick (s, nch));
                    ox = o0.x; oy = o0.y;
                    ox2 = o1.x; oy2 = o1.y;
                    m_dup[i] = 1;
                    m_diag[i] = (uint8_t)diag;
                } else {
                    Vx o = pull.off (pick (s, pull.n_off ()));
                    ox = o.x; oy = o.y;
                    ox2 = ox; oy2 = oy;
                }
            }
            m_junc[i] = Vx (geo.x + ox, geo.y + oy);
            m_junc2[i] = Vx (geo.x + ox2, geo.y + oy2);
        }
    }
}

Vx EdgeThread::end (int jr, int jc, int t0r, int t0c, int t1r, int t1c) const {
    int i = idx (jr, jc);
    if (!m_dup[i]) return m_junc[i];
    auto quad = [&](int tr, int tc) -> int {
        if (tr == jr && tc == jc) return 0;
        if (tr == jr && tc == jc + 1) return 1;
        if (tr == jr + 1 && tc == jc) return 2;
        if (tr == jr + 1 && tc == jc + 1) return 3;
        return -1;
    };
    int q0 = quad (t0r, t0c);
    int q1 = quad (t1r, t1c);
    int d = (int)m_diag[i];
    auto from_q = [&](int q) -> Vx {
        if (d == 0) {
            if (q == 0) return m_junc[i];
            if (q == 3) return m_junc2[i];
        } else {
            if (q == 1) return m_junc[i];
            if (q == 2) return m_junc2[i];
        }
        return m_junc[i];
    };
    if (d == 0) {
        if (q0 == 0 || q0 == 3) return from_q (q0);
        if (q1 == 0 || q1 == 3) return from_q (q1);
    } else {
        if (q0 == 1 || q0 == 2) return from_q (q0);
        if (q1 == 1 || q1 == 2) return from_q (q1);
    }
    return m_junc[i];
}

void EdgeThread::morph (Vx a, Vx b, const std::vector<Vx> &tpl, std::vector<Vx> &pts) const {
    pts.clear ();
    if (tpl.size () < 2) {
        pts.push_back (a);
        pts.push_back (b);
        return;
    }
    float ax = (float)a.x, ay = (float)a.y;
    float bx = (float)b.x, by = (float)b.y;
    float tdx = (float)(tpl.back ().x - tpl.front ().x);
    float tdy = (float)(tpl.back ().y - tpl.front ().y);
    float tlen = std::sqrt (tdx * tdx + tdy * tdy);
    float dx = bx - ax, dy = by - ay;
    float len = std::sqrt (dx * dx + dy * dy);
    if (tlen < 1e-3f || len < 1e-3f) {
        pts.push_back (a);
        pts.push_back (b);
        return;
    }
    float ux = tdx / tlen, uy = tdy / tlen;
    float vx = -uy, vy = ux;
    float sx = dx / tlen, sy = dy / tlen;
    float px = -sy, py = sx;
    float x0 = (float)tpl.front ().x, y0 = (float)tpl.front ().y;
    pts.reserve (tpl.size ());
    for (size_t k = 0; k < tpl.size (); k++) {
        float lx = (float)tpl[k].x - x0;
        float ly = (float)tpl[k].y - y0;
        float along = lx * ux + ly * uy;
        float perp = lx * vx + ly * vy;
        float wx = ax + along * sx + perp * px;
        float wy = ay + along * sy + perp * py;
        pts.push_back (Vx ((int)std::lround (wx), (int)std::lround (wy)));
    }
    pts.front () = a;
    pts.back () = b;
}

void EdgeThread::line (Vx a, Vx b, int n_mid, std::vector<Vx> &pts) const {
    if (n_mid < 0) n_mid = 0;
    pts.clear ();
    pts.reserve ((size_t)n_mid + 2);
    pts.push_back (a);
    for (int i = 1; i <= n_mid; i++) {
        float t = (float)i / (float)(n_mid + 1);
        int px = (int)std::lround ((1.f - t) * (float)a.x + t * (float)b.x);
        int py = (int)std::lround ((1.f - t) * (float)a.y + t * (float)b.y);
        pts.push_back (Vx (px, py));
    }
    pts.push_back (b);
}

void EdgeThread::walk_ne (const BlendMap &map, int r, int c) {
    EdgePath ep;
    ep.kind = EDGE_NE;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r, c + 1);
    Vx a = end (r - 1, c, r, c, r, c + 1);
    Vx b = end (r, c, r, c, r, c + 1);
    int ti = hash (r, c, EDGE_NE);
    morph (a, b, m_bank[ti], ep.pts);
    m_paths.push_back (ep);
}

void EdgeThread::walk_se (const BlendMap &map, int r, int c) {
    EdgePath ep;
    ep.kind = EDGE_SE;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r + 1, c);
    Vx a = end (r, c, r, c, r + 1, c);
    Vx b = end (r, c - 1, r, c, r + 1, c);
    int ti = hash (r, c, EDGE_SE);
    morph (a, b, m_bank[ti], ep.pts);
    m_paths.push_back (ep);
}

void EdgeThread::walk_ne_c2c (const BlendMap &map, int r, int c, int c2c) {
    EdgePath ep;
    ep.kind = EDGE_NE;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r, c + 1);
    Vx a = end (r - 1, c, r, c, r, c + 1);
    Vx b = end (r, c, r, c, r, c + 1);
    line (a, b, c2c, ep.pts);
    m_paths.push_back (ep);
}

void EdgeThread::walk_se_c2c (const BlendMap &map, int r, int c, int c2c) {
    EdgePath ep;
    ep.kind = EDGE_SE;
    ep.r = r;
    ep.c = c;
    ep.ta = map.get (r, c);
    ep.tb = map.get (r + 1, c);
    Vx a = end (r, c, r, c, r + 1, c);
    Vx b = end (r, c - 1, r, c, r + 1, c);
    line (a, b, c2c, ep.pts);
    m_paths.push_back (ep);
}

void EdgeThread::link (const BlendMap &map, bool use_bank, int c2c) {
    for (int jr = 0; jr + 1 < m_rows; jr++) {
        for (int jc = 0; jc + 1 < m_cols; jc++) {
            int i = idx (jr, jc);
            if (!m_dup[i]) continue;
            EdgePath ep;
            ep.kind = EDGE_D;
            ep.r = jr;
            ep.c = jc;
            if (m_diag[i] == 0) {
                ep.ta = map.get (jr, jc);
                ep.tb = map.get (jr + 1, jc + 1);
            } else {
                ep.ta = map.get (jr, jc + 1);
                ep.tb = map.get (jr + 1, jc);
            }
            Vx a = m_junc[i];
            Vx b = m_junc2[i];
            if (use_bank && !m_bank.empty ()) {
                int ti = hash (jr, jc, EDGE_D);
                morph (a, b, m_bank[ti], ep.pts);
            } else {
                line (a, b, c2c, ep.pts);
            }
            m_paths.push_back (ep);
        }
    }
}

void EdgeThread::run (const BlendMap &map, const DiaGrid &grid) {
    m_paths.clear ();
    m_rows = map.rows ();
    m_cols = map.cols ();
    calc_juncs (map, grid);
    if (m_bank.empty ()) {
        run_c2c (map, grid, 0);
        return;
    }
    for (int r = 0; r < m_rows; r++) {
        for (int c = 0; c < m_cols; c++) {
            if (c + 1 < m_cols && map.get (r, c) != map.get (r, c + 1)) walk_ne (map, r, c);
            if (r + 1 < m_rows && map.get (r, c) != map.get (r + 1, c)) walk_se (map, r, c);
        }
    }
    link (map, true, 0);
}

void EdgeThread::run_c2c (const BlendMap &map, const DiaGrid &grid, int c2c) {
    m_paths.clear ();
    m_rows = map.rows ();
    m_cols = map.cols ();
    calc_juncs (map, grid);
    for (int r = 0; r < m_rows; r++) {
        for (int c = 0; c < m_cols; c++) {
            if (c + 1 < m_cols && map.get (r, c) != map.get (r, c + 1)) walk_ne_c2c (map, r, c, c2c);
            if (r + 1 < m_rows && map.get (r, c) != map.get (r + 1, c)) walk_se_c2c (map, r, c, c2c);
        }
    }
    link (map, false, c2c);
}

int EdgeThread::n_path () const { return (int)m_paths.size (); }
const EdgePath& EdgeThread::path (int i) const { return m_paths[i]; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
