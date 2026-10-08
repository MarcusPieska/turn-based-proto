//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <png.h>

#include "dia_cfg.h"
#include "dia_grid.h"

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/blend_mk2";

//================================================================================================================================
//=> - Image helpers -
//================================================================================================================================

struct Img {
    int w, h;
    std::vector<uint8_t> px;
    Img (int ww, int hh) : w(ww), h(hh), px((size_t)ww * hh * 3, 255) {}
};

static void put (Img &im, int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    int i = (y * im.w + x) * 3;
    im.px[i] = c.r; im.px[i + 1] = c.g; im.px[i + 2] = c.b;
}

static void seg (Img &im, Vx a, Vx b, Rgb c) {
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    int dx = std::abs (x1 - x0), dy = std::abs (y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        put (im, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
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
//=> - Draw tests -
//================================================================================================================================

static void draw_grid_outlines (const DiaGrid &g, Img &im) {
    Rgb ink (20, 20, 20);
    for (int r = 0; r < g.rows (); r++) {
        for (int c = 0; c < g.cols (); c++) {
            Vx d[4];
            g.diamond (r, c, d);
            seg (im, d[0], d[1], ink);
            seg (im, d[1], d[2], ink);
            seg (im, d[2], d[3], ink);
            seg (im, d[3], d[0], ink);
        }
    }
}

static void draw_tile_subtilles (const DiaGrid &g, int tr, int tc, Img &im) {
    Rgb ink (0, 0, 0);
    Rgb junc (255, 0, 0);
    Vx d[4];
    g.diamond (tr, tc, d);
    seg (im, d[0], d[1], ink);
    seg (im, d[1], d[2], ink);
    seg (im, d[2], d[3], ink);
    seg (im, d[3], d[0], ink);

    std::vector<Vx> vx;
    g.sub_vx (tr, tc, vx);
    int n = g.n_sub ();
    for (int sj = 0; sj < n; sj++) {
        for (int si = 0; si < n - 1; si++) {
            seg (im, vx[sj * n + si], vx[sj * n + si + 1], ink);
        }
    }
    for (int si = 0; si < n; si++) {
        for (int sj = 0; sj < n - 1; sj++) {
            seg (im, vx[sj * n + si], vx[(sj + 1) * n + si], ink);
        }
    }
    for (size_t i = 0; i < vx.size (); i++) put (im, vx[i].x, vx[i].y, junc);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    struct stat st;
    if (stat ("/home/w/Projects/simple-map-gen/blending", &st) != 0) mkdir ("/home/w/Projects/simple-map-gen/blending", 0755);
    if (stat (G_OUT, &st) != 0) mkdir (G_OUT, 0755);

    DiaGrid g;
    g.build (GRID_N, GRID_N, 20);
    std::printf ("grid %dx%d canvas %dx%d\n", g.rows (), g.cols (), g.img_w (), g.img_h ());

    char p1[512], p2[512];
    std::snprintf (p1, sizeof (p1), "%s/out_grid_outlines.png", G_OUT);
    std::snprintf (p2, sizeof (p2), "%s/out_tile_subtilles.png", G_OUT);

    Img all (g.img_w (), g.img_h ());
    draw_grid_outlines (g, all);
    if (!save_png (all, p1)) return 1;

    DiaGrid g1;
    g1.build (1, 1, 16);
    Img one (g1.img_w (), g1.img_h ());
    draw_tile_subtilles (g1, 0, 0, one);
    if (!save_png (one, p2)) return 1;

    std::printf ("wrote %s (%dx%d) %s (%dx%d)\n", p1, all.w, all.h, p2, one.w, one.h);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
