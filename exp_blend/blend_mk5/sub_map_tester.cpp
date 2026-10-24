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
#include <sys/stat.h>
#include <sys/types.h>
#include <png.h>

#include "game_map_defs.h"
#include "map_loader.h"
#include "map_terrain_data.h"

#include "sub_cfg.h"
#include "sub_map.h"
#include "sub_pat.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const char *G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk5-sub";
static const char *G_TEX = "/home/w/Projects/simple-map-gen/blending/textures";
static const char *G_ROOT = ".";
static const char *TEX_NAME[TERRAIN_N] = {
    "desert", "plains", "grasslands", "blacksoil", "ocean", "sea", "coast", "sea", "coast"
};
static const int WIN = 20;
static const int SAVE_MAX = 8;
static const double LAND_MIN = 0.50;
static const int COLOR_MIN = 4;

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

static Rgb sample (const Img &tex, int lx, int ly) {
    if (tex.w <= 0 || tex.h <= 0) return Rgb (255, 0, 255);
    int b = 2;
    int iw = tex.w - 2 * b, ih = tex.h - 2 * b;
    if (iw <= 0 || ih <= 0) { b = 0; iw = tex.w; ih = tex.h; }
    if (lx < 0) lx = 0;
    if (ly < 0) ly = 0;
    if (lx >= tex.w) lx = tex.w - 1;
    if (ly >= tex.h) ly = tex.h - 1;
    if (b > 0) {
        lx = b + (lx * iw) / tex.w;
        ly = b + (ly * ih) / tex.h;
    }
    int i = (ly * tex.w + lx) * 3;
    return Rgb (tex.px[i], tex.px[i + 1], tex.px[i + 2]);
}

static void blit_subs (Img &im, const SubGrid &sg, const Img *tex) {
    for (int sr = 0; sr < sg.srows (); sr++) {
        for (int sc = 0; sc < sg.scols (); sc++) {
            int id = sg.get (sr, sc);
            if (id < 0 || id >= TERRAIN_N) id = 0;
            int li = sr % SUBTILE_DIV, lj = sc % SUBTILE_DIV;
            int lx0 = lj * SUBTILE_PX, ly0 = li * SUBTILE_PX;
            Vx o = sg.origin (sr, sc);
            for (int y = 0; y < SUBTILE_PX; y++) {
                for (int x = 0; x < SUBTILE_PX; x++) {
                    put (im, o.x + x, o.y + y, sample (tex[id], lx0 + x, ly0 + y));
                }
            }
        }
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
//=> - Map classify -
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

static int clim_to_ter (u8 clim) {
    if (clim == CLIMATE_DESERT) return TER_DESERT;
    if (clim == CLIMATE_PLAINS) return TER_PLAINS;
    if (clim == CLIMATE_GRASSLAND) return TER_GRASSLANDS;
    if (clim == CLIMATE_BLACK_SOIL) return TER_BLACKSOIL;
    return TER_PLAINS;
}

static int tile_to_ter (u8 terr, u8 clim) {
    if (terr == TERR_OCEAN[0]) return TER_OCEAN;
    if (terr == TERR_SEA[0]) return TER_SEA;
    if (terr == TERR_COASTAL[0]) return TER_COAST;
    if (terr == TERR_INLAND_SEA[0]) return TER_INLAND_SEA;
    if (terr == TERR_INLAND_LAKE[0]) return TER_INLAND_LAKE;
    return clim_to_ter (clim);
}

static bool is_water (int id) {
    return ter_any_water (id);
}

static bool win_ok (const int *ids, int full_w, int r0, int c0, int win) {
    int land = 0, seen[TERRAIN_N] = {};
    for (int r = 0; r < win; r++) {
        for (int c = 0; c < win; c++) {
            int id = ids[(r0 + r) * full_w + (c0 + c)];
            if (id < 0 || id >= TERRAIN_N) continue;
            seen[id] = 1;
            if (!is_water (id)) land++;
        }
    }
    int n_col = 0;
    for (int i = 0; i < TERRAIN_N; i++) n_col += seen[i];
    return ((double)land / (double)(win * win)) >= LAND_MIN && n_col >= COLOR_MIN;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char **argv) {
    const char *terr_path = G_TERR;
    const char *clim_path = G_CLIM;
    const char *out_dir = G_OUT;
    const char *tex_dir = G_TEX;
    const char *root = G_ROOT;
    int win = WIN, save_max = SAVE_MAX;
    if (argc > 1) terr_path = argv[1];
    if (argc > 2) clim_path = argv[2];
    if (argc > 3) out_dir = argv[3];
    if (argc > 4) win = std::atoi (argv[4]);
    if (argc > 5) save_max = std::atoi (argv[5]);
    if (win <= 0 || save_max <= 0) {
        std::fprintf (stderr, "usage: %s [terrain] [climate] [out_dir] [win] [save_max]\n", argv[0]);
        return 1;
    }
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") || !mkdir_p (out_dir)) return 1;

    Img tex[TERRAIN_N];
    for (int i = 0; i < TERRAIN_N; i++) {
        char path[512];
        std::snprintf (path, sizeof (path), "%s/%s.png", tex_dir, TEX_NAME[i]);
        if (!load_png (path, tex[i]) || tex[i].w != TILE_PX || tex[i].h != TILE_PX) {
            std::fprintf (stderr, "bad texture %s\n", path);
            return 1;
        }
    }

    PatSet pats;
    if (!pats.ld (root)) {
        std::fprintf (stderr, "failed to load pattern banks under: %s\n", root);
        return 1;
    }

    MapTerrainData terr;
    if (!MapLoader::load_terrain_ppm (terr_path, terr)) {
        std::fprintf (stderr, "failed to load terrain: %s\n", terr_path);
        return 1;
    }
    int full_w = (int)terr.width (), full_h = (int)terr.height ();
    std::vector<uint8_t> clim;
    if (!load_clim (clim_path, full_w, full_h, clim)) {
        std::fprintf (stderr, "failed to load climate: %s\n", clim_path);
        return 1;
    }
    std::vector<int> ids ((size_t)full_w * full_h);
    for (int i = 0; i < full_w * full_h; i++) ids[i] = tile_to_ter (terr.data ()[i], clim[i]);
    if (full_w % win || full_h % win) {
        std::fprintf (stderr, "map %dx%d not divisible by win=%d\n", full_w, full_h, win);
        return 1;
    }

    std::vector<std::pair<int, int>> wins;
    for (int ir = 0; ir < full_h / win && (int)wins.size () < save_max; ir++) {
        for (int ic = 0; ic < full_w / win && (int)wins.size () < save_max; ic++) {
            int r0 = ir * win, c0 = ic * win;
            if (win_ok (ids.data (), full_w, r0, c0, win)) wins.push_back ({ r0, c0 });
        }
    }
    if (wins.empty ()) {
        std::fprintf (stderr, "no suitable %dx%d windows\n", win, win);
        return 1;
    }

    double tmin = 0, tmax = 0, tsum = 0;
    using clock = std::chrono::steady_clock;
    for (size_t wi = 0; wi < wins.size (); wi++) {
        int r0 = wins[wi].first, c0 = wins[wi].second;
        std::vector<int> local ((size_t)win * win);
        for (int r = 0; r < win; r++) {
            for (int c = 0; c < win; c++) local[r * win + c] = ids[(r0 + r) * full_w + (c0 + c)];
        }

        BlendMap map;
        if (!map.ld_ids (win, win, local.data ())) return 1;

        auto t0 = clock::now ();
        SubGrid sg;
        sg.build (map);
        JuncRun run;
        run.bind (&pats);
        run.run (map, sg);
        Img fill (sg.px_w (), sg.px_h ());
        blit_subs (fill, sg, tex);
        auto t1 = clock::now ();
        double ms = std::chrono::duration<double, std::milli> (t1 - t0).count ();
        if (wi == 0) { tmin = ms; tmax = ms; }
        else {
            if (ms < tmin) tmin = ms;
            if (ms > tmax) tmax = ms;
        }
        tsum += ms;

        char path[512];
        std::snprintf (path, sizeof (path), "%s/map_scr_%02zu_r%03d_c%03d.png", out_dir, wi, r0, c0);
        if (!save_png (fill, path)) return 1;
        Img gridded = fill;
        draw_tile_grid (gridded, map);
        std::snprintf (path, sizeof (path), "%s/map_scr_%02zu_r%03d_c%03d_grid.png", out_dir, wi, r0, c0);
        if (!save_png (gridded, path)) return 1;
        std::printf ("screen %zu  origin=(%d,%d)  img=%dx%d  junc=%d  tile=%.2f ms\n",
                     wi, r0, c0, fill.w, fill.h, run.n_junc (), ms);
    }

    std::printf ("mk5 sub_map  terr=%s  win=%d  screens=%zu\n", terr_path, win, wins.size ());
    std::printf ("screen tile (build+junc+blit)  min=%.2f  max=%.2f  avg=%.2f  ms\n",
                 tmin, tmax, tsum / (double)wins.size ());
    std::printf ("wrote %s/map_scr_*.png and *_grid.png\n", out_dir);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
