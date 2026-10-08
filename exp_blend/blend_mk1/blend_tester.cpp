//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>

#include "blend_cfg.h"
#include "blend_core.h"

static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/blend_mk1";

//================================================================================================================================
//=> - Image helpers -
//================================================================================================================================

struct Img {
    int w;
    int h;
    std::vector<uint8_t> px;

    Img (int ww, int hh) : w(ww), h(hh), px(ww * hh * 3, 0) {}
};

void put (Img &im, int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    int i = (y * im.w + x) * 3;
    im.px[i] = c.r; im.px[i + 1] = c.g; im.px[i + 2] = c.b;
}

Rgb get (const Img &im, int x, int y) {
    int i = (y * im.w + x) * 3;
    return Rgb (im.px[i], im.px[i + 1], im.px[i + 2]);
}

void fill_map (Img &im, const BlendMap &map) {
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            Rgb col = TER_COL[map.get (r, c)];
            int x0 = c * TILE_PX, y0 = r * TILE_PX;
            for (int y = 0; y < TILE_PX; y++) {
                for (int x = 0; x < TILE_PX; x++) put (im, x0 + x, y0 + y, col);
            }
        }
    }
}

void draw_vx (Img &im, const SubtileMesh &mesh, const BlendMap &map) {
    Rgb ink (20, 20, 20);
    int nv = mesh.n_vx ();
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            const Vx *v = mesh.tile_vx (r, c);
            for (int j = 0; j < nv; j++) {
                for (int i = 0; i < nv; i++) {
                    int x = v[j * nv + i].x;
                    int y = v[j * nv + i].y;
                    put (im, x, y, ink);
                    put (im, x + 1, y, ink);
                    put (im, x, y + 1, ink);
                }
            }
        }
    }
}

void draw_seg (Img &im, Vx a, Vx b, Rgb col) {
    int x0 = a.x, y0 = a.y, x1 = b.x, y1 = b.y;
    int dx = std::abs (x1 - x0), dy = std::abs (y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        put (im, x0, y0, col);
        put (im, x0 + 1, y0, col);
        put (im, x0, y0 + 1, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void draw_threads (Img &im, const EdgeThread &th) {
    Rgb col (180, 30, 40);
    for (int p = 0; p < th.n_path (); p++) {
        const std::vector<Vx> &path = th.path (p).pts;
        for (size_t i = 1; i < path.size (); i++) draw_seg (im, path[i - 1], path[i], col);
    }
}

bool save_ppm (const Img &im, const char *path) {
    FILE *f = fopen (path, "wb");
    if (!f) return false;
    fprintf (f, "P6\n%d %d\n255\n", im.w, im.h);
    fwrite (im.px.data (), 1, im.px.size (), f);
    fclose (f);
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *map_path = "maps/ex_continents.map";
    if (argc > 1) map_path = argv[1];

    BlendMap map;
    if (!map.ld (map_path)) {
        std::fprintf (stderr, "failed to load map: %s\n", map_path);
        return 1;
    }

    struct stat st;
    if (stat ("/home/w/Projects/simple-map-gen/blending", &st) != 0) mkdir ("/home/w/Projects/simple-map-gen/blending", 0755);
    if (stat (G_OUT, &st) != 0) mkdir (G_OUT, 0755);

    SubtileMesh mesh;
    mesh.build (map);

    EdgeThread th;
    th.run (map);

    char p1[512], p2[512], p3[512];
    std::snprintf (p1, sizeof (p1), "%s/out_01_unblended.ppm", G_OUT);
    std::snprintf (p2, sizeof (p2), "%s/out_02_subtile_vx.ppm", G_OUT);
    std::snprintf (p3, sizeof (p3), "%s/out_03_threaded.ppm", G_OUT);

    Img base (map.px_w (), map.px_h ());
    fill_map (base, map);
    if (!save_ppm (base, p1)) return 1;

    Img vx_im = base;
    draw_vx (vx_im, mesh, map);
    if (!save_ppm (vx_im, p2)) return 1;

    Img th_im = vx_im;
    draw_threads (th_im, th);
    if (!save_ppm (th_im, p3)) return 1;

    std::printf ("wrote %s %s %s (%dx%d)\n", p1, p2, p3, map.px_w (), map.px_h ());
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
