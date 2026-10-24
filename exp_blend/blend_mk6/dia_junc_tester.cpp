//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_pat.h"
#include "dia_blit.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk6-dia";
static const char *G_ROOT = ".";
static const char *G_TEX = "/home/w/Projects/simple-map-gen/blending/textures_dia";
static const char *TEX_NAME[TERRAIN_N] = {
    "desert", "plains", "grasslands", "blacksoil", "ocean", "sea", "coast", "sea", "coast"
};
static const int MARGIN = 8;

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

static void put (Img &im, int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    int i = (y * im.w + x) * 3;
    im.px[i] = c.r; im.px[i + 1] = c.g; im.px[i + 2] = c.b;
}

static void draw_seg (Img &im, Vx a, Vx b, Rgb col) {
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    int dx = std::abs (x1 - x0), dy = std::abs (y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        put (im, x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

static void draw_tile_grid (Img &im, const BlendMap &map, const DiaGrid &grid) {
    Rgb ink (20, 20, 20);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Vx d[4];
            grid.diamond (r, c, d);
            draw_seg (im, d[0], d[1], ink);
            draw_seg (im, d[1], d[2], ink);
            draw_seg (im, d[2], d[3], ink);
            draw_seg (im, d[3], d[0], ink);
        }
    }
}

static void draw_sub_grid (Img &im, const BlendMap &map, const DiaGrid &grid) {
    Rgb ink (90, 90, 90);
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            for (int sj = 0; sj < SUBTILE_DIV; sj++) {
                for (int si = 0; si < SUBTILE_DIV; si++) {
                    Vx d[4];
                    grid.sub_dia (r, c, si, sj, d);
                    draw_seg (im, d[0], d[1], ink);
                    draw_seg (im, d[1], d[2], ink);
                    draw_seg (im, d[2], d[3], ink);
                    draw_seg (im, d[3], d[0], ink);
                }
            }
        }
    }
}

static bool mkdir_p (const char *path) {
    struct stat st;
    if (stat (path, &st) == 0) return S_ISDIR (st.st_mode);
    return mkdir (path, 0755) == 0;
}

static bool load_png (const char *path, Img &out) {
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
    if (ct == PNG_COLOR_TYPE_GRAY || ct == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb (png);
    if (ct == PNG_COLOR_TYPE_RGB_ALPHA || ct == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_strip_alpha (png);
    png_read_update_info (png, info);
    out = Img (w, h);
    std::vector<png_bytep> rows (h);
    for (int y = 0; y < h; y++) rows[y] = (png_bytep)&out.px[y * w * 3];
    png_read_image (png, rows.data ());
    png_destroy_read_struct (&png, &info, NULL);
    fclose (f);
    return true;
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
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *map_path = "maps/ex_continents.map";
    const char *stem = "dia_junc";
    const char *out_dir = G_OUT;
    const char *root = G_ROOT;
    const char *tex_dir = G_TEX;
    if (argc > 1) map_path = argv[1];
    if (argc > 2) stem = argv[2];
    if (argc > 3) out_dir = argv[3];
    if (argc > 4) root = argv[4];
    if (argc > 5) tex_dir = argv[5];
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") || !mkdir_p (out_dir)) {
        std::fprintf (stderr, "failed to create out dir: %s\n", out_dir);
        return 1;
    }

    Img tex[TERRAIN_N];
    DiaTex views[TERRAIN_N];
    for (int i = 0; i < TERRAIN_N; i++) {
        char tpath[512];
        std::snprintf (tpath, sizeof (tpath), "%s/%s.png", tex_dir, TEX_NAME[i]);
        if (!load_png (tpath, tex[i]) || tex[i].w != TILE_SPACE_X || tex[i].h != TILE_H) {
            std::fprintf (stderr, "bad texture %s\n", tpath);
            return 1;
        }
        views[i].px = tex[i].px.data ();
        views[i].w = tex[i].w;
        views[i].h = tex[i].h;
    }

    BlendMap map;
    if (!map.ld (map_path)) {
        std::fprintf (stderr, "failed to load map: %s\n", map_path);
        return 1;
    }

    PatSet pats;
    if (!pats.ld (root)) {
        std::fprintf (stderr, "failed to load pattern banks under: %s\n", root);
        return 1;
    }

    DiaGrid grid;
    grid.build (map.rows (), map.cols (), MARGIN);

    SubGrid sg;
    sg.build (map);

    DiaBlit blit;
    blit.bind (views, TERRAIN_N);

    Img raw (grid.img_w (), grid.img_h ());
    blit.run (raw.px.data (), raw.w, raw.h, map, sg, grid);

    JuncRun run;
    run.bind (&pats);
    run.run (map, sg);

    char path[512];
    std::snprintf (path, sizeof (path), "%s/%s_raw.png", out_dir, stem);
    if (!save_png (raw, path)) return 1;

    Img fill (grid.img_w (), grid.img_h ());
    blit.run (fill.px.data (), fill.w, fill.h, map, sg, grid);
    std::snprintf (path, sizeof (path), "%s/%s_fill.png", out_dir, stem);
    if (!save_png (fill, path)) return 1;

    Img grids = fill;
    draw_sub_grid (grids, map, grid);
    draw_tile_grid (grids, map, grid);
    std::snprintf (path, sizeof (path), "%s/%s_grids.png", out_dir, stem);
    if (!save_png (grids, path)) return 1;

    std::printf ("mk6 dia_junc  map=%s  junc=%d  img=%dx%d  tex=%s  banks=c%d/h%d/t%d/d%d/q%d\n",
                 map_path, run.n_junc (), grid.img_w (), grid.img_h (), tex_dir,
                 pats.bank (PAT_CORNER).n (), pats.bank (PAT_HSPLIT).n (),
                 pats.bank (PAT_TCROSS).n (), pats.bank (PAT_DIAG).n (),
                 pats.bank (PAT_QUAD).n ());
    std::printf ("junc select+tile  min=%.2f  max=%.2f  avg=%.2f  us\n",
                 run.t_min_us (), run.t_max_us (), run.t_avg_us ());
    std::printf ("wrote %s/%s_{raw,fill,grids}.png\n", out_dir, stem);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
