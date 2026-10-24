//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "dia_sisj.h"
#include "dia_corn.h"

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <png.h>

//================================================================================================================================
//=> - Class: DiaSiSj -
//================================================================================================================================

DiaSiSj::DiaSiSj () : m_sw(0), m_sh(0), m_ox(0), m_oy(0), m_splat(SPLAT_RED),
                      m_tips(CORN_N | CORN_E | CORN_S | CORN_W) {}
DiaSiSj::~DiaSiSj () {}

bool DiaSiSj::ld (const std::string &bin_path) {
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
    m_rgb.clear ();
    return m_sw > 0 && m_sh > 0;
}

bool DiaSiSj::ld_png (const std::string &png_path) {
    FILE *f = fopen (png_path.c_str (), "rb");
    if (!f) return false;
    png_structp png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose (f); return false; }
    png_infop info = png_create_info_struct (png);
    if (!info) { png_destroy_read_struct (&png, NULL, NULL); fclose (f); return false; }
    if (setjmp (png_jmpbuf (png))) {
        png_destroy_read_struct (&png, &info, NULL);
        fclose (f);
        return false;
    }
    png_init_io (png, f);
    png_read_info (png, info);
    int w = (int)png_get_image_width (png, info);
    int h = (int)png_get_image_height (png, info);
    png_byte ct = png_get_color_type (png, info);
    png_byte bd = png_get_bit_depth (png, info);
    if (bd == 16) png_set_strip_16 (png);
    if (ct == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb (png);
    if (ct == PNG_COLOR_TYPE_GRAY && bd < 8) png_set_expand_gray_1_2_4_to_8 (png);
    if (png_get_valid (png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha (png);
    if (ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (png);
    if (ct == PNG_COLOR_TYPE_RGB || ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_PALETTE)
        png_set_filler (png, 0xFF, PNG_FILLER_AFTER);
    png_read_update_info (png, info);
    if (w != m_sw || h != m_sh) {
        png_destroy_read_struct (&png, &info, NULL);
        fclose (f);
        return false;
    }
    size_t rowbytes = png_get_rowbytes (png, info);
    std::vector<uint8_t> raw ((size_t)h * rowbytes);
    std::vector<png_bytep> rows ((size_t)h);
    for (int y = 0; y < h; y++) rows[y] = raw.data () + (size_t)y * rowbytes;
    png_read_image (png, rows.data ());
    png_destroy_read_struct (&png, &info, NULL);
    fclose (f);
    int bpp = (int)(rowbytes / (size_t)w);
    if (bpp < 3) return false;
    m_rgb.assign ((size_t)m_sw * m_sh * 3, 0);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            size_t si = ((size_t)y * w + x) * 3;
            size_t di = (size_t)y * rowbytes + (size_t)x * bpp;
            m_rgb[si] = raw[di];
            m_rgb[si + 1] = raw[di + 1];
            m_rgb[si + 2] = raw[di + 2];
        }
    }
    return true;
}

bool DiaSiSj::ld_one_splat (const std::string &path, std::vector<uint8_t> &out) const {
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

bool DiaSiSj::ld_splat (const std::string &root) {
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

void DiaSiSj::set_splat (SplatMode mode) { m_splat = mode; }
void DiaSiSj::set_tips (uint8_t allow) { m_tips = allow ? allow : (CORN_N | CORN_E | CORN_S | CORN_W); }

int DiaSiSj::sw () const { return m_sw; }
int DiaSiSj::sh () const { return m_sh; }
int DiaSiSj::ox () const { return m_ox; }
int DiaSiSj::oy () const { return m_oy; }

bool DiaSiSj::on (int sx, int sy) const {
    if (sx < 0 || sy < 0 || sx >= m_sw || sy >= m_sh) return false;
    return m_mask[(size_t)sy * m_sw + sx] != 0;
}

int DiaSiSj::sp_ix (int x, int y) const {
    unsigned u = (unsigned)x * 374761393u + (unsigned)y * 668265263u;
    u ^= u >> 13;
    return (int)(u & 1u);
}

bool DiaSiSj::splat_on (uint8_t tip, int sx, int sy, int ix) const {
    if (sx < 0 || sy < 0 || sx >= m_sw || sy >= m_sh) return false;
    if (ix < 0 || ix >= SP_N) ix = 0;
    size_t i = (size_t)sy * m_sw + sx;
    if (tip == CORN_N) return i < m_sn[ix].size () && m_sn[ix][i];
    if (tip == CORN_E) return i < m_se[ix].size () && m_se[ix][i];
    if (tip == CORN_S) return i < m_ss[ix].size () && m_ss[ix][i];
    if (tip == CORN_W) return i < m_sw_splat[ix].size () && m_sw_splat[ix][i];
    return false;
}

uint8_t DiaSiSj::tip_hit (uint8_t corn, int sx, int sy, int ix) const {
    uint8_t c = corn & m_tips;
    if ((c & CORN_N) && splat_on (CORN_N, sx, sy, ix)) return CORN_N;
    if ((c & CORN_E) && splat_on (CORN_E, sx, sy, ix)) return CORN_E;
    if ((c & CORN_S) && splat_on (CORN_S, sx, sy, ix)) return CORN_S;
    if ((c & CORN_W) && splat_on (CORN_W, sx, sy, ix)) return CORN_W;
    return 0;
}

Rgb DiaSiSj::darken (Rgb c, float t) const {
    return Rgb ((uint8_t)std::max (0, (int)(c.r * t)),
                (uint8_t)std::max (0, (int)(c.g * t)),
                (uint8_t)std::max (0, (int)(c.b * t)));
}

Rgb DiaSiSj::stamp_col (int sx, int sy, int tid, bool splat) const {
    if (tid < 0 || tid >= TERRAIN_N) tid = 0;
    Rgb base = TER_COL[tid];
    if (m_rgb.size () == (size_t)m_sw * m_sh * 3) {
        size_t i = ((size_t)sy * m_sw + sx) * 3;
        uint8_t sr = m_rgb[i], sg = m_rgb[i + 1], sb = m_rgb[i + 2];
        if (sr > 200 && sg < 80) {
            Rgb accent (sr, sg, sb);
            return splat ? darken (accent, 0.55f) : accent;
        }
        float y = sr ? (sr / 220.0f) : 1.0f;
        if (y < 0.35f) y = 0.35f;
        if (y > 1.0f) y = 1.0f;
        Rgb col ((uint8_t)std::min (255, (int)(base.r * y)),
                 (uint8_t)std::min (255, (int)(base.g * y)),
                 (uint8_t)std::min (255, (int)(base.b * y)));
        return splat ? darken (col, 0.42f) : col;
    }
    return splat ? darken (base, 0.42f) : base;
}

//================================================================================================================================
//=> - Corn fill (diamond lattice neighbors; +sj crosses to col-1, not col+1) -
//================================================================================================================================

static bool lat_step (int trows, int tcols, int r, int c, int si, int sj,
                      int dsi, int dsj, int *or_, int *oc, int *osi, int *osj) {
    int nsi = si + dsi, nsj = sj + dsj, nr = r, nc = c;
    if (nsi >= SUBTILE_DIV) { nsi -= SUBTILE_DIV; nr++; }
    else if (nsi < 0) { nsi += SUBTILE_DIV; nr--; }
    if (nsj >= SUBTILE_DIV) { nsj -= SUBTILE_DIV; nc--; }
    else if (nsj < 0) { nsj += SUBTILE_DIV; nc++; }
    if (nr < 0 || nc < 0 || nr >= trows || nc >= tcols) return false;
    *or_ = nr; *oc = nc; *osi = nsi; *osj = nsj;
    return true;
}

void DiaSiSj::fill_corn (SubGrid &sg) const {
    const int N = SUBTILE_DIV;
    const int trows = sg.srows () / N, tcols = sg.scols () / N;
    auto ter = [&](int r, int c, int si, int sj) -> int {
        return sg.get (r * N + si, c * N + sj);
    };
    auto nb = [&](int r, int c, int si, int sj, int dsi, int dsj) -> int {
        int nr, nc, nsi, nsj;
        if (!lat_step (trows, tcols, r, c, si, sj, dsi, dsj, &nr, &nc, &nsi, &nsj))
            return -1;
        return ter (nr, nc, nsi, nsj);
    };
    auto trip = [](int a, int b, int c, int t) -> bool {
        return a >= 0 && a == b && b == c && a != t;
    };

    for (int r = 0; r < trows; r++) {
        for (int c = 0; c < tcols; c++) {
            for (int si = 0; si < N; si++) {
                for (int sj = 0; sj < N; sj++) {
                    int t = ter (r, c, si, sj);
                    uint8_t raw = 0;
                    int on = -1, oe = -1, os = -1, ow = -1;
                    int a, b, c3;
                    a = nb (r, c, si, sj, -1, -1);
                    b = nb (r, c, si, sj, -1, 0);
                    c3 = nb (r, c, si, sj, 0, -1);
                    if (trip (a, b, c3, t)) { raw |= CORN_W; ow = a; }
                    a = nb (r, c, si, sj, -1, 0);
                    b = nb (r, c, si, sj, -1, 1);
                    c3 = nb (r, c, si, sj, 0, 1);
                    if (trip (a, b, c3, t)) { raw |= CORN_N; on = a; }
                    a = nb (r, c, si, sj, 0, 1);
                    b = nb (r, c, si, sj, 1, 1);
                    c3 = nb (r, c, si, sj, 1, 0);
                    if (trip (a, b, c3, t)) { raw |= CORN_E; oe = a; }
                    a = nb (r, c, si, sj, 0, -1);
                    b = nb (r, c, si, sj, 1, -1);
                    c3 = nb (r, c, si, sj, 1, 0);
                    if (trip (a, b, c3, t)) { raw |= CORN_S; os = a; }

                    uint8_t bits = dia_tips_si_sj (raw);
                    int sr = r * N + si, sc = c * N + sj;
                    sg.set_corn (sr, sc, bits);
                    if (!bits) {
                        sg.set_oth (sr, sc, -1, -1, -1, -1);
                        continue;
                    }
                    int rn = ow, re = os, rs = oe, rw = on;
                    sg.set_oth (sr, sc,
                                (bits & CORN_N) ? rn : -1,
                                (bits & CORN_E) ? re : -1,
                                (bits & CORN_S) ? rs : -1,
                                (bits & CORN_W) ? rw : -1);
                }
            }
        }
    }
}

//================================================================================================================================
//=> - Blit -
//================================================================================================================================

void DiaSiSj::flat (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid) const {
    std::memset (dst, 0, (size_t)dw * dh * 3);
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
                            Rgb col = stamp_col (sx, sy, id, false);
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
    splat_flat (dst, dw, dh, map, sg, grid);
}

void DiaSiSj::splat_flat (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg,
                          const DiaGrid &grid) const {
    if (m_splat == SPLAT_NONE) return;
    const Rgb red (255, 0, 0);
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
                            Rgb out = red;
                            if (m_splat == SPLAT_OTHER) {
                                int oid = sg.oth (sr, sc, tip);
                                if (oid < 0 || oid >= TERRAIN_N) oid = id;
                                out = stamp_col (sx, sy, oid, true);
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

void DiaSiSj::tex (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid,
                   const DiaDat15 *const *banks, int n_banks) const {
    std::memset (dst, 0, (size_t)dw * dh * 3);
    std::vector<uint8_t> hit ((size_t)dw * dh, 0);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Vx ctr = grid.center (r, c);
            int srow = r - c;
            int scol = (r + c - (srow & 1)) / 2;
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= n_banks) id = 0;
                    const DiaDat15 *b = banks[id];
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
    splat_tex (dst, dw, dh, map, sg, grid, banks, n_banks);
}

void DiaSiSj::splat_tex (uint8_t *dst, int dw, int dh, const BlendMap &map, const SubGrid &sg, const DiaGrid &grid,
                         const DiaDat15 *const *banks, int n_banks) const {
    if (m_splat == SPLAT_NONE) return;
    const Rgb red (255, 0, 0);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Vx ctr = grid.center (r, c);
            int srow = r - c;
            int scol = (r + c - (srow & 1)) / 2;
            for (int si = 0; si < SUBTILE_DIV; si++) {
                for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                    int sr = r * SUBTILE_DIV + si;
                    int sc = c * SUBTILE_DIV + sj;
                    uint8_t corn = sg.corn (sr, sc);
                    if (!corn) continue;
                    int id = sg.get (sr, sc);
                    if (id < 0 || id >= n_banks) id = 0;
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
