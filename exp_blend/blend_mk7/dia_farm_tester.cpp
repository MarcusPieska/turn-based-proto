//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_pat.h"
#include "dia_dat15.h"
#include "dia_sisj.h"
#include "dia_edge.h"
#include "dia_save_map.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_SAVE = "/home/w/Projects/game-saves/game-loop-seed-99-p100-0280.bin";
static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk7-dia/farm_tile";
static const char *G_DAT = "/home/w/Projects/rts-proto/game_engine_dev/game_view_mk1";
static const char *G_FARM = "/home/w/Projects/rts-proto/gen_farm";
static const char *G_SET = "set01";
static const char *G_FOREST = "/home/w/Projects/img-content/forest_subtile";
static const char *G_ROOT = ".";
static const char *G_SUPER = "dia_submask/super_submask.bin";
static const char *G_SUPER_PNG = "/home/w/Projects/simple-map-gen/blending/mk7-dia/dia_submask/super_submask.png";
static const int TEX_W = TILE_SPACE_X + 1;
static const int TEX_H = TILE_H + 1;
static const int WIN = 20;
static const int SAVE_MAX = 4;
static const int MARGIN = 8;
static const double LAND_MIN = 0.50;
static const int COLOR_MIN = 3;
static const int FARM_MIN = 8;
static const int BORDER_A = 96;
static const int FARM_OPAQUE = 20;
static const int FOREST_OPAQUE = 100;
static const int FOREST_PINE = 0;
static const int FOREST_BROADLEAF = 1;
static const int FOREST_KIND = FOREST_BROADLEAF;
static const int FOREST_SKIP_PCT = 75;
static const int FOREST_N = 5;
static const int FOREST_PAD_TOP = 14;
static const int FOREST_PAD_SIDE = 10;
static const int FOREST_OX = (TILE_SPACE_X / SUBTILE_DIV) / 2 + FOREST_PAD_SIDE;
static const int FOREST_OY_S = FOREST_PAD_TOP + TILE_H / SUBTILE_DIV;
static const uint8_t KEY_R = 255;
static const uint8_t KEY_G = 0;
static const uint8_t KEY_B = 255;

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

struct ImgA {
    int w;
    int h;
    std::vector<uint8_t> px;

    ImgA () : w(0), h(0) {}
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

static bool ld_png_rgba (const char *path, ImgA &out) {
    FILE *f = fopen (path, "rb");
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
    if (ct == PNG_COLOR_TYPE_RGB || ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_PALETTE)
        png_set_filler (png, 0xff, PNG_FILLER_AFTER);
    if (ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb (png);
    png_read_update_info (png, info);
    out.w = w;
    out.h = h;
    out.px.assign ((size_t)w * h * 4, 0);
    std::vector<png_bytep> rows (h);
    for (int y = 0; y < h; y++) rows[y] = (png_bytep)&out.px[y * w * 4];
    png_read_image (png, rows.data ());
    png_destroy_read_struct (&png, &info, NULL);
    fclose (f);
    return true;
}

static void put_blk (Img &im, int x, int y, int a) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    size_t i = ((size_t)y * im.w + x) * 3;
    int inv = 255 - a;
    im.px[i] = (uint8_t)((im.px[i] * inv) / 255);
    im.px[i + 1] = (uint8_t)((im.px[i + 1] * inv) / 255);
    im.px[i + 2] = (uint8_t)((im.px[i + 2] * inv) / 255);
}

static void line_blk (Img &im, int x0, int y0, int x1, int y1, int a) {
    int dx = x1 - x0, dy = y1 - y0;
    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    int err = ax - ay;
    for (;;) {
        put_blk (im, x0, y0, a);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -ay) { err -= ay; x0 += sx; }
        if (e2 < ax) { err += ax; y0 += sy; }
    }
}

static void draw_tile_brd (Img &im, const DiaGrid &grid, int a) {
    for (int r = 0; r < grid.rows (); r++) {
        for (int c = 0; c < grid.cols (); c++) {
            Vx d[4];
            grid.diamond (r, c, d);
            for (int e = 0; e < 4; e++) {
                int n = (e + 1) & 3;
                line_blk (im, d[e].x, d[e].y, d[n].x, d[n].y, a);
            }
        }
    }
}

static void farm_home (Vx ctr, int x, int y, int &si, int &sj) {
    int n = SUBTILE_DIV;
    float ax = (float)TILE_HALF_W / (float)n;
    float ay = (float)TILE_HALF_H / (float)n;
    float dx = (float)(x - ctr.x);
    float dy = (float)(y - (ctr.y - TILE_HALF_H));
    float fsi = 0.5f * (dx / ax + dy / ay);
    float fsj = 0.5f * (dy / ay - dx / ax);
    si = (int)std::floor (fsi);
    sj = (int)std::floor (fsj);
    if (si < 0) si = 0;
    if (sj < 0) sj = 0;
    if (si >= n) si = n - 1;
    if (sj >= n) sj = n - 1;
}

static bool farm_ok (const DiaSiSj &blit, const DiaGrid &grid, const SubGrid &sg,
                     int tr, int tc, int si, int sj, int x, int y) {
    int sr = tr * SUBTILE_DIV + si, sc = tc * SUBTILE_DIV + sj;
    int id = sg.get (sr, sc);
    if (ter_any_water (id)) return false;
    bool sand = sg.beach (sr, sc) && id == TER_DESERT;
    uint8_t tip = blit.tip_px (sg, grid, tr, tc, si, sj, x, y);
    if (sand) {
        if (!tip) return false;
        int oid = sg.oth (sr, sc, tip);
        return oid >= 0 && !ter_any_water (oid) && oid != TER_DESERT;
    }
    if (tip && ter_any_water (sg.oth (sr, sc, tip))) return false;
    return true;
}

static void blit_key (Img &dst, const ImgA &src, Vx ctr, int x0, int y0, int opaque,
                      const DiaSiSj &blit, const DiaGrid &grid, const SubGrid &sg, int tr, int tc) {
    for (int y = 0; y < src.h; y++) {
        int dy = y0 + y;
        if (dy < 0 || dy >= dst.h) continue;
        for (int x = 0; x < src.w; x++) {
            int dx = x0 + x;
            if (dx < 0 || dx >= dst.w) continue;
            size_t pi = ((size_t)y * src.w + x) * 4;
            uint8_t r = src.px[pi], g = src.px[pi + 1], b = src.px[pi + 2], a = src.px[pi + 3];
            if (a == 0 || (r == KEY_R && g == KEY_G && b == KEY_B)) continue;
            int si, sj;
            farm_home (ctr, dx, dy, si, sj);
            if (!farm_ok (blit, grid, sg, tr, tc, si, sj, dx, dy)) continue;
            int aa = (a * opaque) / 100;
            if (aa <= 0) continue;
            size_t di = ((size_t)dy * dst.w + dx) * 3;
            int inv = 255 - aa;
            dst.px[di] = (uint8_t)((dst.px[di] * inv + r * aa) / 255);
            dst.px[di + 1] = (uint8_t)((dst.px[di + 1] * inv + g * aa) / 255);
            dst.px[di + 2] = (uint8_t)((dst.px[di + 2] * inv + b * aa) / 255);
        }
    }
}

static bool sub_forest_ok (const SubGrid &sg, int sr, int sc) {
    int id = sg.get (sr, sc);
    if (ter_any_water (id)) return false;
    if (sg.beach (sr, sc) && id == TER_DESERT) return false;
    uint8_t corn = sg.corn (sr, sc);
    if (!corn) return true;
    if ((corn & CORN_N) && ter_any_water (sg.oth (sr, sc, CORN_N))) return false;
    if ((corn & CORN_E) && ter_any_water (sg.oth (sr, sc, CORN_E))) return false;
    if ((corn & CORN_S) && ter_any_water (sg.oth (sr, sc, CORN_S))) return false;
    if ((corn & CORN_W) && ter_any_water (sg.oth (sr, sc, CORN_W))) return false;
    return true;
}

static void blit_spr (Img &dst, const ImgA &src, int x0, int y0, int opaque) {
    for (int y = 0; y < src.h; y++) {
        int dy = y0 + y;
        if (dy < 0 || dy >= dst.h) continue;
        for (int x = 0; x < src.w; x++) {
            int dx = x0 + x;
            if (dx < 0 || dx >= dst.w) continue;
            size_t pi = ((size_t)y * src.w + x) * 4;
            uint8_t r = src.px[pi], g = src.px[pi + 1], b = src.px[pi + 2], a = src.px[pi + 3];
            if (a == 0 || (r == KEY_R && g == KEY_G && b == KEY_B)) continue;
            int aa = (a * opaque) / 100;
            if (aa <= 0) continue;
            size_t di = ((size_t)dy * dst.w + dx) * 3;
            int inv = 255 - aa;
            dst.px[di] = (uint8_t)((dst.px[di] * inv + r * aa) / 255);
            dst.px[di + 1] = (uint8_t)((dst.px[di + 1] * inv + g * aa) / 255);
            dst.px[di + 2] = (uint8_t)((dst.px[di + 2] * inv + b * aa) / 255);
        }
    }
}

static bool ld_forest (std::vector<ImgA> &out) {
    const char *tag = (FOREST_KIND == FOREST_PINE) ? "pine" : "broadleaf";
    out.clear ();
    out.resize ((size_t)FOREST_N);
    char path[512];
    for (int i = 0; i < FOREST_N; i++) {
        std::snprintf (path, sizeof (path), "%s/%s_%02d.png", G_FOREST, tag, i + 1);
        if (!ld_png_rgba (path, out[(size_t)i])) {
            std::fprintf (stderr, "failed to load forest: %s\n", path);
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - Window score -
//================================================================================================================================

struct WinCand {
    int r0;
    int c0;
    int farms;
    int cols;
    int land;
};

static bool win_ok (const DiaSaveMap &map, int r0, int c0, int win, WinCand &out) {
    int land = 0, farms = 0, seen[TERRAIN_N] = {};
    for (int r = 0; r < win; r++) {
        for (int c = 0; c < win; c++) {
            int id = map.ter (r0 + r, c0 + c);
            if (id < 0 || id >= TERRAIN_N) continue;
            seen[id] = 1;
            if (!ter_any_water (id)) land++;
            if (map.farm (r0 + r, c0 + c)) farms++;
        }
    }
    int n_col = 0;
    for (int i = 0; i < TERRAIN_N; i++) n_col += seen[i];
    if (farms < FARM_MIN) return false;
    if (((double)land / (double)(win * win)) < LAND_MIN) return false;
    if (n_col < COLOR_MIN) return false;
    out.r0 = r0; out.c0 = c0; out.farms = farms; out.cols = n_col; out.land = land;
    return true;
}

static bool better (const WinCand &a, const WinCand &b) {
    if (a.farms != b.farms) return a.farms > b.farms;
    if (a.cols != b.cols) return a.cols > b.cols;
    return a.land > b.land;
}

static uint32_t farm_h (int r, int c) {
    uint32_t x = (uint32_t)r * 374761393u + (uint32_t)c * 668265263u;
    x = (x ^ (x >> 13)) * 1274126177u;
    return x ^ (x >> 16);
}

static bool is_png (const char *name) {
    size_t n = std::strlen (name);
    if (n < 4) return false;
    const char *e = name + n - 4;
    return (e[0] == '.' &&
            (e[1] == 'p' || e[1] == 'P') &&
            (e[2] == 'n' || e[2] == 'N') &&
            (e[3] == 'g' || e[3] == 'G'));
}

static bool ld_farm_set (const char *dir, std::vector<ImgA> &out) {
    DIR *d = opendir (dir);
    if (!d) return false;
    std::vector<std::string> names;
    while (dirent *ent = readdir (d)) {
        if (ent->d_name[0] == '.') continue;
        if (!is_png (ent->d_name)) continue;
        names.push_back (ent->d_name);
    }
    closedir (d);
    std::sort (names.begin (), names.end ());
    out.clear ();
    out.resize (names.size ());
    for (size_t i = 0; i < names.size (); i++) {
        std::string path = std::string (dir) + "/" + names[i];
        if (!ld_png_rgba (path.c_str (), out[i])) {
            std::fprintf (stderr, "failed to load farm: %s\n", path.c_str ());
            return false;
        }
    }
    return !out.empty ();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *save_path = G_SAVE;
    const char *out_dir = G_OUT;
    const char *dat_dir = G_DAT;
    const char *farm_root = G_FARM;
    const char *farm_set = G_SET;
    const char *root = G_ROOT;
    const char *super_path = G_SUPER;
    const char *super_png = G_SUPER_PNG;
    int win = WIN, save_max = SAVE_MAX;
    SplatMode splat = SPLAT_OTHER;
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
        if (pos == 0) farm_set = argv[i];
        else if (pos == 1) save_path = argv[i];
        else if (pos == 2) out_dir = argv[i];
        else if (pos == 3) win = std::atoi (argv[i]);
        else if (pos == 4) save_max = std::atoi (argv[i]);
        pos++;
    }
    if (win <= 0 || save_max <= 0) {
        std::fprintf (stderr,
            "usage: %s [--other|--red|--none] [--N] [--E] [--S] [--W] [set] [save.bin] [out_dir] [win] [save_max]\n",
            argv[0]);
        return 1;
    }
    if (!tip_filt) tips = CORN_N | CORN_E | CORN_S | CORN_W;
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") ||
        !mkdir_p ("/home/w/Projects/simple-map-gen/blending/mk7-dia") ||
        !mkdir_p (out_dir)) return 1;

    DiaSaveMap smap;
    if (!smap.ld (save_path)) {
        std::fprintf (stderr, "failed to load TILS: %s\n", save_path);
        return 1;
    }
    int full_w = smap.w (), full_h = smap.h ();
    if (full_w % win || full_h % win) {
        std::fprintf (stderr, "map %dx%d not divisible by win=%d\n", full_w, full_h, win);
        return 1;
    }
    std::printf ("TILS %s  %dx%d\n", save_path, full_w, full_h);

    char path[512];
    std::snprintf (path, sizeof (path), "%s/%s", farm_root, farm_set);
    std::vector<ImgA> farms;
    if (!ld_farm_set (path, farms)) {
        std::fprintf (stderr, "failed to load farm set: %s\n", path);
        return 1;
    }
    std::printf ("farm set %s  n=%zu\n", farm_set, farms.size ());

    std::vector<ImgA> trees;
    if (!ld_forest (trees)) return 1;
    const char *tree_tag = (FOREST_KIND == FOREST_PINE) ? "pine" : "broadleaf";
    std::printf ("forest %s  n=%zu\n", tree_tag, trees.size ());

    DiaSiSj blit;
    if (!blit.ld (super_path)) {
        std::fprintf (stderr, "failed to load super mask: %s\n", super_path);
        return 1;
    }
    if (!blit.ld_png (super_png))
        std::fprintf (stderr, "warning: no stamp png (%s); solid TER_COL shading\n", super_png);
    if (!blit.ld_splat (root)) {
        std::fprintf (stderr, "failed to load splat banks under: %s\n", root);
        return 1;
    }
    blit.set_splat (splat);
    blit.set_tips (tips);

    DiaEdge edge;
    if (!edge.ld (super_path) || !edge.ld_splat (root)) {
        std::fprintf (stderr, "failed to load edge stamp/splat\n");
        return 1;
    }
    edge.set_tips (tips);
    edge.prep ();

    const char *splat_name = "other";
    if (splat == SPLAT_RED) splat_name = "red";
    else if (splat == SPLAT_NONE) splat_name = "none";
    std::printf ("farm overlay  stamp %dx%d  splat=%s  rim AA  tex=dat15\n",
                 blit.sw (), blit.sh (), splat_name);

    DiaDat15 bank_desert, bank_plains, bank_grass, bank_blacksoil;
    DiaDat15 bank_ocean, bank_sea, bank_coast;
    std::snprintf (path, sizeof (path), "%s/tile_desert.dat15", dat_dir);
    if (!bank_desert.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_plains.dat15", dat_dir);
    if (!bank_plains.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_grassland.dat15", dat_dir);
    if (!bank_grass.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_blacksoil.dat15", dat_dir);
    if (!bank_blacksoil.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_ocean.dat15", dat_dir);
    if (!bank_ocean.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_sea.dat15", dat_dir);
    if (!bank_sea.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_coast.dat15", dat_dir);
    if (!bank_coast.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }

    const DiaDat15 *banks[TERRAIN_N] = {
        &bank_desert, &bank_plains, &bank_grass, &bank_blacksoil,
        &bank_ocean, &bank_sea, &bank_coast, &bank_sea, &bank_coast
    };

    PatSet pats;
    if (!pats.ld (root)) {
        std::fprintf (stderr, "failed to load pattern banks under: %s\n", root);
        return 1;
    }

    std::vector<WinCand> cands;
    for (int ir = 0; ir < full_h / win; ir++) {
        for (int ic = 0; ic < full_w / win; ic++) {
            WinCand wc;
            if (win_ok (smap, ir * win, ic * win, win, wc)) cands.push_back (wc);
        }
    }
    std::sort (cands.begin (), cands.end (), better);
    if ((int)cands.size () > save_max) cands.resize ((size_t)save_max);
    if (cands.empty ()) {
        std::fprintf (stderr, "no suitable %dx%d windows (need farms>=%d)\n", win, win, FARM_MIN);
        return 1;
    }

    using clock = std::chrono::steady_clock;
    for (size_t wi = 0; wi < cands.size (); wi++) {
        int r0 = cands[wi].r0, c0 = cands[wi].c0;
        int er0 = r0 > 0 ? r0 - 1 : r0;
        int ec0 = c0 > 0 ? c0 - 1 : c0;
        int er1 = r0 + win < full_h ? r0 + win + 1 : r0 + win;
        int ec1 = c0 + win < full_w ? c0 + win + 1 : c0 + win;
        int eh = er1 - er0, ew = ec1 - ec0;
        int tr0 = r0 - er0, tc0 = c0 - ec0;

        std::vector<int> halo ((size_t)eh * ew);
        for (int r = 0; r < eh; r++) {
            for (int c = 0; c < ew; c++)
                halo[r * ew + c] = smap.ter (er0 + r, ec0 + c);
        }
        std::vector<int> local ((size_t)win * win);
        for (int r = 0; r < win; r++) {
            for (int c = 0; c < win; c++) local[r * win + c] = smap.ter (r0 + r, c0 + c);
        }

        BlendMap emap;
        if (!emap.ld_ids (eh, ew, halo.data ())) return 1;
        BlendMap map;
        if (!map.ld_ids (win, win, local.data ())) return 1;

        auto t0 = clock::now ();
        SubGrid esg;
        esg.build (emap);
        JuncRun run;
        run.bind (&pats);
        run.run (emap, esg);
        blit.fill_corn (esg);

        SubGrid sg;
        sg.copy_tiles (esg, tr0, tc0, win, win);

        DiaGrid grid;
        grid.build (map.rows (), map.cols (), MARGIN);
        int dw = grid.img_w (), dh = grid.img_h ();
        Img tex (dw, dh);
        blit.tex (tex.px.data (), dw, dh, map, sg, grid, banks, TERRAIN_N);
        edge.apply (tex.px.data (), dw, dh, map, sg, grid);

        int n_farm = 0, n_tree = 0;
        for (int r = 0; r < win; r++) {
            for (int c = 0; c < win; c++) {
                if (!smap.farm (r0 + r, c0 + c)) continue;
                if (ter_any_water (map.get (r, c))) continue;
                const ImgA &spr = farms[farm_h (r0 + r, c0 + c) % (uint32_t)farms.size ()];
                Vx ctr = grid.center (r, c);
                blit_key (tex, spr, ctr, ctr.x - spr.w / 2, ctr.y - spr.h / 2, FARM_OPAQUE,
                          blit, grid, sg, r, c);
                n_farm++;
            }
        }
        for (int r = 0; r < win; r++) {
            for (int c = win - 1; c >= 0; c--) {
                if (!smap.forest (r0 + r, c0 + c)) continue;
                if (ter_any_water (map.get (r, c))) continue;
                for (int sum = 0; sum <= 2 * (SUBTILE_DIV - 1); sum++) {
                    for (int si = 0; si < SUBTILE_DIV; si++) {
                        int sj = sum - si;
                        if (sj < 0 || sj >= SUBTILE_DIV) continue;
                        int sr = r * SUBTILE_DIV + si, sc = c * SUBTILE_DIV + sj;
                        if (!sub_forest_ok (sg, sr, sc)) continue;
                        if ((farm_h (sr, sc) % 100u) < (uint32_t)FOREST_SKIP_PCT) continue;
                        const ImgA &spr = trees[farm_h (sr + 17, sc + 31) % (uint32_t)trees.size ()];
                        Vx tip[4];
                        grid.sub_dia (r, c, si, sj, tip);
                        blit_spr (tex, spr, tip[2].x - FOREST_OX, tip[2].y - FOREST_OY_S, FOREST_OPAQUE);
                        n_tree++;
                    }
                }
            }
        }
        auto t1 = clock::now ();
        double ms = std::chrono::duration<double, std::milli> (t1 - t0).count ();

        std::snprintf (path, sizeof (path), "%s/farm_%02zu_r%03d_c%03d.png", out_dir, wi, r0, c0);
        if (!save_png (tex, path)) return 1;
        std::printf ("screen %zu  origin=(%d,%d)  farms=%d  forest=%d  cols=%d  halo=%dx%d  img=%dx%d  %.2f ms\n",
                     wi, r0, c0, n_farm, n_tree, cands[wi].cols, eh, ew, dw, dh, ms);
        std::printf ("  wrote %s\n", path);

        Img brd = tex;
        draw_tile_brd (brd, grid, BORDER_A);
        std::snprintf (path, sizeof (path), "%s/farm_%02zu_r%03d_c%03d_brd.png", out_dir, wi, r0, c0);
        if (!save_png (brd, path)) return 1;
        std::printf ("  wrote %s\n", path);
    }

    std::printf ("mk7 dia_farm_tester  screens=%zu  out=%s\n", cands.size (), out_dir);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
