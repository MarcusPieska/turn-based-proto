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
#include "dia_grid.h"
#include "dia_thread.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk4-dia";
static const char *G_TPL = "pre_bank";
static const int MARGIN = 8;

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

static bool in_dia (int cx, int cy, int x, int y) {
    return std::abs (x - cx) * TILE_HALF_H + std::abs (y - cy) * TILE_HALF_W <= TILE_HALF_W * TILE_HALF_H;
}

static bool tile_owns_px (int cx, int cy, int x, int y, const BlendMap &map, int r, int c) {
    int dx = x - cx, dy = y - cy;
    int s = std::abs (dx) * TILE_HALF_H + std::abs (dy) * TILE_HALF_W;
    int lim = TILE_HALF_W * TILE_HALF_H;
    if (s > lim) return false;
    if (s < lim) return true;
    int id = map.get (r, c);
    if (dx >= 0 && dy <= 0 && c + 1 < map.cols () && map.get (r, c + 1) != id) return false;
    if (dx >= 0 && dy >= 0 && r + 1 < map.rows () && map.get (r + 1, c) != id) return false;
    if (dx <= 0 && dy <= 0 && r > 0 && map.get (r - 1, c) != id) return false;
    if (dx <= 0 && dy >= 0 && c > 0 && map.get (r, c - 1) != id) return false;
    return dx >= 0;
}

static void blit_tiles (Img &im, const BlendMap &map, const DiaGrid &grid) {
    std::memset (im.px.data (), 0, im.px.size ());
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int id = map.get (r, c);
            Rgb col = (id >= 0 && id < TERRAIN_N) ? TER_COL[id] : Rgb (255, 0, 255);
            Vx ctr = grid.center (r, c);
            int x0 = ctr.x - TILE_HALF_W, x1 = ctr.x + TILE_HALF_W;
            int y0 = ctr.y - TILE_HALF_H, y1 = ctr.y + TILE_HALF_H;
            if (x0 < 0) x0 = 0;
            if (y0 < 0) y0 = 0;
            if (x1 >= im.w) x1 = im.w - 1;
            if (y1 >= im.h) y1 = im.h - 1;
            for (int y = y0; y <= y1; y++) {
                for (int x = x0; x <= x1; x++) {
                    if (!tile_owns_px (ctr.x, ctr.y, x, y, map, r, c)) continue;
                    put (im, x, y, col);
                }
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
    for (int jr = -1; jr < map.rows (); jr++) {
        for (int jc = -1; jc < map.cols (); jc++) {
            Vx j = th.junc (jr, jc);
            if (j.x == 0 && j.y == 0) continue;
            draw_dot (im, j.x, j.y, 3, ink);
            if (th.dup (jr, jc)) {
                Vx j2 = th.junc2 (jr, jc);
                draw_dot (im, j2.x, j2.y, 3, ink);
            }
        }
    }
}

static void draw_thread_lines (Img &im, const EdgeThread &th) {
    Rgb line (180, 30, 40);
    for (int p = 0; p < th.n_path (); p++) {
        const std::vector<Vx> &pts = th.path (p).pts;
        for (size_t i = 1; i < pts.size (); i++) draw_seg (im, pts[i - 1], pts[i], line);
    }
}

static void draw_thread_pts (Img &im, const EdgeThread &th) {
    Rgb col (180, 30, 40);
    Rgb line (160, 50, 55);
    for (int p = 0; p < th.n_path (); p++) {
        const std::vector<Vx> &pts = th.path (p).pts;
        for (size_t i = 1; i < pts.size (); i++) draw_seg (im, pts[i - 1], pts[i], line);
        for (size_t i = 0; i < pts.size (); i++) draw_dot (im, pts[i].x, pts[i].y, 2, col);
    }
}

static void draw_tile_grid (Img &im, const BlendMap &map, const DiaGrid &grid) {
    Rgb ink (30, 30, 30);
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

static float cross2 (float ax, float ay, float bx, float by) {
    return ax * by - ay * bx;
}

static void path_point (const std::vector<Vx> &pts, float t, float &ox, float &oy) {
    if (pts.size () < 2) { ox = oy = 0.f; return; }
    float u = t * (float)(pts.size () - 1);
    int i = (int)u;
    if (i < 0) i = 0;
    if (i >= (int)pts.size () - 1) i = (int)pts.size () - 2;
    float f = u - (float)i;
    ox = (1.f - f) * (float)pts[i].x + f * (float)pts[i + 1].x;
    oy = (1.f - f) * (float)pts[i].y + f * (float)pts[i + 1].y;
}

static void clip_one_path (Img &im, const EdgePath &ep, const BlendMap &map, const DiaGrid &grid) {
    if (ep.pts.size () < 2) return;
    Vx ga, gb;
    Vx cta, ctb;
    if (ep.kind == EDGE_NE) {
        Vx dia[4];
        grid.diamond (ep.r, ep.c, dia);
        ga = dia[0]; gb = dia[1];
        cta = grid.center (ep.r, ep.c);
        ctb = grid.center (ep.r, ep.c + 1);
    } else if (ep.kind == EDGE_SE) {
        Vx dia[4];
        grid.diamond (ep.r, ep.c, dia);
        ga = dia[1]; gb = dia[2];
        cta = grid.center (ep.r, ep.c);
        ctb = grid.center (ep.r + 1, ep.c);
    } else {
        ga = ep.pts.front ();
        gb = ep.pts.back ();
        if (map.get (ep.r, ep.c) == ep.ta) {
            cta = grid.center (ep.r, ep.c);
            ctb = grid.center (ep.r + 1, ep.c + 1);
        } else {
            cta = grid.center (ep.r, ep.c + 1);
            ctb = grid.center (ep.r + 1, ep.c);
        }
    }
    int pad = 3 * PULL_UX + 4;
    int x0 = std::min (ga.x, gb.x) - pad;
    int x1 = std::max (ga.x, gb.x) + pad;
    int y0 = std::min (ga.y, gb.y) - pad;
    int y1 = std::max (ga.y, gb.y) + pad;
    for (size_t i = 0; i < ep.pts.size (); i++) {
        x0 = std::min (x0, (int)ep.pts[i].x - pad);
        x1 = std::max (x1, (int)ep.pts[i].x + pad);
        y0 = std::min (y0, (int)ep.pts[i].y - pad);
        y1 = std::max (y1, (int)ep.pts[i].y + pad);
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > im.w) x1 = im.w;
    if (y1 > im.h) y1 = im.h;
    float abx = (float)(gb.x - ga.x);
    float aby = (float)(gb.y - ga.y);
    float ab2 = abx * abx + aby * aby;
    if (ab2 < 1.f) return;
    float ab_len = std::sqrt (ab2);
    int ta = (ep.ta >= 0 && ep.ta < TERRAIN_N) ? ep.ta : 0;
    int tb = (ep.tb >= 0 && ep.tb < TERRAIN_N) ? ep.tb : 0;
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (!in_dia (cta.x, cta.y, x, y) && !in_dia (ctb.x, ctb.y, x, y)) continue;
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float t = ((px - (float)ga.x) * abx + (py - (float)ga.y) * aby) / ab2;
            if (t < 0.f || t > 1.f) continue;
            float qx, qy;
            path_point (ep.pts, t, qx, qy);
            float gx = (1.f - t) * (float)ga.x + t * (float)gb.x;
            float gy = (1.f - t) * (float)ga.y + t * (float)gb.y;
            float c_geo = cross2 (abx, aby, px - gx, py - gy);
            float c_path = cross2 (abx, aby, px - qx, py - qy);
            float dist_geo = std::fabs (c_geo) / ab_len;
            int id = -1;
            if (c_path > 0.f && c_geo <= 0.f) id = ta;
            if (c_path <= 0.f && c_geo > 0.f) id = tb;
            if (id < 0 && dist_geo <= 1.25f) id = (c_path > 0.f) ? ta : tb;
            if (id < 0) continue;
            put (im, x, y, TER_COL[id]);
        }
    }
}

static void compose_tiles (Img &im, const BlendMap &map, const DiaGrid &grid, const EdgeThread &th) {
    blit_tiles (im, map, grid);
    for (int p = 0; p < th.n_path (); p++) clip_one_path (im, th.path (p), map, grid);
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
    const char *stem = "dia_tpl";
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

    DiaGrid grid;
    grid.build (map.rows (), map.cols (), MARGIN);

    EdgeThread th;
    if (!th.ld_bank (tpl_dir)) {
        std::fprintf (stderr, "failed to load templates from: %s\n", tpl_dir);
        return 1;
    }

    char path[512];
    Img fill (grid.img_w (), grid.img_h ());

    th.run (map, grid);
    compose_tiles (fill, map, grid, th);
    std::snprintf (path, sizeof (path), "%s/%s_fill_manual.png", out_dir, stem);
    if (!save_png (fill, path)) return 1;
    Img grid_man = fill;
    draw_tile_grid (grid_man, map, grid);
    std::snprintf (path, sizeof (path), "%s/%s_grid_manual.png", out_dir, stem);
    if (!save_png (grid_man, path)) return 1;
    Img tpl_grid = grid_man;
    draw_thread_lines (tpl_grid, th);
    std::snprintf (path, sizeof (path), "%s/%s_tpl_grid_manual.png", out_dir, stem);
    if (!save_png (tpl_grid, path)) return 1;

    Img raw (grid.img_w (), grid.img_h ());
    blit_tiles (raw, map, grid);
    std::snprintf (path, sizeof (path), "%s/%s_raw.png", out_dir, stem);
    if (!save_png (raw, path)) return 1;

    Img vis = raw;
    draw_corners (vis, map, th);
    draw_thread_pts (vis, th);
    std::snprintf (path, sizeof (path), "%s/%s_fit.png", out_dir, stem);
    if (!save_png (vis, path)) return 1;

    std::printf ("mk4 dia-thread  bank=%d  paths=%d  map=%s  img=%dx%d\n",
                 th.n_bank (), th.n_path (), map_path, grid.img_w (), grid.img_h ());
    std::printf ("wrote %s/%s_{fill_manual,grid_manual,tpl_grid_manual,raw,fit}.png\n", out_dir, stem);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
