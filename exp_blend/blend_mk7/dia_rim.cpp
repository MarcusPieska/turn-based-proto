//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_rim.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Class: DiaRim -
//================================================================================================================================

DiaRim::DiaRim () : m_sw(0), m_sh(0), m_ox(0), m_oy(0),
                    m_tips(CORN_N | CORN_E | CORN_S | CORN_W) {}
DiaRim::~DiaRim () {}

bool DiaRim::ld (const std::string &bin_path) {
    FILE *f = fopen (bin_path.c_str (), "rb");
    if (!f) return false;
    uint16_t sw = 0, sh = 0, ox = 0, oy = 0;
    int32_t min_rx = 0, min_ry = 0;
    if (fread (&sw, 2, 1, f) != 1 || fread (&sh, 2, 1, f) != 1 ||
        fread (&ox, 2, 1, f) != 1 || fread (&oy, 2, 1, f) != 1 ||
        fread (&min_rx, 4, 1, f) != 1 || fread (&min_ry, 4, 1, f) != 1) {
        fclose (f);
        return false;
    }
    m_sw = (int)sw;
    m_sh = (int)sh;
    m_ox = (int)ox;
    m_oy = (int)oy;
    m_mask.assign ((size_t)m_sw * m_sh, 0);
    if (fread (m_mask.data (), 1, m_mask.size (), f) != m_mask.size ()) {
        fclose (f);
        return false;
    }
    fclose (f);
    return m_sw > 0 && m_sh > 0;
}

bool DiaRim::ld_one_splat (const std::string &path, std::vector<uint8_t> &out) const {
    FILE *f = fopen (path.c_str (), "r");
    if (!f) return false;
    out.clear ();
    char line[128];
    while (fgets (line, sizeof (line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;
        int n = 0;
        while (line[n] == '0' || line[n] == '1') n++;
        if (n != m_sw) { fclose (f); return false; }
        for (int i = 0; i < m_sw; i++) out.push_back ((uint8_t)(line[i] - '0'));
    }
    fclose (f);
    return (int)out.size () == m_sw * m_sh;
}

bool DiaRim::ld_splat (const std::string &root) {
    char path[512];
    for (int v = 0; v < SP_N; v++) {
        std::snprintf (path, sizeof (path), "%s/splat_n_bank/n-%03d.splat", root.c_str (), v);
        if (!ld_one_splat (path, m_sn[v])) return false;
        std::snprintf (path, sizeof (path), "%s/splat_e_bank/e-%03d.splat", root.c_str (), v);
        if (!ld_one_splat (path, m_se[v])) return false;
        std::snprintf (path, sizeof (path), "%s/splat_s_bank/s-%03d.splat", root.c_str (), v);
        if (!ld_one_splat (path, m_ss[v])) return false;
        std::snprintf (path, sizeof (path), "%s/splat_w_bank/w-%03d.splat", root.c_str (), v);
        if (!ld_one_splat (path, m_sw_splat[v])) return false;
    }
    return true;
}

void DiaRim::set_tips (uint8_t allow) { m_tips = allow ? allow : (CORN_N | CORN_E | CORN_S | CORN_W); }

int DiaRim::sw () const { return m_sw; }
int DiaRim::sh () const { return m_sh; }
const uint8_t *DiaRim::stamp_rim () const { return m_stamp_rim.empty () ? nullptr : m_stamp_rim.data (); }

const uint8_t *DiaRim::splat_rim (uint8_t tip) const {
    if (tip == CORN_N) return m_rim_n.empty () ? nullptr : m_rim_n.data ();
    if (tip == CORN_E) return m_rim_e.empty () ? nullptr : m_rim_e.data ();
    if (tip == CORN_S) return m_rim_s.empty () ? nullptr : m_rim_s.data ();
    if (tip == CORN_W) return m_rim_w.empty () ? nullptr : m_rim_w.data ();
    return nullptr;
}

bool DiaRim::on (int sx, int sy) const {
    if (sx < 0 || sy < 0 || sx >= m_sw || sy >= m_sh) return false;
    return m_mask[(size_t)sy * m_sw + sx] != 0;
}

int DiaRim::sp_ix (int x, int y) const {
    unsigned u = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    u ^= u >> 13;
    return (int)(u & 1u);
}

bool DiaRim::splat_on (uint8_t tip, int sx, int sy, int ix) const {
    if (sx < 0 || sy < 0 || sx >= m_sw || sy >= m_sh) return false;
    if (ix < 0 || ix >= SP_N) ix = 0;
    size_t i = (size_t)sy * m_sw + sx;
    if (tip == CORN_N) return i < m_sn[ix].size () && m_sn[ix][i];
    if (tip == CORN_E) return i < m_se[ix].size () && m_se[ix][i];
    if (tip == CORN_S) return i < m_ss[ix].size () && m_ss[ix][i];
    if (tip == CORN_W) return i < m_sw_splat[ix].size () && m_sw_splat[ix][i];
    return false;
}

uint8_t DiaRim::tip_hit (uint8_t corn, int sx, int sy, int ix) const {
    uint8_t c = corn & m_tips;
    if ((c & CORN_N) && splat_on (CORN_N, sx, sy, ix)) return CORN_N;
    if ((c & CORN_E) && splat_on (CORN_E, sx, sy, ix)) return CORN_E;
    if ((c & CORN_S) && splat_on (CORN_S, sx, sy, ix)) return CORN_S;
    if ((c & CORN_W) && splat_on (CORN_W, sx, sy, ix)) return CORN_W;
    return 0;
}

void DiaRim::rim_of (const std::vector<uint8_t> &src, std::vector<uint8_t> &out) const {
    out.assign ((size_t)m_sw * m_sh, 0);
    auto at = [&](int x, int y) -> bool {
        if (x < 0 || y < 0 || x >= m_sw || y >= m_sh) return false;
        return src[(size_t)y * m_sw + x] != 0;
    };
    for (int y = 0; y < m_sh; y++) {
        for (int x = 0; x < m_sw; x++) {
            if (!at (x, y)) continue;
            if (!at (x - 1, y) || !at (x + 1, y) || !at (x, y - 1) || !at (x, y + 1))
                out[(size_t)y * m_sw + x] = 1;
        }
    }
}

void DiaRim::prep () {
    rim_of (m_mask, m_stamp_rim);
    auto and_mask = [&](const std::vector<uint8_t> &sp, std::vector<uint8_t> &tmp) {
        tmp.assign ((size_t)m_sw * m_sh, 0);
        for (size_t i = 0; i < tmp.size (); i++)
            tmp[i] = (m_mask[i] && i < sp.size () && sp[i]) ? 1 : 0;
    };
    std::vector<uint8_t> tmp;
    and_mask (m_sn[0], tmp); rim_of (tmp, m_rim_n);
    and_mask (m_se[0], tmp); rim_of (tmp, m_rim_e);
    and_mask (m_ss[0], tmp); rim_of (tmp, m_rim_s);
    and_mask (m_sw_splat[0], tmp); rim_of (tmp, m_rim_w);
}

//================================================================================================================================
//=> - Image-space texture ownership then rim mark -
//================================================================================================================================

void DiaRim::paint_ids (std::vector<int> &ids, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                        const DiaGrid &grid) const {
    ids.assign ((size_t)dw * dh, -1);
    std::vector<uint8_t> hit ((size_t)dw * dh, 0);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= TERRAIN_N) id = 0;
                    Vx a = grid.sub_ctr (r, c, si, sj);
                    int tlx = a.x - m_ox, tly = a.y - m_oy;
                    for (int sy = 0; sy < m_sh; sy++) {
                        for (int sx = 0; sx < m_sw; sx++) {
                            if (!on (sx, sy)) continue;
                            int x = tlx + sx, y = tly + sy;
                            if (x < 0 || y < 0 || x >= dw || y >= dh) continue;
                            size_t p = (size_t)y * dw + x;
                            if (hit[p]) continue;
                            hit[p] = 1;
                            ids[p] = id;
                        }
                    }
                }
            }
        }
    }
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    uint8_t corn = sg.corn (sr, sc);
                    if (!corn) continue;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= TERRAIN_N) id = 0;
                    int ix = sp_ix (sr, sc);
                    Vx a = grid.sub_ctr (r, c, si, sj);
                    int tlx = a.x - m_ox, tly = a.y - m_oy;
                    for (int sy = 0; sy < m_sh; sy++) {
                        for (int sx = 0; sx < m_sw; sx++) {
                            if (!on (sx, sy)) continue;
                            uint8_t tip = tip_hit (corn, sx, sy, ix);
                            if (!tip) continue;
                            int x = tlx + sx, y = tly + sy;
                            if (x < 0 || y < 0 || x >= dw || y >= dh) continue;
                            int oid = sg.oth (sr, sc, tip);
                            if (oid < 0 || oid >= TERRAIN_N) oid = id;
                            ids[(size_t)y * dw + x] = oid;
                        }
                    }
                }
            }
        }
    }
}

void DiaRim::mark (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                   const DiaGrid &grid) const {
    std::vector<int> ids;
    paint_ids (ids, dw, dh, map, sg, grid);
    auto id_at = [&](int x, int y) -> int {
        if (x < 0 || y < 0 || x >= dw || y >= dh) return -1;
        return ids[(size_t)y * dw + x];
    };
    for (int y = 0; y < dh; y++) {
        for (int x = 0; x < dw; x++) {
            int id = id_at (x, y);
            if (id < 0) continue;
            int n = id_at (x, y - 1), e = id_at (x + 1, y), s = id_at (x, y + 1), w = id_at (x - 1, y);
            bool rim = (n >= 0 && n != id) || (e >= 0 && e != id) || (s >= 0 && s != id) || (w >= 0 && w != id);
            if (!rim) continue;
            size_t i = ((size_t)y * dw + x) * 3;
            dst[i] = 0;
            dst[i + 1] = 0;
            dst[i + 2] = 0;
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
