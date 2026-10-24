//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_super.h"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - Class: DiaSuper -
//================================================================================================================================

DiaSuper::DiaSuper () : m_sw(0), m_sh(0), m_ox(0), m_oy(0), m_splat(SPLAT_RED),
                        m_tips(CORN_N | CORN_E | CORN_S | CORN_W) {}
DiaSuper::~DiaSuper () {}

bool DiaSuper::ld (const std::string &bin_path) {
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

bool DiaSuper::ld_one_splat (const std::string &path, std::vector<uint8_t> &out) const {
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

bool DiaSuper::ld_splat (const std::string &root) {
    char path[512];
    std::snprintf (path, sizeof (path), "%s/splat_n_bank/n-000.splat", root.c_str ());
    if (!ld_one_splat (path, m_sn)) return false;
    std::snprintf (path, sizeof (path), "%s/splat_e_bank/e-000.splat", root.c_str ());
    if (!ld_one_splat (path, m_se)) return false;
    std::snprintf (path, sizeof (path), "%s/splat_s_bank/s-000.splat", root.c_str ());
    if (!ld_one_splat (path, m_ss)) return false;
    std::snprintf (path, sizeof (path), "%s/splat_w_bank/w-000.splat", root.c_str ());
    if (!ld_one_splat (path, m_sw_splat)) return false;
    return true;
}

void DiaSuper::set_splat (SplatMode mode) { m_splat = mode; }
void DiaSuper::set_tips (uint8_t allow) { m_tips = allow ? allow : (CORN_N | CORN_E | CORN_S | CORN_W); }

int DiaSuper::sw () const { return m_sw; }
int DiaSuper::sh () const { return m_sh; }
int DiaSuper::ox () const { return m_ox; }
int DiaSuper::oy () const { return m_oy; }

bool DiaSuper::on (int sx, int sy) const {
    if (sx < 0 || sy < 0 || sx >= m_sw || sy >= m_sh) return false;
    return m_mask[(size_t)sy * m_sw + sx] != 0;
}

bool DiaSuper::splat_on (uint8_t tip, int sx, int sy) const {
    if (sx < 0 || sy < 0 || sx >= m_sw || sy >= m_sh) return false;
    size_t i = (size_t)sy * m_sw + sx;
    if (tip == CORN_N) return i < m_sn.size () && m_sn[i];
    if (tip == CORN_E) return i < m_se.size () && m_se[i];
    if (tip == CORN_S) return i < m_ss.size () && m_ss[i];
    if (tip == CORN_W) return i < m_sw_splat.size () && m_sw_splat[i];
    return false;
}

uint8_t DiaSuper::tip_hit (uint8_t corn, int sx, int sy) const {
    uint8_t c = corn & m_tips;
    if ((c & CORN_N) && splat_on (CORN_N, sx, sy)) return CORN_N;
    if ((c & CORN_E) && splat_on (CORN_E, sx, sy)) return CORN_E;
    if ((c & CORN_S) && splat_on (CORN_S, sx, sy)) return CORN_S;
    if ((c & CORN_W) && splat_on (CORN_W, sx, sy)) return CORN_W;
    return 0;
}

void DiaSuper::flat (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid) const {
    std::memset (dst, 0, (size_t)dw * dh * 3);
    std::vector<uint8_t> hit ((size_t)dw * dh, 0);
    const Rgb red (255, 0, 0);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= TERRAIN_N) id = 0;
                    Rgb col = TER_COL[id];
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
                            size_t i = p * 3;
                            dst[i] = col.r;
                            dst[i + 1] = col.g;
                            dst[i + 2] = col.b;
                        }
                    }
                }
            }
        }
    }
    if (m_splat == SPLAT_NONE) return;
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= TERRAIN_N) id = 0;
                    uint8_t corn = sg.corn (sr, sc);
                    if (!corn) continue;
                    Vx a = grid.sub_ctr (r, c, si, sj);
                    int tlx = a.x - m_ox, tly = a.y - m_oy;
                    for (int sy = 0; sy < m_sh; sy++) {
                        for (int sx = 0; sx < m_sw; sx++) {
                            if (!on (sx, sy)) continue;
                            uint8_t tip = tip_hit (corn, sx, sy);
                            if (!tip) continue;
                            int x = tlx + sx, y = tly + sy;
                            if (x < 0 || y < 0 || x >= dw || y >= dh) continue;
                            Rgb out = red;
                            if (m_splat == SPLAT_OTHER) {
                                int oid = sg.oth (sr, sc, tip);
                                if (oid < 0 || oid >= TERRAIN_N) oid = id;
                                out = TER_COL[oid];
                            }
                            size_t i = ((size_t)y * dw + x) * 3;
                            dst[i] = out.r;
                            dst[i + 1] = out.g;
                            dst[i + 2] = out.b;
                        }
                    }
                }
            }
        }
    }
}

void DiaSuper::tex (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid,
                    const DiaDat15 *const *banks, int n_banks) const {
    std::memset (dst, 0, (size_t)dw * dh * 3);
    std::vector<uint8_t> hit ((size_t)dw * dh, 0);
    const Rgb red (255, 0, 0);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Vx ctr = grid.center (r, c);
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= n_banks) id = 0;
                    const DiaDat15 *b = banks[id];
                    Vx a = grid.sub_ctr (r, c, si, sj);
                    int tlx = a.x - m_ox, tly = a.y - m_oy;
                    int srow = r - c;
                    int scol = (r + c - (srow & 1)) / 2;
                    for (int sy = 0; sy < m_sh; sy++) {
                        for (int sx = 0; sx < m_sw; sx++) {
                            if (!on (sx, sy)) continue;
                            int x = tlx + sx, y = tly + sy;
                            if (x < 0 || y < 0 || x >= dw || y >= dh) continue;
                            size_t p = (size_t)y * dw + x;
                            if (hit[p]) continue;
                            hit[p] = 1;
                            int lx = x - (ctr.x - TILE_HALF_W);
                            int ly = y - (ctr.y - TILE_HALF_H);
                            Rgb col = b ? b->sample (scol, srow, lx, ly) : Rgb (255, 0, 255);
                            size_t i = p * 3;
                            dst[i] = col.r;
                            dst[i + 1] = col.g;
                            dst[i + 2] = col.b;
                        }
                    }
                }
            }
        }
    }
    if (m_splat == SPLAT_NONE) return;
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Vx ctr = grid.center (r, c);
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= n_banks) id = 0;
                    uint8_t corn = sg.corn (sr, sc);
                    if (!corn) continue;
                    Vx a = grid.sub_ctr (r, c, si, sj);
                    int tlx = a.x - m_ox, tly = a.y - m_oy;
                    int srow = r - c;
                    int scol = (r + c - (srow & 1)) / 2;
                    for (int sy = 0; sy < m_sh; sy++) {
                        for (int sx = 0; sx < m_sw; sx++) {
                            if (!on (sx, sy)) continue;
                            uint8_t tip = tip_hit (corn, sx, sy);
                            if (!tip) continue;
                            int x = tlx + sx, y = tly + sy;
                            if (x < 0 || y < 0 || x >= dw || y >= dh) continue;
                            int lx = x - (ctr.x - TILE_HALF_W);
                            int ly = y - (ctr.y - TILE_HALF_H);
                            Rgb col = red;
                            if (m_splat == SPLAT_OTHER) {
                                int oid = sg.oth (sr, sc, tip);
                                if (oid < 0 || oid >= n_banks) oid = id;
                                const DiaDat15 *ob = banks[oid];
                                col = ob ? ob->sample (scol, srow, lx, ly) : Rgb (255, 0, 255);
                            }
                            size_t i = ((size_t)y * dw + x) * 3;
                            dst[i] = col.r;
                            dst[i + 1] = col.g;
                            dst[i + 2] = col.b;
                        }
                    }
                }
            }
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
