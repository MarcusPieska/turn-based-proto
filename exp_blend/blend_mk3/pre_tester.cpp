//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#include "pre_cfg.h"
#include "pre_thread.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk3-pre";
static const char *G_TPL = "pre_bank";

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

static void fill_map (Img &im, const BlendMap &map) {
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int id = map.get (r, c);
            Rgb col = (id >= 0 && id < TERRAIN_N) ? TER_COL[id] : Rgb (255, 0, 255);
            int x0 = c * TILE_PX, y0 = r * TILE_PX;
            for (int y = 0; y < TILE_PX; y++) {
                for (int x = 0; x < TILE_PX; x++) put (im, x0 + x, y0 + y, col);
            }
        }
    }
}

static void draw_dot (Img &im, int cx, int cy, int rad, Rgb col) {
    int r2 = rad * rad;
    for (int y = -rad; y <= rad; y++) {
        for (int x = -rad; x <= rad; x++) {
            if (x * x + y * y <= r2) put (im, cx + x, cy + y, col);
        }
    }
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

static void draw_corners (Img &im, const BlendMap &map, const EdgeThread &th) {
    Rgb ink (20, 20, 220);
    for (int vr = 0; vr <= map.rows (); vr++) {
        for (int vc = 0; vc <= map.cols (); vc++) {
            Vx j = th.junc (vr, vc);
            draw_dot (im, j.x, j.y, 5, ink);
            if (th.dup (vr, vc)) {
                Vx j2 = th.junc2 (vr, vc);
                draw_dot (im, j2.x, j2.y, 5, ink);
            }
        }
    }
}

static void draw_thread_pts (Img &im, const EdgeThread &th) {
    Rgb col (180, 30, 40);
    Rgb line (160, 50, 55);
    for (int p = 0; p < th.n_path (); p++) {
        const std::vector<Vx> &pts = th.path (p).pts;
        for (size_t i = 1; i < pts.size (); i++) draw_seg (im, pts[i - 1], pts[i], line);
        for (size_t i = 0; i < pts.size (); i++) draw_dot (im, pts[i].x, pts[i].y, 3, col);
    }
}

static void mark_seg (std::vector<uint8_t> &seam, int w, int h, Vx a, Vx b) {
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    int dx = std::abs (x1 - x0), dy = std::abs (y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        if (x0 >= 0 && y0 >= 0 && x0 < w && y0 < h) seam[y0 * w + x0] = 1;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

static void draw_tile_grid (Img &im, const BlendMap &map) {
    Rgb ink (30, 30, 30);
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

static void fill_threads (Img &im, const BlendMap &map, const EdgeThread &th) {
    int w = map.px_w (), h = map.px_h ();
    std::vector<uint8_t> seam (w * h, 0);
    for (int p = 0; p < th.n_path (); p++) {
        const std::vector<Vx> &pts = th.path (p).pts;
        for (size_t i = 1; i < pts.size (); i++) mark_seg (seam, w, h, pts[i - 1], pts[i]);
    }
    std::vector<int> ter (w * h, -1);
    std::queue<int> q;
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int cx = c * TILE_PX + TILE_PX / 2;
            int cy = r * TILE_PX + TILE_PX / 2;
            int idx = cy * w + cx;
            if (seam[idx]) {
                bool found = false;
                for (int rad = 1; rad < TILE_PX / 2 && !found; rad++) {
                    for (int dy = -rad; dy <= rad && !found; dy++) {
                        for (int dx = -rad; dx <= rad && !found; dx++) {
                            int x = cx + dx, y = cy + dy;
                            if (x < c * TILE_PX || y < r * TILE_PX) continue;
                            if (x >= (c + 1) * TILE_PX || y >= (r + 1) * TILE_PX) continue;
                            int j = y * w + x;
                            if (!seam[j]) { idx = j; found = true; }
                        }
                    }
                }
            }
            if (ter[idx] < 0) {
                ter[idx] = map.get (r, c);
                q.push (idx);
            }
        }
    }
    static const int dx4[4] = { 1, -1, 0, 0 };
    static const int dy4[4] = { 0, 0, 1, -1 };
    while (!q.empty ()) {
        int idx = q.front ();
        q.pop ();
        int x = idx % w, y = idx / w;
        int t = ter[idx];
        for (int k = 0; k < 4; k++) {
            int nx = x + dx4[k], ny = y + dy4[k];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
            int ni = ny * w + nx;
            if (seam[ni] || ter[ni] >= 0) continue;
            ter[ni] = t;
            q.push (ni);
        }
    }
    for (int i = 0; i < w * h; i++) {
        int t = ter[i];
        if (t < 0) {
            int x = i % w, y = i / w;
            t = 0;
            for (int k = 0; k < 4; k++) {
                int nx = x + dx4[k], ny = y + dy4[k];
                if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                int n = ter[ny * w + nx];
                if (n >= 0) { t = n; break; }
            }
        }
        if (t < 0 || t >= TERRAIN_N) t = 0;
        put (im, i % w, i / w, TER_COL[t]);
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
    const char *stem = "pre_tpl";
    const char *out_dir = G_OUT;
    const char *tpl_dir = G_TPL;
    if (argc > 1) map_path = argv[1];
    if (argc > 2) stem = argv[2];
    if (argc > 3) out_dir = argv[3];
    if (argc > 4) tpl_dir = argv[4];
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") || !mkdir_p (out_dir)) {
        std::fprintf (stderr, "failed to create out dir: %s\n", out_dir);
        return 1;
    }

    BlendMap map;
    if (!map.ld (map_path)) {
        std::fprintf (stderr, "failed to load map: %s\n", map_path);
        return 1;
    }

    EdgeThread th;
    if (!th.ld_bank (tpl_dir)) {
        std::fprintf (stderr, "failed to load templates from: %s\n", tpl_dir);
        return 1;
    }

    char path[512];
    Img fill (map.px_w (), map.px_h ());

    th.run_c2c (map, 0);
    fill_threads (fill, map, th);
    std::snprintf (path, sizeof (path), "%s/%s_fill_c2c.png", out_dir, stem);
    if (!save_png (fill, path)) return 1;
    Img grid_c2c = fill;
    draw_tile_grid (grid_c2c, map);
    std::snprintf (path, sizeof (path), "%s/%s_grid_c2c.png", out_dir, stem);
    if (!save_png (grid_c2c, path)) return 1;

    th.run (map);
    fill_threads (fill, map, th);
    std::snprintf (path, sizeof (path), "%s/%s_fill_manual.png", out_dir, stem);
    if (!save_png (fill, path)) return 1;
    Img grid_man = fill;
    draw_tile_grid (grid_man, map);
    std::snprintf (path, sizeof (path), "%s/%s_grid_manual.png", out_dir, stem);
    if (!save_png (grid_man, path)) return 1;

    Img raw (map.px_w (), map.px_h ());
    fill_map (raw, map);
    std::snprintf (path, sizeof (path), "%s/%s_raw.png", out_dir, stem);
    if (!save_png (raw, path)) return 1;

    Img vis = raw;
    draw_corners (vis, map, th);
    draw_thread_pts (vis, th);
    std::snprintf (path, sizeof (path), "%s/%s_fit.png", out_dir, stem);
    if (!save_png (vis, path)) return 1;

    std::printf ("mk3 pre-thread  tpl=%d  paths=%d  map=%s\n", th.n_bank (), th.n_path (), map_path);
    std::printf ("wrote %s/%s_{fill_c2c,fill_manual,grid_c2c,grid_manual,raw,fit}.png (%dx%d)\n",
                 out_dir, stem, map.px_w (), map.px_h ());
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
