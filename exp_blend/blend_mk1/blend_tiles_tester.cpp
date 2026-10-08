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

#include "blend_cfg.h"
#include "blend_core.h"
#include "blend_tile.h"
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
static const char *G_OUT  = "/home/w/Projects/simple-map-gen/blending/p1-seed-43";
static const char *G_TEX  = "/home/w/Projects/simple-map-gen/blending/textures";
static const int WIN = 20;
static const int SAVE_MAX = 100;
static const double LAND_MIN = 0.50;
static const int COLOR_MIN = 5;

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
//=> - Texture views for BlendTile -
//================================================================================================================================

static void fill_views (TileTex *out, const Img *tex, int n) {
    for (int i = 0; i < n; i++) {
        out[i].px = tex[i].px.data ();
        out[i].w = tex[i].w;
        out[i].h = tex[i].h;
    }
}

static void fill_from_clip (Img &im, const BlendClip &clip, const TileTex *tex, int n_tex) {
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            int id = clip.get (x, y);
            if (id < 0 || id >= n_tex) id = BC_PLAINS;
            const TileTex &t = tex[id];
            int lx = x % TILE_PX, ly = y % TILE_PX;
            int si = (ly * t.w + lx) * 3;
            int di = (y * im.w + x) * 3;
            im.px[di] = t.px[si];
            im.px[di + 1] = t.px[si + 1];
            im.px[di + 2] = t.px[si + 2];
        }
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
    BlendTile tiler;
    BlendClip clip;
    TileTex views[BC_N];
    fill_views (views, tex, BC_N);
    Img scratch;

    for (size_t i = 0; i < wins.size (); i++) {
        int r0 = wins[i].first, c0 = wins[i].second;
        if (!map.from_win (bc.data (), full_w, full_h, r0, c0, WIN)) return false;
        Img im (map.px_w (), map.px_h ());

        auto t0 = std::chrono::steady_clock::now ();
        th.run (map);
        double th_ms = ms_since (t0);

        double own_ms = 0, fill_ms = 0, blit_ms = 0;
        if (method == M_PIXEL) {
            t0 = std::chrono::steady_clock::now ();
            clip.run (map, th, CLIP_FLOOD);
            own_ms = ms_since (t0);
            t0 = std::chrono::steady_clock::now ();
            fill_from_clip (im, clip, views, BC_N);
            fill_ms = ms_since (t0);
        } else if (method == M_SEAM) {
            t0 = std::chrono::steady_clock::now ();
            tiler.compose (im.px.data (), im.w, im.h, map, th, views, BC_N, BC_PLAINS, false);
            fill_ms = ms_since (t0);
        } else if (method == M_SEAM_OPT) {
            t0 = std::chrono::steady_clock::now ();
            tiler.compose (im.px.data (), im.w, im.h, map, th, views, BC_N, BC_PLAINS, true);
            fill_ms = ms_since (t0);
        } else {
            t0 = std::chrono::steady_clock::now ();
            tiler.compose (im.px.data (), im.w, im.h, map, th, views, BC_N, BC_PLAINS, true);
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

        a_th.add (th_ms);
        a_own.add (own_ms);
        a_fill.add (fill_ms);
        a_blit.add (blit_ms);
        a_save.add (save_ms);
        a_pipe.add (th_ms + own_ms + fill_ms + blit_ms);
    }

    std::printf ("\n=== %d %s (%zu windows) -> %s ===\n", idx, M_NAME[method], wins.size (), out_dir);
    a_th.print ("thread");
    if (method == M_PIXEL) { a_own.print ("flood_px"); a_fill.print ("fill_tex"); }
    else if (method == M_SEAM) a_fill.print ("compose");
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
        if (!load_png (path, tex[i]) || tex[i].w != TILE_PX || tex[i].h != TILE_PX) {
            std::fprintf (stderr, "bad texture %s\n", path);
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
