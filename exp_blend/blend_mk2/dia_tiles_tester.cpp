//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#include "dia_cfg.h"
#include "dia_grid.h"
#include "dia_blend.h"
#include "game_map_defs.h"
#include "map_loader.h"
#include "map_terrain_data.h"

//================================================================================================================================
//=> - Blend class ids -
//================================================================================================================================

enum BlendClass {
    BC_DESERT = 0, BC_PLAINS = 1, BC_GRASS = 2, BC_BLACK = 3,
    BC_OCEAN = 4, BC_SEA = 5, BC_COAST = 6, BC_N = 7
};

static const char *BC_TEX_NAME[BC_N] = {
    "desert", "plains", "grasslands", "blacksoil", "ocean", "sea", "coast"
};

//================================================================================================================================
//=> - Paths / config -
//================================================================================================================================

static const char *G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const char *G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const char *G_OUT  = "/home/w/Projects/simple-map-gen/blending/p1-seed-43-dia";
static const char *G_TEX  = "/home/w/Projects/simple-map-gen/blending/textures_dia";
static const int WIN = 20;
static const int SAVE_MAX = 2;
static const double LAND_MIN = 0.50;
static const int COLOR_MIN = 5;
static const int GRID_MARGIN = 24;

//================================================================================================================================
//=> - Image / timing helpers -
//================================================================================================================================

struct Img {
    int w, h;
    std::vector<uint8_t> px;
    Img () : w(0), h(0) {}
    Img (int ww, int hh) : w(ww), h(hh), px((size_t)ww * hh * 3, 0) {}
};

struct Acc {
    double sum, mn, mx;
    long long n;
    Acc () : sum(0), mn(1e300), mx(0), n(0) {}
    void add (double v) {
        sum += v;
        if (v < mn) mn = v;
        if (v > mx) mx = v;
        n++;
    }
    void print (const char *name) const {
        if (n <= 0) { std::printf ("  %-12s (no samples)\n", name); return; }
        std::printf ("  %-12s min %8.3f  max %8.3f  avg %8.3f  ms  (n=%lld)\n",
                     name, mn, mx, sum / (double)n, n);
    }
};

static double ms_since (std::chrono::steady_clock::time_point t0) {
    return std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now () - t0).count ();
}

static void put (Img &im, int x, int y, Rgb c) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    int i = (y * im.w + x) * 3;
    im.px[i] = c.r; im.px[i + 1] = c.g; im.px[i + 2] = c.b;
}

static Rgb sample (const Img &tex, int lx, int ly) {
    if (tex.w <= 0 || tex.h <= 0) return Rgb (255, 0, 255);
    if (lx < 0) lx = 0;
    if (ly < 0) ly = 0;
    if (lx >= tex.w) lx = tex.w - 1;
    if (ly >= tex.h) ly = tex.h - 1;
    int i = (ly * tex.w + lx) * 3;
    return Rgb (tex.px[i], tex.px[i + 1], tex.px[i + 2]);
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
//=> - Map load / classify -
//================================================================================================================================

static bool rd_ppm_rgb (const char *path, int *out_w, int *out_h, std::vector<uint8_t> &rgb) {
    FILE *fp = std::fopen (path, "rb");
    if (!fp) return false;
    char magic[3] = {};
    if (std::fscanf (fp, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '6') {
        std::fclose (fp); return false;
    }
    int c = std::fgetc (fp);
    while (c == '#') {
        while (c != '\n' && c != EOF) c = std::fgetc (fp);
        c = std::fgetc (fp);
    }
    std::ungetc (c, fp);
    unsigned wi = 0, hi = 0, maxv = 0;
    if (std::fscanf (fp, "%u %u %u", &wi, &hi, &maxv) != 3 || maxv != 255u) {
        std::fclose (fp); return false;
    }
    c = std::fgetc (fp);
    if (c == EOF) { std::fclose (fp); return false; }
    rgb.resize ((size_t)wi * hi * 3u);
    if (std::fread (rgb.data (), 1, rgb.size (), fp) != rgb.size ()) {
        std::fclose (fp); return false;
    }
    std::fclose (fp);
    *out_w = (int)wi; *out_h = (int)hi;
    return true;
}

static bool load_clim (const char *path, int ew, int eh, std::vector<uint8_t> &out) {
    int w = 0, h = 0;
    std::vector<uint8_t> rgb;
    if (!rd_ppm_rgb (path, &w, &h, rgb) || w != ew || h != eh) return false;
    out.resize ((size_t)w * h);
    for (int i = 0; i < w * h; i++) {
        bool ok = false;
        out[i] = climate_from_rgb (rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2], &ok);
        if (!ok) return false;
    }
    return true;
}

static int clim_to_bc (u8 clim) {
    if (clim == CLIMATE_DESERT) return BC_DESERT;
    if (clim == CLIMATE_PLAINS) return BC_PLAINS;
    if (clim == CLIMATE_GRASSLAND) return BC_GRASS;
    if (clim == CLIMATE_BLACK_SOIL) return BC_BLACK;
    return BC_PLAINS;
}

static int tile_to_bc (u8 terr, u8 clim) {
    if (terr == TERR_OCEAN[0]) return BC_OCEAN;
    if (terr == TERR_SEA[0] || terr == TERR_INLAND_SEA[0]) return BC_SEA;
    if (terr == TERR_COASTAL[0] || terr == TERR_INLAND_LAKE[0]) return BC_COAST;
    return clim_to_bc (clim);
}

static bool is_water (int bc) {
    return bc == BC_OCEAN || bc == BC_SEA || bc == BC_COAST;
}

static bool win_ok (const uint8_t *bc, int full_w, int r0, int c0, int win) {
    int land = 0, seen[BC_N] = {};
    for (int r = 0; r < win; r++) {
        for (int c = 0; c < win; c++) {
            int id = bc[(r0 + r) * full_w + (c0 + c)];
            if (id < 0 || id >= BC_N) continue;
            seen[id] = 1;
            if (!is_water (id)) land++;
        }
    }
    int n_col = 0;
    for (int i = 0; i < BC_N; i++) n_col += seen[i];
    return ((double)land / (double)(win * win)) >= LAND_MIN && n_col >= COLOR_MIN;
}

//================================================================================================================================
//=> - Diamond blit / seam clip -
//================================================================================================================================

static bool in_dia (int cx, int cy, int x, int y) {
    return std::abs (x - cx) * TILE_HALF_H + std::abs (y - cy) * TILE_HALF_W <= TILE_HALF_W * TILE_HALF_H;
}

static bool tile_owns_px (int cx, int cy, int x, int y,
                          const BlendMap &map, int r, int c) {
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

static bool in_dia_tex (int lx, int ly) {
    float cx = 0.5f * (float)(TILE_DRAW_W - 1);
    float cy = 0.5f * (float)(TILE_H - 1);
    float hw = 0.5f * (float)TILE_DRAW_W;
    float hh = 0.5f * (float)TILE_H;
    return std::fabs ((float)lx - cx) * hh + std::fabs ((float)ly - cy) * hw <= hw * hh;
}

static Rgb sample_dia (const Img &tex, int lx, int ly) {
    if (tex.w <= 0 || tex.h <= 0) return Rgb (255, 0, 255);
    if (lx < 0) lx = 0;
    if (ly < 0) ly = 0;
    if (lx >= tex.w) lx = tex.w - 1;
    if (ly >= tex.h) ly = tex.h - 1;
    if (!in_dia_tex (lx, ly)) return sample (tex, tex.w / 2, tex.h / 2);
    return sample (tex, lx, ly);
}

static void write_tex_at (Img &im, const Img &tex, int cx, int cy, int x, int y) {
    int lx = x - (cx - TILE_HALF_W);
    int ly = y - (cy - TILE_HALF_H);
    Rgb c = sample_dia (tex, lx, ly);
    put (im, x, y, c);
}

static void blit_tiles (Img &im, const BlendMap &map, const DiaGrid &grid, const Img *tex) {
    std::memset (im.px.data (), 0, im.px.size ());
    for (int r = 0; r < map.rows (); r++) {
        for (int c = 0; c < map.cols (); c++) {
            int id = map.get (r, c);
            if (id < 0 || id >= BC_N) id = BC_PLAINS;
            const Img &t = tex[id];
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
                    write_tex_at (im, t, ctr.x, ctr.y, x, y);
                }
            }
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

static void clip_one_path (Img &im, const EdgePath &ep, const DiaGrid &grid, const Img *tex, bool block_skip) {
    if (ep.pts.size () < 2) return;
    int pad = (EDGE_MAX_DEV + 1) * (TILE_HALF_W / SUBTILE_DIV) + 3;
    int x0 = std::min (ep.a.x, ep.b.x) - pad;
    int x1 = std::max (ep.a.x, ep.b.x) + pad;
    int y0 = std::min (ep.a.y, ep.b.y) - pad;
    int y1 = std::max (ep.a.y, ep.b.y) + pad;
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

    float abx = (float)(ep.b.x - ep.a.x);
    float aby = (float)(ep.b.y - ep.a.y);
    float ab2 = abx * abx + aby * aby;
    if (ab2 < 1.f) return;

    Vx cta = grid.center (ep.r, ep.c);
    Vx ctb = (ep.kind == EDGE_NE) ? grid.center (ep.r, ep.c + 1) : grid.center (ep.r + 1, ep.c);
    const Img &ta_tex = tex[ep.ta >= 0 && ep.ta < BC_N ? ep.ta : BC_PLAINS];
    const Img &tb_tex = tex[ep.tb >= 0 && ep.tb < BC_N ? ep.tb : BC_PLAINS];

    auto apply_px = [&] (int x, int y) {
        float px = (float)x + 0.5f, py = (float)y + 0.5f;
        float t = ((px - (float)ep.a.x) * abx + (py - (float)ep.a.y) * aby) / ab2;
        if (t < 0.f || t > 1.f) return;
        float qx, qy;
        path_point (ep.pts, t, qx, qy);
        float gx = (1.f - t) * (float)ep.a.x + t * (float)ep.b.x;
        float gy = (1.f - t) * (float)ep.a.y + t * (float)ep.b.y;
        float c_geo = cross2 (abx, aby, px - gx, py - gy);
        float c_path = cross2 (abx, aby, px - qx, py - qy);
        float ab_len = std::sqrt (ab2);
        float dist_geo = std::fabs (c_geo) / ab_len;
        int id = -1;
        if (c_path > 0.f && c_geo <= 0.f) id = ep.ta;
        if (c_path <= 0.f && c_geo > 0.f) id = ep.tb;
        if (id < 0 && dist_geo <= 1.25f) id = (c_path > 0.f) ? ep.ta : ep.tb;
        if (id < 0) return;
        if (id == ep.ta) write_tex_at (im, ta_tex, cta.x, cta.y, x, y);
        else write_tex_at (im, tb_tex, ctb.x, ctb.y, x, y);
    };

    if (!block_skip) {
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++) apply_px (x, y);
        return;
    }

    const int bs = TILE_HALF_W / SUBTILE_DIV;
    for (int by = y0; by < y1; by += bs) {
        int by1 = std::min (by + bs, y1);
        for (int bx = x0; bx < x1; bx += bs) {
            int bx1 = std::min (bx + bs, x1);
            bool maybe = false;
            for (int y = by; y < by1 && !maybe; y += std::max (1, (by1 - by) / 2)) {
                for (int x = bx; x < bx1 && !maybe; x += std::max (1, (bx1 - bx) / 2)) {
                    float px = (float)x + 0.5f, py = (float)y + 0.5f;
                    float t = ((px - (float)ep.a.x) * abx + (py - (float)ep.a.y) * aby) / ab2;
                    if (t < 0.f || t > 1.f) continue;
                    float qx, qy;
                    path_point (ep.pts, t, qx, qy);
                    float gx = (1.f - t) * (float)ep.a.x + t * (float)ep.b.x;
                    float gy = (1.f - t) * (float)ep.a.y + t * (float)ep.b.y;
                    float c_geo = cross2 (abx, aby, px - gx, py - gy);
                    float c_path = cross2 (abx, aby, px - qx, py - qy);
                    float ab_len = std::sqrt (ab2);
                    float dist_geo = std::fabs (c_geo) / ab_len;
                    if ((c_path > 0.f && c_geo <= 0.f) || (c_path <= 0.f && c_geo > 0.f) || dist_geo <= 1.25f) maybe = true;
                }
            }
            if (!maybe) continue;
            for (int y = by; y < by1; y++)
                for (int x = bx; x < bx1; x++) apply_px (x, y);
        }
    }
}

static void clip_seam_band (Img &im, const EdgeThread &th, const DiaGrid &grid, const Img *tex, bool block_skip) {
    for (int p = 0; p < th.n_path (); p++) clip_one_path (im, th.path (p), grid, tex, block_skip);
}

static void compose_seam (Img &im, const BlendMap &map, const DiaGrid &grid, const EdgeThread &th, const Img *tex, bool block_skip) {
    blit_tiles (im, map, grid, tex);
    clip_seam_band (im, th, grid, tex, block_skip);
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

static void draw_threads (Img &im, const EdgeThread &th) {
    Rgb geo (40, 40, 40);
    Rgb ne (220, 40, 40);
    Rgb se (40, 80, 220);
    for (int p = 0; p < th.n_path (); p++) {
        const EdgePath &ep = th.path (p);
        seg (im, ep.a, ep.b, geo);
        Rgb col = (ep.kind == EDGE_NE) ? ne : se;
        for (size_t i = 1; i < ep.pts.size (); i++) seg (im, ep.pts[i - 1], ep.pts[i], col);
    }
}

//================================================================================================================================
//=> - Benchmark runners -
//================================================================================================================================

enum Method { M_PIXEL = 0, M_SEAM = 1, M_SEAM_OPT = 2, M_BAKE = 3, M_N = 4 };

static const char *M_NAME[M_N] = { "tex_pixel", "tex_seam", "tex_seam_opt", "tex_bake" };

static bool run_method (
    Method method,
    int idx,
    const char *out_dir,
    const std::vector<uint8_t> &bc,
    int full_w, int full_h,
    const std::vector<std::pair<int,int>> &wins,
    const Img *tex)
{
    Acc a_th, a_own, a_fill, a_blit, a_save, a_pipe;
    BlendMap map;
    EdgeThread th;
    DiaGrid grid;
    Img scratch;

    for (size_t i = 0; i < wins.size (); i++) {
        int r0 = wins[i].first, c0 = wins[i].second;
        if (!map.from_win (bc.data (), full_w, full_h, r0, c0, WIN)) return false;
        grid.build (map.rows (), map.cols (), GRID_MARGIN);
        Img im (grid.img_w (), grid.img_h ());

        auto t0 = std::chrono::steady_clock::now ();
        th.run (map, grid);
        double th_ms = ms_since (t0);

        double own_ms = 0, fill_ms = 0, blit_ms = 0;
        if (method == M_SEAM) {
            t0 = std::chrono::steady_clock::now ();
            compose_seam (im, map, grid, th, tex, false);
            fill_ms = ms_since (t0);
        } else if (method == M_SEAM_OPT) {
            t0 = std::chrono::steady_clock::now ();
            compose_seam (im, map, grid, th, tex, true);
            fill_ms = ms_since (t0);
        } else {
            t0 = std::chrono::steady_clock::now ();
            compose_seam (im, map, grid, th, tex, true);
            own_ms = ms_since (t0);
            if (scratch.w != im.w || scratch.h != im.h) scratch = Img (im.w, im.h);
            t0 = std::chrono::steady_clock::now ();
            std::memcpy (scratch.px.data (), im.px.data (), im.px.size ());
            blit_ms = ms_since (t0);
        }

        char path[512];
        std::snprintf (path, sizeof (path), "%s/r%03d_c%03d_%d.png", out_dir, r0, c0, idx);
        t0 = std::chrono::steady_clock::now ();
        if (!save_png (method == M_BAKE ? scratch : im, path)) return false;
        double save_ms = ms_since (t0);

        if (method == M_SEAM) {
            Img dbg (grid.img_w (), grid.img_h ());
            blit_tiles (dbg, map, grid, tex);
            draw_threads (dbg, th);
            std::snprintf (path, sizeof (path), "%s/r%03d_c%03d_threads.png", out_dir, r0, c0);
            if (!save_png (dbg, path)) return false;
        }

        a_th.add (th_ms);
        a_own.add (own_ms);
        a_fill.add (fill_ms);
        a_blit.add (blit_ms);
        a_save.add (save_ms);
        a_pipe.add (th_ms + own_ms + fill_ms + blit_ms);
    }

    std::printf ("\n=== %d %s (%zu windows) -> %s ===\n", idx, M_NAME[method], wins.size (), out_dir);
    a_th.print ("thread");
    if (method == M_SEAM) a_fill.print ("compose");
    else if (method == M_SEAM_OPT) a_fill.print ("compose_opt");
    else { a_own.print ("bake_build"); a_blit.print ("bake_blit"); }
    a_save.print ("png_save");
    a_pipe.print ("pipe");
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *terr_path = G_TERR;
    const char *clim_path = G_CLIM;
    const char *out_root = G_OUT;
    const char *tex_dir = G_TEX;
    int win = WIN, save_max = SAVE_MAX;
    if (argc > 1) terr_path = argv[1];
    if (argc > 2) clim_path = argv[2];
    if (argc > 3) out_root = argv[3];
    if (argc > 4) win = std::atoi (argv[4]);
    if (argc > 5) save_max = std::atoi (argv[5]);
    if (win <= 0 || save_max <= 0) {
        std::fprintf (stderr, "usage: %s [terrain] [climate] [out_root] [win] [save_max]\n", argv[0]);
        return 1;
    }
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") || !mkdir_p (out_root)) return 1;

    Img tex[BC_N];
    for (int i = 0; i < BC_N; i++) {
        char path[512];
        std::snprintf (path, sizeof (path), "%s/%s.png", tex_dir, BC_TEX_NAME[i]);
        if (!load_png (path, tex[i]) || tex[i].w != TILE_DRAW_W || tex[i].h != TILE_H) {
            std::fprintf (stderr, "bad texture %s (%dx%d)\n", path, tex[i].w, tex[i].h);
            return 1;
        }
    }

    MapTerrainData terr;
    if (!MapLoader::load_terrain_ppm (terr_path, terr)) return 1;
    int full_w = (int)terr.width (), full_h = (int)terr.height ();
    std::vector<uint8_t> clim;
    if (!load_clim (clim_path, full_w, full_h, clim)) return 1;
    std::vector<uint8_t> bc ((size_t)full_w * full_h);
    for (int i = 0; i < full_w * full_h; i++) bc[i] = (uint8_t)tile_to_bc (terr.data ()[i], clim[i]);
    if (full_w % win || full_h % win) return 1;

    std::vector<std::pair<int,int>> wins;
    for (int ir = 0; ir < full_h / win && (int)wins.size () < save_max; ir++) {
        for (int ic = 0; ic < full_w / win && (int)wins.size () < save_max; ic++) {
            int r0 = ir * win, c0 = ic * win;
            if (win_ok (bc.data (), full_w, r0, c0, win)) wins.push_back ({ r0, c0 });
        }
    }
    std::printf ("selected %zu windows once; running methods into %s\n", wins.size (), out_root);
    std::printf ("suffixes: 0=%s(skip) 1=%s 2=%s 3=%s\n",
                 M_NAME[0], M_NAME[1], M_NAME[2], M_NAME[3]);
    if (wins.empty ()) return 1;

    for (int m = 0; m < M_N; m++) {
        if (m == M_PIXEL) continue;
        if (!run_method ((Method)m, m, out_root, bc, full_w, full_h, wins, tex)) {
            std::fprintf (stderr, "method %s failed\n", M_NAME[m]);
            return 1;
        }
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
