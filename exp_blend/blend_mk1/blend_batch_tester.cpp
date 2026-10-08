//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/stat.h>
#include <png.h>

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

void fill_raw (Img &im, const BlendMap &map) {
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

void fill_clip (Img &im, const BlendClip &clip) {
    for (int y = 0; y < clip.h (); y++) {
        for (int x = 0; x < clip.w (); x++) put (im, x, y, TER_COL[clip.get (x, y)]);
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
        const std::vector<Vx> &pts = th.path (p).pts;
        for (size_t i = 1; i < pts.size (); i++) draw_seg (im, pts[i - 1], pts[i], col);
    }
}

bool save_png (const Img &im, const char *path) {
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
//=> - Batch render -
//================================================================================================================================

bool render_map (const char *map_path, const char *stem) {
    BlendMap map;
    if (!map.ld (map_path)) {
        std::fprintf (stderr, "failed to load map: %s\n", map_path);
        return false;
    }

    SubtileMesh mesh;
    mesh.build (map);

    EdgeThread th;
    th.run (map);

    BlendClip clip_leg;
    clip_leg.run (map, th, CLIP_LEGACY);
    BlendClip clip;
    clip.run (map, th, CLIP_FLOOD);

    Img raw (map.px_w (), map.px_h ());
    fill_raw (raw, map);
    std::string raw_path = std::string (stem) + "_raw.png";
    if (!save_png (raw, raw_path.c_str ())) return false;

    Img leg_im (map.px_w (), map.px_h ());
    fill_clip (leg_im, clip_leg);
    std::string leg_path = std::string (stem) + "_clip_legacy.png";
    if (!save_png (leg_im, leg_path.c_str ())) return false;

    Img clip_im (map.px_w (), map.px_h ());
    fill_clip (clip_im, clip);
    std::string clip_path = std::string (stem) + "_clip.png";
    if (!save_png (clip_im, clip_path.c_str ())) return false;

    Img dbg = clip_im;
    draw_vx (dbg, mesh, map);
    draw_threads (dbg, th);
    std::string dbg_path = std::string (stem) + "_debug.png";
    if (!save_png (dbg, dbg_path.c_str ())) return false;

    std::printf ("wrote %s %s %s %s\n", raw_path.c_str (), leg_path.c_str (), clip_path.c_str (), dbg_path.c_str ());
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    struct stat st;
    if (stat ("/home/w/Projects/simple-map-gen/blending", &st) != 0) mkdir ("/home/w/Projects/simple-map-gen/blending", 0755);
    if (stat (G_OUT, &st) != 0) mkdir (G_OUT, 0755);

    char stems[4][512];
    std::snprintf (stems[0], sizeof (stems[0]), "%s/ex_continents", G_OUT);
    std::snprintf (stems[1], sizeof (stems[1]), "%s/ex_stripes", G_OUT);
    std::snprintf (stems[2], sizeof (stems[2]), "%s/ex_checker", G_OUT);
    std::snprintf (stems[3], sizeof (stems[3]), "%s/ex_solos", G_OUT);
    const char *jobs[][2] = {
        { "maps/ex_continents.map", stems[0] },
        { "maps/ex_stripes.map",    stems[1] },
        { "maps/ex_checker.map",    stems[2] },
        { "maps/ex_solos.map",      stems[3] },
    };
    int n = (int)(sizeof (jobs) / sizeof (jobs[0]));
    int rc = 0;
    for (int i = 0; i < n; i++) {
        if (!render_map (jobs[i][0], jobs[i][1])) rc = 1;
    }
    return rc;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
