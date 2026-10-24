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

#include "sub_cfg.h"
#include "sub_map.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk5-sub";

//================================================================================================================================
//=> - Image helpers -
//================================================================================================================================

struct Img {
    int w;
    int h;
    std::vector<uint8_t> px;

    Img (int ww, int hh) : w(ww), h(hh), px((size_t)ww * hh * 3, 0) {}
};

static void put (Img &im, int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    int i = (y * im.w + x) * 3;
    im.px[i] = c.r; im.px[i + 1] = c.g; im.px[i + 2] = c.b;
}

static void fill_subs (Img &im, const SubGrid &sg) {
    for (int sr = 0; sr < sg.srows (); sr++) {
        for (int sc = 0; sc < sg.scols (); sc++) {
            int id = sg.get (sr, sc);
            Rgb col = (id >= 0 && id < TERRAIN_N) ? TER_COL[id] : Rgb (255, 0, 255);
            Vx o = sg.origin (sr, sc);
            for (int y = 0; y < SUBTILE_PX; y++) {
                for (int x = 0; x < SUBTILE_PX; x++) put (im, o.x + x, o.y + y, col);
            }
        }
    }
}

static void draw_sub_grid (Img &im, const SubGrid &sg) {
    Rgb ink (90, 90, 90);
    int w = sg.px_w (), h = sg.px_h ();
    for (int sc = 0; sc <= sg.scols (); sc++) {
        int x = sc * SUBTILE_PX;
        if (x >= w) x = w - 1;
        for (int y = 0; y < h; y++) put (im, x, y, ink);
    }
    for (int sr = 0; sr <= sg.srows (); sr++) {
        int y = sr * SUBTILE_PX;
        if (y >= h) y = h - 1;
        for (int x = 0; x < w; x++) put (im, x, y, ink);
    }
}

static void draw_tile_grid (Img &im, const BlendMap &map) {
    Rgb ink (20, 20, 20);
    int w = map.px_w (), h = map.px_h ();
    for (int c = 0; c <= map.cols (); c++) {
        int x = c * TILE_PX;
        if (x >= w) x = w - 1;
        for (int y = 0; y < h; y++) put (im, x, y, ink);
    }
    for (int r = 0; r <= map.rows (); r++) {
        int y = r * TILE_PX;
        if (y >= h) y = h - 1;
        for (int x = 0; x < w; x++) put (im, x, y, ink);
    }
}

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
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *map_path = "maps/ex_continents.map";
    const char *stem = "sub_base";
    const char *out_dir = G_OUT;
    if (argc > 1) map_path = argv[1];
    if (argc > 2) stem = argv[2];
    if (argc > 3) out_dir = argv[3];
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") || !mkdir_p (out_dir)) {
        std::fprintf (stderr, "failed to create out dir: %s\n", out_dir);
        return 1;
    }

    BlendMap map;
    if (!map.ld (map_path)) {
        std::fprintf (stderr, "failed to load map: %s\n", map_path);
        return 1;
    }

    SubGrid sg;
    sg.build (map);

    char path[512];
    Img fill (sg.px_w (), sg.px_h ());
    fill_subs (fill, sg);
    std::snprintf (path, sizeof (path), "%s/%s_fill.png", out_dir, stem);
    if (!save_png (fill, path)) return 1;

    Img subg = fill;
    draw_sub_grid (subg, sg);
    std::snprintf (path, sizeof (path), "%s/%s_subgrid.png", out_dir, stem);
    if (!save_png (subg, path)) return 1;

    Img both = subg;
    draw_tile_grid (both, map);
    std::snprintf (path, sizeof (path), "%s/%s_grids.png", out_dir, stem);
    if (!save_png (both, path)) return 1;

    std::printf ("mk5 sub  tiles=%dx%d  subs=%dx%d  sub_px=%d  map=%s  img=%dx%d\n",
                 map.rows (), map.cols (), sg.srows (), sg.scols (), SUBTILE_PX,
                 map_path, sg.px_w (), sg.px_h ());
    std::printf ("wrote %s/%s_{fill,subgrid,grids}.png\n", out_dir, stem);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
