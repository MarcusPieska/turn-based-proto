//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_pat.h"
#include "dia_corn.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <dirent.h>
#include <chrono>

static const int PAT_N = SUBTILE_DIV;
static const int PAT_HALF = SUBTILE_DIV / 2;

//================================================================================================================================
//=> - Class: PatBank -
//================================================================================================================================

PatBank::PatBank () {}
PatBank::~PatBank () {}

static bool corn_triple (int a, int b, int c, int t) {
    if (a < 0 || b < 0 || c < 0) return false;
    return a == b && b == c && a != t;
}

void PatBank::der_corn (const std::vector<uint8_t> &pat, std::vector<uint8_t> &out) const {
    std::vector<int> grid ((size_t)PAT_N * PAT_N);
    for (int i = 0; i < PAT_N * PAT_N; i++) grid[i] = (int)pat[i];
    out.assign ((size_t)PAT_N * PAT_N, 0);
    dia_corn_grid (grid.data (), PAT_N, PAT_N, out.data ());
}

bool PatBank::ld (const std::string &dir, const char *prefix) {
    m_pats.clear ();
    m_corn.clear ();
    DIR *d = opendir (dir.c_str ());
    if (!d) return false;
    std::vector<int> ids;
    char fmt[64];
    std::snprintf (fmt, sizeof (fmt), "%s-%%d.data", prefix);
    for (dirent *e = readdir (d); e; e = readdir (d)) {
        int id = -1;
        if (std::sscanf (e->d_name, fmt, &id) == 1 && id >= 0) ids.push_back (id);
    }
    closedir (d);
    std::sort (ids.begin (), ids.end ());
    char path[512];
    for (int id : ids) {
        std::snprintf (path, sizeof (path), "%s/%s-%03d.data", dir.c_str (), prefix, id);
        FILE *f = fopen (path, "r");
        if (!f) return false;
        std::vector<uint8_t> cells;
        char line[128];
        while (fgets (line, sizeof (line), f)) {
            if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;
            int n = 0;
            while (line[n] >= '0' && line[n] <= '9') n++;
            if (n != PAT_N) { fclose (f); return false; }
            for (int i = 0; i < PAT_N; i++) cells.push_back ((uint8_t)(line[i] - '0'));
        }
        fclose (f);
        if ((int)cells.size () != PAT_N * PAT_N) return false;
        std::vector<uint8_t> corn;
        der_corn (cells, corn);
        m_pats.push_back (std::move (cells));
        m_corn.push_back (std::move (corn));
    }
    return !m_pats.empty ();
}

int PatBank::n () const { return (int)m_pats.size (); }

const uint8_t *PatBank::get (int i) const {
    if (i < 0 || i >= (int)m_pats.size ()) return nullptr;
    return m_pats[i].data ();
}

const uint8_t *PatBank::corn (int i) const {
    if (i < 0 || i >= (int)m_corn.size ()) return nullptr;
    return m_corn[i].data ();
}

//================================================================================================================================
//=> - Class: PatSet -
//================================================================================================================================

PatSet::PatSet () {}
PatSet::~PatSet () {}

bool PatSet::ld (const std::string &root) {
    if (!m_corner.ld (root + "/beach_corner_bank", "corner")) return false;
    if (!m_hsplit.ld (root + "/beach_hsplit_bank", "hsplit")) return false;
    if (!m_tcross.ld (root + "/beach_tcross_bank", "tcross")) return false;
    if (!m_diag.ld (root + "/beach_diag_bank", "diag")) return false;
    if (!m_quad.ld (root + "/beach_quad_bank", "quad")) return false;
    return true;
}

const PatBank& PatSet::bank (PatKind k) const {
    switch (k) {
        case PAT_CORNER: return m_corner;
        case PAT_HSPLIT: return m_hsplit;
        case PAT_TCROSS: return m_tcross;
        case PAT_DIAG: return m_diag;
        case PAT_DDIAG: return m_diag;
        case PAT_QUAD: return m_quad;
        default: return m_corner;
    }
}

//================================================================================================================================
//=> - Class: JuncRun -
//================================================================================================================================

JuncRun::JuncRun () : m_pats(nullptr), m_n(0), m_tmin(0), m_tmax(0), m_tsum(0) {}
JuncRun::~JuncRun () {}

void JuncRun::bind (const PatSet *pats) { m_pats = pats; }

uint32_t JuncRun::seed (int a, int b, int c, int d) const {
    uint32_t s = 2166136261u;
    s = (s ^ (uint32_t)a) * 16777619u;
    s = (s ^ (uint32_t)b) * 16777619u;
    s = (s ^ (uint32_t)c) * 16777619u;
    s = (s ^ (uint32_t)d) * 16777619u;
    return s ? s : 1u;
}

int JuncRun::pick (uint32_t &s, int n) const {
    if (n <= 0) return 0;
    s = s * 1664525u + 1013904223u;
    return (int)(s % (uint32_t)n);
}

void JuncRun::xform (int pr, int pc, int rot, bool flip_h, int &qr, int &qc) const {
    int r = pr, c = pc;
    if (flip_h) c = PAT_N - 1 - c;
    if (rot == 1) { int t = r; r = c; c = PAT_N - 1 - t; }
    else if (rot == 2) { r = PAT_N - 1 - r; c = PAT_N - 1 - c; }
    else if (rot == 3) { int t = r; r = PAT_N - 1 - c; c = t; }
    qr = r;
    qc = c;
}

uint8_t JuncRun::xform_corn (uint8_t bits, int rot, bool flip_h) const {
    bool n = (bits & CORN_N) != 0;
    bool e = (bits & CORN_E) != 0;
    bool s = (bits & CORN_S) != 0;
    bool w = (bits & CORN_W) != 0;
    if (flip_h) { bool t = e; e = w; w = t; }
    for (int i = 0; i < rot; i++) {
        bool tn = n;
        n = w;
        w = s;
        s = e;
        e = tn;
    }
    uint8_t o = 0;
    if (n) o |= CORN_N;
    if (e) o |= CORN_E;
    if (s) o |= CORN_S;
    if (w) o |= CORN_W;
    return o;
}

void JuncRun::apply (SubGrid &sg, int tr, int tc, const uint8_t *pat, const uint8_t *corn, int rot, bool flip_h,
                     const int *ter, bool beach_ok) {
    auto plat = [&](int r, int c) -> int {
        if (r < 0 || c < 0 || r >= PAT_N || c >= PAT_N) return -1;
        return (int)pat[r * PAT_N + c];
    };
    auto resolve = [&](int lab) -> int {
        int slot = (lab >= 4) ? lab - 4 : lab;
        if (slot < 0 || slot > 3) slot = 0;
        int tid = ter[slot];
        if (lab >= 4 && beach_ok && ter_beach_water (tid))
            tid = TER_DESERT;
        return tid;
    };
    for (int pr = 0; pr < PAT_N; pr++) {
        for (int pc = 0; pc < PAT_N; pc++) {
            int qr, qc;
            xform (pr, pc, rot, flip_h, qr, qc);
            int lab = (int)pat[pr * PAT_N + pc];
            int id = resolve (lab);
            int tile_r, tile_c, si, sj;
            if (qr < PAT_HALF && qc < PAT_HALF) {
                tile_r = tr;
                tile_c = tc;
                si = PAT_HALF + qr;
                sj = PAT_HALF - 1 - qc;
            } else if (qr < PAT_HALF) {
                tile_r = tr;
                tile_c = tc + 1;
                si = PAT_HALF + qr;
                sj = PAT_N - 1 - (qc - PAT_HALF);
            } else if (qc < PAT_HALF) {
                tile_r = tr + 1;
                tile_c = tc;
                si = qr - PAT_HALF;
                sj = PAT_HALF - 1 - qc;
            } else {
                tile_r = tr + 1;
                tile_c = tc + 1;
                si = qr - PAT_HALF;
                sj = PAT_N - 1 - (qc - PAT_HALF);
            }
            int li = PAT_N - 1 - sj;
            int lj = si;
            int sr = tile_r * SUBTILE_DIV + li;
            int sc = tile_c * SUBTILE_DIV + lj;
            sg.set (sr, sc, id, lab >= 4);

            uint8_t bits = 0;
            int on = -1, oe = -1, os = -1, ow = -1;
            auto add = [&](uint8_t tip, int oa, int ob, int oc) {
                if (!corn_triple (oa, ob, oc, lab)) return;
                int oid = resolve (oa);
                uint8_t wt = dia_tips_si_sj (xform_corn (tip, rot, flip_h));
                bits |= wt;
                if (wt & CORN_N) on = oid;
                if (wt & CORN_E) oe = oid;
                if (wt & CORN_S) os = oid;
                if (wt & CORN_W) ow = oid;
            };
            (void)corn;
            add (CORN_W, plat (pr - 1, pc - 1), plat (pr - 1, pc), plat (pr, pc - 1));
            add (CORN_N, plat (pr - 1, pc), plat (pr - 1, pc + 1), plat (pr, pc + 1));
            add (CORN_E, plat (pr, pc + 1), plat (pr + 1, pc + 1), plat (pr + 1, pc));
            add (CORN_S, plat (pr, pc - 1), plat (pr + 1, pc - 1), plat (pr + 1, pc));
            sg.set_corn (sr, sc, bits);
            sg.set_oth (sr, sc, on, oe, os, ow);
        }
    }
}

void JuncRun::run (const BlendMap &map, SubGrid &sg) {
    m_n = 0;
    m_tmin = 0;
    m_tmax = 0;
    m_tsum = 0;
    if (!m_pats) return;
    using clock = std::chrono::steady_clock;
    for (int r = 0; r + 1 < map.rows (); r++) {
        for (int c = 0; c + 1 < map.cols (); c++) {
            int nw = map.get (r, c);
            int ne = map.get (r, c + 1);
            int sw = map.get (r + 1, c);
            int se = map.get (r + 1, c + 1);
            if (nw == ne && ne == sw && sw == se) continue;

            auto t0 = clock::now ();

            PatKind kind = PAT_NONE;
            int rot = 0;
            bool flip_h = false;
            int ter[4] = { 0, 0, 0, 0 };
            int t[4] = { nw, ne, sw, se };

            int cnt[TERRAIN_ID_MAX];
            std::memset (cnt, 0, sizeof (cnt));
            for (int i = 0; i < 4; i++) if (t[i] >= 0 && t[i] < TERRAIN_ID_MAX) cnt[t[i]]++;
            int nuniq = 0;
            for (int i = 0; i < TERRAIN_ID_MAX; i++) if (cnt[i]) nuniq++;

            if (nuniq == 2) {
                int a = -1, b = -1;
                for (int i = 0; i < TERRAIN_ID_MAX; i++) {
                    if (cnt[i] == 0) continue;
                    if (a < 0) a = i; else b = i;
                }
                if (cnt[a] == 3 || cnt[b] == 3) {
                    int solo = (cnt[a] == 1) ? a : b;
                    int maj = (solo == a) ? b : a;
                    int which = 0;
                    for (int i = 0; i < 4; i++) if (t[i] == solo) which = i;
                    kind = PAT_CORNER;
                    ter[0] = maj;
                    ter[1] = solo;
                    if (which == 3) { rot = 0; flip_h = false; }
                    else if (which == 2) { rot = 0; flip_h = true; }
                    else if (which == 1) { rot = 2; flip_h = true; }
                    else { rot = 2; flip_h = false; }
                } else if (nw == ne && sw == se) {
                    kind = PAT_HSPLIT;
                    ter[0] = nw;
                    ter[1] = sw;
                    rot = 0;
                } else if (nw == sw && ne == se) {
                    kind = PAT_HSPLIT;
                    ter[0] = nw;
                    ter[1] = ne;
                    rot = 3;
                } else {
                    kind = PAT_DDIAG;
                    ter[0] = nw;
                    ter[1] = ne;
                    ter[2] = ne;
                    rot = 0;
                    flip_h = false;
                }
            } else if (nuniq == 3) {
                if (nw == ne || sw == se || nw == sw || ne == se) {
                    kind = PAT_TCROSS;
                    if (nw == ne) {
                        ter[0] = nw; ter[1] = sw; ter[2] = se; rot = 0;
                    } else if (sw == se) {
                        ter[0] = sw; ter[1] = ne; ter[2] = nw; rot = 2;
                    } else if (nw == sw) {
                        ter[0] = nw; ter[1] = se; ter[2] = ne; rot = 3;
                    } else {
                        ter[0] = ne; ter[1] = nw; ter[2] = sw; rot = 1;
                    }
                } else {
                    kind = PAT_DIAG;
                    if (nw == se) {
                        ter[0] = nw; ter[1] = ne; ter[2] = sw; rot = 0; flip_h = false;
                    } else {
                        ter[0] = ne; ter[1] = nw; ter[2] = se; rot = 0; flip_h = true;
                    }
                }
            } else {
                kind = PAT_QUAD;
                ter[0] = nw; ter[1] = ne; ter[2] = sw; ter[3] = se;
                rot = 0;
                flip_h = false;
            }

            bool beach_ok = false;
            for (int i = 0; i < 4; i++) {
                if (!ter_any_water (t[i])) { beach_ok = true; break; }
            }

            const PatBank &bk = m_pats->bank (kind);
            uint32_t s = seed (r, c, kind, nw + 17 * ne + 31 * sw + 61 * se);
            int pi = pick (s, bk.n ());
            const uint8_t *pat = bk.get (pi);
            const uint8_t *corn = bk.corn (pi);
            if (pat) apply (sg, r, c, pat, corn, rot, flip_h, ter, beach_ok);

            auto t1 = clock::now ();
            double us = std::chrono::duration<double, std::micro> (t1 - t0).count ();
            if (m_n == 0) { m_tmin = us; m_tmax = us; }
            else {
                if (us < m_tmin) m_tmin = us;
                if (us > m_tmax) m_tmax = us;
            }
            m_tsum += us;
            m_n++;
        }
    }
}

int JuncRun::n_junc () const { return m_n; }
double JuncRun::t_min_us () const { return m_tmin; }
double JuncRun::t_max_us () const { return m_tmax; }
double JuncRun::t_avg_us () const { return m_n > 0 ? m_tsum / (double)m_n : 0.0; }

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
