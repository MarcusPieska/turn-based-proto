//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
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

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_pat.h"
#include "dia_blit.h"
#include "dia_dat15.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const char *G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk6-dia-game";
static const char *G_DAT = "/home/w/Projects/rts-proto/game_engine_dev/game_view_mk1";
static const char *G_ROOT = ".";
static const int TEX_W = TILE_SPACE_X + 1;
static const int TEX_H = TILE_H + 1;
static const int WIN = 20;
static const int SAVE_MAX = 8;
static const int MARGIN = 8;
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

static bool mkdir_p (const char *path) {
    struct stat st;
    if (stat (path, &st) == 0) return S_ISDIR (st.st_mode);
    return mkdir (path, 0755) == 0;
}

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
    const char *dat_dir = G_DAT;
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

    DiaDat15 bank_desert, bank_plains, bank_grass, bank_tundra, bank_ocean;
    char path[512];
    std::snprintf (path, sizeof (path), "%s/tile_desert.dat15", dat_dir);
    if (!bank_desert.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_plains.dat15", dat_dir);
    if (!bank_plains.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_grassland.dat15", dat_dir);
    if (!bank_grass.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_tundra.dat15", dat_dir);
    if (!bank_tundra.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_ocean.dat15", dat_dir);
    if (!bank_ocean.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }

    const DiaDat15 *banks[TERRAIN_N] = {
        &bank_desert, &bank_plains, &bank_grass, &bank_tundra,
        &bank_ocean, &bank_ocean, &bank_ocean, &bank_ocean, &bank_ocean
    };
    std::printf ("dat15 banks %dx%d cells desert=%dx%d plains=%dx%d grass=%dx%d tundra=%dx%d ocean=%dx%d\n",
                 TEX_W, TEX_H,
                 bank_desert.n_rows (), bank_desert.n_cols (),
                 bank_plains.n_rows (), bank_plains.n_cols (),
                 bank_grass.n_rows (), bank_grass.n_cols (),
                 bank_tundra.n_rows (), bank_tundra.n_cols (),
                 bank_ocean.n_rows (), bank_ocean.n_cols ());

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

    DiaBlit blit;
    blit.bind_banks (banks, TERRAIN_N);

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
        DiaGrid grid;
        grid.build (map.rows (), map.cols (), MARGIN);
        SubGrid sg;
        sg.build (map);
        JuncRun run;
        run.bind (&pats);
        run.run (map, sg);
        Img fill (grid.img_w (), grid.img_h ());
        blit.run (fill.px.data (), fill.w, fill.h, map, sg, grid);
        auto t1 = clock::now ();
        double ms = std::chrono::duration<double, std::milli> (t1 - t0).count ();
        if (wi == 0) { tmin = ms; tmax = ms; }
        else {
            if (ms < tmin) tmin = ms;
            if (ms > tmax) tmax = ms;
        }
        tsum += ms;

        std::snprintf (path, sizeof (path), "%s/game_scr_%02zu_r%03d_c%03d.png", out_dir, wi, r0, c0);
        if (!save_png (fill, path)) return 1;
        Img gridded = fill;
        draw_tile_grid (gridded, map, grid);
        std::snprintf (path, sizeof (path), "%s/game_scr_%02zu_r%03d_c%03d_grid.png", out_dir, wi, r0, c0);
        if (!save_png (gridded, path)) return 1;
        std::printf ("screen %zu  origin=(%d,%d)  img=%dx%d  junc=%d  tile=%.2f ms\n",
                     wi, r0, c0, fill.w, fill.h, run.n_junc (), ms);
    }

    std::printf ("mk6 dia_game  terr=%s  win=%d  screens=%zu\n", terr_path, win, wins.size ());
    std::printf ("screen tile (build+junc+blit)  min=%.2f  max=%.2f  avg=%.2f  ms\n",
                 tmin, tmax, tsum / (double)wins.size ());
    std::printf ("wrote %s/game_scr_*.png and *_grid.png\n", out_dir);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
