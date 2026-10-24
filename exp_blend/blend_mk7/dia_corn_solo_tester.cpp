//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#include "dia_cfg.h"
#include "dia_corn.h"
#include "dia_map.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk7-dia/corn_solo";
static const char *G_ROOT = ".";
static const char *G_SUPER = "dia_submask/super_submask.bin";
static const int MARGIN = 8;
static const int SI0 = 4;
static const int SJ0 = 4;
static const int TEX_MAJ = TER_DESERT;
static const int TEX_SOLO = TER_SEA;

//================================================================================================================================
//=> - Image helpers -
//================================================================================================================================

struct Img {
    int w;
    int h;
    std::vector<uint8_t> px;

    Img () : w(0), h(0) {}
    Img (int ww, int hh) : w(ww), h(hh), px((size_t)ww * hh * 3, 0) {}
};

static bool mkdir_p (const char *path) {
    struct stat st;
    if (stat (path, &st) == 0) return S_ISDIR (st.st_mode);
    return mkdir (path, 0755) == 0;
}

static bool save_png (const Img &im, const char *path) {
    FILE *f = fopen (path, "wb");
    if (!f) return false;
    png_structp png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose (f); return false; }
    png_infop info = png_create_info_struct (png);
    if (!info) { png_destroy_write_struct (&png, NULL); fclose (f); return false; }
    if (setjmp (png_jmpbuf (png))) {
        png_destroy_write_struct (&png, &info);
        fclose (f);
        return false;
    }
    png_init_io (png, f);
    png_set_IHDR (png, info, im.w, im.h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info (png, info);
    std::vector<png_bytep> rows (im.h);
    for (int y = 0; y < im.h; y++) rows[y] = (png_bytep)&im.px[y * im.w * 3];
    png_write_image (png, rows.data ());
    png_write_end (png, NULL);
    png_destroy_write_struct (&png, &info);
    fclose (f);
    return true;
}

//================================================================================================================================
//=> - Stamp + splat load -
//================================================================================================================================

struct Stamp {
    int sw, sh, ox, oy;
    std::vector<uint8_t> mask;
    std::vector<uint8_t> sn, se, ss, sw_splat;
};

static bool ld_stamp (const char *path, Stamp &st) {
    FILE *f = fopen (path, "rb");
    if (!f) return false;
    uint16_t sw = 0, sh = 0, ox = 0, oy = 0;
    int32_t min_rx = 0, min_ry = 0;
    if (fread (&sw, 2, 1, f) != 1 || fread (&sh, 2, 1, f) != 1 ||
        fread (&ox, 2, 1, f) != 1 || fread (&oy, 2, 1, f) != 1 ||
        fread (&min_rx, 4, 1, f) != 1 || fread (&min_ry, 4, 1, f) != 1) {
        fclose (f);
        return false;
    }
    st.sw = (int)sw; st.sh = (int)sh; st.ox = (int)ox; st.oy = (int)oy;
    st.mask.assign ((size_t)st.sw * st.sh, 0);
    if (fread (st.mask.data (), 1, st.mask.size (), f) != st.mask.size ()) {
        fclose (f);
        return false;
    }
    fclose (f);
    return st.sw > 0 && st.sh > 0;
}

static bool ld_splat (const char *path, int sw, int sh, std::vector<uint8_t> &out) {
    FILE *f = fopen (path, "r");
    if (!f) return false;
    out.clear ();
    char line[128];
    while (fgets (line, sizeof (line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') continue;
        int n = 0;
        while (line[n] == '0' || line[n] == '1') n++;
        if (n != sw) { fclose (f); return false; }
        for (int i = 0; i < sw; i++) out.push_back ((uint8_t)(line[i] - '0'));
    }
    fclose (f);
    return (int)out.size () == sw * sh;
}

static bool ld_splats (const char *root, Stamp &st) {
    char path[512];
    std::snprintf (path, sizeof (path), "%s/splat_n_bank/n-000.splat", root);
    if (!ld_splat (path, st.sw, st.sh, st.sn)) return false;
    std::snprintf (path, sizeof (path), "%s/splat_e_bank/e-000.splat", root);
    if (!ld_splat (path, st.sw, st.sh, st.se)) return false;
    std::snprintf (path, sizeof (path), "%s/splat_s_bank/s-000.splat", root);
    if (!ld_splat (path, st.sw, st.sh, st.ss)) return false;
    std::snprintf (path, sizeof (path), "%s/splat_w_bank/w-000.splat", root);
    if (!ld_splat (path, st.sw, st.sh, st.sw_splat)) return false;
    return true;
}

static bool mask_on (const Stamp &st, int sx, int sy) {
    return sx >= 0 && sy >= 0 && sx < st.sw && sy < st.sh && st.mask[(size_t)sy * st.sw + sx];
}

static bool splat_on (const Stamp &st, uint8_t tip, int sx, int sy) {
    if (sx < 0 || sy < 0 || sx >= st.sw || sy >= st.sh) return false;
    size_t i = (size_t)sy * st.sw + sx;
    if (tip == CORN_N) return i < st.sn.size () && st.sn[i];
    if (tip == CORN_E) return i < st.se.size () && st.se[i];
    if (tip == CORN_S) return i < st.ss.size () && st.ss[i];
    if (tip == CORN_W) return i < st.sw_splat.size () && st.sw_splat[i];
    return false;
}

static uint8_t tip_hit (const Stamp &st, uint8_t corn, uint8_t tips, int sx, int sy) {
    uint8_t c = corn & tips;
    if ((c & CORN_N) && splat_on (st, CORN_N, sx, sy)) return CORN_N;
    if ((c & CORN_E) && splat_on (st, CORN_E, sx, sy)) return CORN_E;
    if ((c & CORN_S) && splat_on (st, CORN_S, sx, sy)) return CORN_S;
    if ((c & CORN_W) && splat_on (st, CORN_W, sx, sy)) return CORN_W;
    return 0;
}

//================================================================================================================================
//=> - 2x2 subtile layout + draw -
//================================================================================================================================

static void sub_xy (int si, int sj, int cx, int cy, int *px, int *py) {
    *px = cx + (si - sj) * TILE_HALF_W / SUBTILE_DIV;
    *py = cy + (si + sj + 1) * TILE_HALF_H / SUBTILE_DIV - TILE_HALF_H;
}

static void sub_pair (int i, int *si, int *sj) {
    static const int dsi[4] = { 0, 0, 1, 1 };
    static const int dsj[4] = { 0, 1, 0, 1 };
    *si = SI0 + dsi[i];
    *sj = SJ0 + dsj[i];
}

enum SplatMode { SPLAT_RED = 0, SPLAT_OTHER = 1, SPLAT_NONE = 2 };

static void layout (const Stamp &st, int *cx, int *cy, int *iw, int *ih) {
    int min_x = 1e9, min_y = 1e9, max_x = -1e9, max_y = -1e9;
    int tcx = MARGIN + TILE_HALF_W, tcy = MARGIN + TILE_HALF_H;
    for (int i = 0; i < 4; i++) {
        int si, sj, ax, ay;
        sub_pair (i, &si, &sj);
        sub_xy (si, sj, tcx, tcy, &ax, &ay);
        int tlx = ax - st.ox, tly = ay - st.oy;
        if (tlx < min_x) min_x = tlx;
        if (tly < min_y) min_y = tly;
        if (tlx + st.sw - 1 > max_x) max_x = tlx + st.sw - 1;
        if (tly + st.sh - 1 > max_y) max_y = tly + st.sh - 1;
    }
    *cx = tcx + (MARGIN - min_x);
    *cy = tcy + (MARGIN - min_y);
    *iw = (max_x - min_x + 1) + MARGIN * 2;
    *ih = (max_y - min_y + 1) + MARGIN * 2;
}

static void put_rgb (Img &im, int x, int y, Rgb col) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    size_t i = ((size_t)y * im.w + x) * 3;
    im.px[i] = col.r; im.px[i + 1] = col.g; im.px[i + 2] = col.b;
}

static void draw_2x2 (Img &im, const Stamp &st, const int cells[4], const uint8_t corn[4],
                      int cx, int cy, SplatMode mode, uint8_t tips) {
    std::vector<uint8_t> hit ((size_t)im.w * im.h, 0);
    const Rgb red (255, 0, 0);
    for (int i = 0; i < 4; i++) {
        int si, sj, ax, ay;
        sub_pair (i, &si, &sj);
        sub_xy (si, sj, cx, cy, &ax, &ay);
        int tid = cells[i];
        if (tid < 0 || tid >= TERRAIN_N) tid = 0;
        Rgb col = TER_COL[tid];
        int tlx = ax - st.ox, tly = ay - st.oy;
        for (int sy = 0; sy < st.sh; sy++) {
            for (int sx = 0; sx < st.sw; sx++) {
                if (!mask_on (st, sx, sy)) continue;
                int x = tlx + sx, y = tly + sy;
                if (x < 0 || y < 0 || x >= im.w || y >= im.h) continue;
                size_t p = (size_t)y * im.w + x;
                if (hit[p]) continue;
                hit[p] = 1;
                put_rgb (im, x, y, col);
            }
        }
    }
    if (mode == SPLAT_NONE) return;
    for (int i = 0; i < 4; i++) {
        if (!corn[i]) continue;
        int si, sj, ax, ay;
        sub_pair (i, &si, &sj);
        sub_xy (si, sj, cx, cy, &ax, &ay);
        int tid = cells[i];
        if (tid < 0 || tid >= TERRAIN_N) tid = 0;
        int tlx = ax - st.ox, tly = ay - st.oy;
        for (int sy = 0; sy < st.sh; sy++) {
            for (int sx = 0; sx < st.sw; sx++) {
                if (!mask_on (st, sx, sy)) continue;
                uint8_t tip = tip_hit (st, corn[i], tips, sx, sy);
                if (!tip) continue;
                int x = tlx + sx, y = tly + sy;
                Rgb out = red;
                if (mode == SPLAT_OTHER) {
                    int oid = (tid == TEX_MAJ) ? TEX_SOLO : TEX_MAJ;
                    out = TER_COL[oid];
                }
                put_rgb (im, x, y, out);
            }
        }
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *out_dir = G_OUT;
    const char *root = G_ROOT;
    const char *super_path = G_SUPER;
    SplatMode splat = SPLAT_RED;
    uint8_t tips = 0;
    bool tip_filt = false;
    int pos = 0;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp (argv[i], "--other") == 0) { splat = SPLAT_OTHER; continue; }
        if (std::strcmp (argv[i], "--red") == 0) { splat = SPLAT_RED; continue; }
        if (std::strcmp (argv[i], "--none") == 0) { splat = SPLAT_NONE; continue; }
        if (std::strcmp (argv[i], "--N") == 0) { tips |= CORN_N; tip_filt = true; continue; }
        if (std::strcmp (argv[i], "--E") == 0) { tips |= CORN_E; tip_filt = true; continue; }
        if (std::strcmp (argv[i], "--S") == 0) { tips |= CORN_S; tip_filt = true; continue; }
        if (std::strcmp (argv[i], "--W") == 0) { tips |= CORN_W; tip_filt = true; continue; }
        if (pos == 0) out_dir = argv[i];
        pos++;
    }
    if (!tip_filt) tips = CORN_N | CORN_E | CORN_S | CORN_W;
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") ||
        !mkdir_p ("/home/w/Projects/simple-map-gen/blending/mk7-dia") ||
        !mkdir_p (out_dir)) return 1;

    Stamp st;
    if (!ld_stamp (super_path, st) || !ld_splats (root, st)) {
        std::fprintf (stderr, "failed to load stamp/splat under %s / %s\n", super_path, root);
        return 1;
    }

    const char *splat_name = "red";
    if (splat == SPLAT_OTHER) splat_name = "other";
    else if (splat == SPLAT_NONE) splat_name = "none";
    char tip_s[8];
    int tp = 0;
    if (tips & CORN_N) tip_s[tp++] = 'N';
    if (tips & CORN_E) tip_s[tp++] = 'E';
    if (tips & CORN_S) tip_s[tp++] = 'S';
    if (tips & CORN_W) tip_s[tp++] = 'W';
    tip_s[tp] = 0;
    std::printf ("solo 2x2 subtilles  stamp %dx%d  center=(%d,%d)  splat=%s  tips=%s\n",
                 st.sw, st.sh, st.ox, st.oy, splat_name, tip_s);
    std::printf ("maj=desert(%d)  solo=sea(%d)  anchor=(si,sj)=(%d,%d)\n", TEX_MAJ, TEX_SOLO, SI0, SJ0);

    int cx = 0, cy = 0, iw = 0, ih = 0;
    layout (st, &cx, &cy, &iw, &ih);

    static const char *tags[4] = { "sub1_si_sj", "sub2_si_sj1", "sub3_si1_sj", "sub4_si1_sj1" };
    char path[512];
    for (int k = 0; k < 4; k++) {
        int cells[4] = { TEX_MAJ, TEX_MAJ, TEX_MAJ, TEX_MAJ };
        cells[k] = TEX_SOLO;
        uint8_t raw[4], corn[4];
        dia_corn_grid (cells, 2, 2, raw);
        for (int i = 0; i < 4; i++) corn[i] = dia_tips_si_sj (raw[i]);

        Img im (iw, ih);
        draw_2x2 (im, st, cells, corn, cx, cy, splat, tips);

        char tip_desc[32];
        int n = 0;
        tip_desc[0] = 0;
        for (int i = 0; i < 4; i++) {
            if (!corn[i]) continue;
            n++;
            char t[8]; int tp2 = 0;
            if (corn[i] & CORN_N) t[tp2++] = 'N';
            if (corn[i] & CORN_E) t[tp2++] = 'E';
            if (corn[i] & CORN_S) t[tp2++] = 'S';
            if (corn[i] & CORN_W) t[tp2++] = 'W';
            t[tp2] = 0;
            std::snprintf (tip_desc + std::strlen (tip_desc), sizeof (tip_desc) - std::strlen (tip_desc),
                           "%s%d:%s", tip_desc[0] ? " " : "", i + 1, t);
        }

        std::snprintf (path, sizeof (path), "%s/solo_%s_flat.png", out_dir, tags[k]);
        if (!save_png (im, path)) {
            std::fprintf (stderr, "failed to write %s\n", path);
            return 1;
        }
        std::printf ("%s  solo=sub%d  img=%dx%d  corn=%d  %s\n", tags[k], k + 1, iw, ih, n, tip_desc);
        std::printf ("  wrote %s\n", path);
    }

    std::printf ("mk7 dia_corn_solo  wrote 4 images under %s\n", out_dir);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
