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

#include "dia_cfg.h"
#include "dia_map.h"
#include "dia_grid.h"
#include "dia_pat.h"
#include "dia_dat15.h"
#include "dia_sisj.h"
#include "dia_edge.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char *G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const char *G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const char *G_OUT = "/home/w/Projects/simple-map-gen/blending/mk7-dia/edge_tile";
static const char *G_DAT = "/home/w/Projects/rts-proto/game_engine_dev/game_view_mk1";
static const char *G_ROOT = ".";
static const char *G_SUPER = "dia_submask/super_submask.bin";
static const char *G_SUPER_PNG = "/home/w/Projects/simple-map-gen/blending/mk7-dia/dia_submask/super_submask.png";
static const int TEX_W = TILE_SPACE_X + 1;
static const int TEX_H = TILE_H + 1;
static const int WIN = 20;
static const int SAVE_MAX = 4;
static const int MARGIN = 8;
static const double LAND_MIN = 0.50;
static const int COLOR_MIN = 4;
static const int BORDER_A = 96;

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

static void put_blk (Img &im, int x, int y, int a) {
    if (x < 0 || y < 0 || x >= im.w || y >= im.h) return;
    size_t i = ((size_t)y * im.w + x) * 3;
    int inv = 255 - a;
    im.px[i] = (uint8_t)((im.px[i] * inv) / 255);
    im.px[i + 1] = (uint8_t)((im.px[i + 1] * inv) / 255);
    im.px[i + 2] = (uint8_t)((im.px[i + 2] * inv) / 255);
}

static void line_blk (Img &im, int x0, int y0, int x1, int y1, int a) {
    int dx = x1 - x0, dy = y1 - y0;
    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    int err = ax - ay;
    for (;;) {
        put_blk (im, x0, y0, a);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -ay) { err -= ay; x0 += sx; }
        if (e2 < ax) { err += ax; y0 += sy; }
    }
}

static void draw_tile_brd (Img &im, const DiaGrid &grid, int a) {
    for (int r = 0; r < grid.rows (); r++) {
        for (int c = 0; c < grid.cols (); c++) {
            Vx d[4];
            grid.diamond (r, c, d);
            for (int e = 0; e < 4; e++) {
                int n = (e + 1) & 3;
                line_blk (im, d[e].x, d[e].y, d[n].x, d[n].y, a);
            }
        }
    }
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

static bool win_ok (const int *ids, int full_w, int r0, int c0, int win) {
    int land = 0, seen[TERRAIN_N] = {};
    for (int r = 0; r < win; r++) {
        for (int c = 0; c < win; c++) {
            int id = ids[(r0 + r) * full_w + (c0 + c)];
            if (id < 0 || id >= TERRAIN_N) continue;
            seen[id] = 1;
            if (!ter_any_water (id)) land++;
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
    const char *super_path = G_SUPER;
    const char *super_png = G_SUPER_PNG;
    int win = WIN, save_max = SAVE_MAX;
    SplatMode splat = SPLAT_OTHER;
    uint8_t tips = 0;
    bool tip_filt = false;
    int pos = 0;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp (argv[i], "--other") == 0) { splat = SPLAT_OTHER; continue; }
        if (std::strcmp (argv[i], "--red") == 0) { splat = SPLAT_RED; continue; }
        if (std::strcmp (argv[i], "--none") == 0) { splat = SPLAT_NONE; continue; }
        if (std::strcmp (argv[i], "--N") == 0) { tips |= CORN_N; tip_filt = true; continue; }
        if (std::strcmp (argv[i], "--E") == 0) { tips |= CORN_E; tip_filt = true; continue; }
        if (std::strcmp (argv[i], "--S") == 0) { tips |= CORN_S; tip_filt = true; continue; }
        if (std::strcmp (argv[i], "--W") == 0) { tips |= CORN_W; tip_filt = true; continue; }
        if (pos == 0) terr_path = argv[i];
        else if (pos == 1) clim_path = argv[i];
        else if (pos == 2) out_dir = argv[i];
        else if (pos == 3) win = std::atoi (argv[i]);
        else if (pos == 4) save_max = std::atoi (argv[i]);
        pos++;
    }
    if (win <= 0 || save_max <= 0) {
        std::fprintf (stderr,
            "usage: %s [--other|--red|--none] [--N] [--E] [--S] [--W] [terrain] [climate] [out_dir] [win] [save_max]\n",
            argv[0]);
        return 1;
    }
    if (!tip_filt) tips = CORN_N | CORN_E | CORN_S | CORN_W;
    if (!mkdir_p ("/home/w/Projects/simple-map-gen/blending") ||
        !mkdir_p ("/home/w/Projects/simple-map-gen/blending/mk7-dia") ||
        !mkdir_p (out_dir)) return 1;

    DiaSiSj blit;
    if (!blit.ld (super_path)) {
        std::fprintf (stderr, "failed to load super mask: %s\n", super_path);
        return 1;
    }
    if (!blit.ld_png (super_png))
        std::fprintf (stderr, "warning: no stamp png (%s); solid TER_COL shading\n", super_png);
    if (!blit.ld_splat (root)) {
        std::fprintf (stderr, "failed to load splat banks under: %s\n", root);
        return 1;
    }
    blit.set_splat (splat);
    blit.set_tips (tips);

    DiaEdge edge;
    if (!edge.ld (super_path) || !edge.ld_splat (root)) {
        std::fprintf (stderr, "failed to load edge stamp/splat\n");
        return 1;
    }
    edge.set_tips (tips);
    edge.prep ();

    const char *splat_name = "other";
    if (splat == SPLAT_RED) splat_name = "red";
    else if (splat == SPLAT_NONE) splat_name = "none";
    std::printf ("edge blend  stamp %dx%d  splat=%s  rim thick/delta AA  tex=dat15\n",
                 blit.sw (), blit.sh (), splat_name);

    DiaDat15 bank_desert, bank_plains, bank_grass, bank_blacksoil;
    DiaDat15 bank_ocean, bank_sea, bank_coast;
    char path[512];
    std::snprintf (path, sizeof (path), "%s/tile_desert.dat15", dat_dir);
    if (!bank_desert.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_plains.dat15", dat_dir);
    if (!bank_plains.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_grassland.dat15", dat_dir);
    if (!bank_grass.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_blacksoil.dat15", dat_dir);
    if (!bank_blacksoil.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_ocean.dat15", dat_dir);
    if (!bank_ocean.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_sea.dat15", dat_dir);
    if (!bank_sea.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }
    std::snprintf (path, sizeof (path), "%s/tile_coast.dat15", dat_dir);
    if (!bank_coast.ld (path, TEX_W, TEX_H)) { std::fprintf (stderr, "bad bank %s\n", path); return 1; }

    const DiaDat15 *banks[TERRAIN_N] = {
        &bank_desert, &bank_plains, &bank_grass, &bank_blacksoil,
        &bank_ocean, &bank_sea, &bank_coast, &bank_sea, &bank_coast
    };

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

    using clock = std::chrono::steady_clock;
    for (size_t wi = 0; wi < wins.size (); wi++) {
        int r0 = wins[wi].first, c0 = wins[wi].second;
        int er0 = r0 > 0 ? r0 - 1 : r0;
        int ec0 = c0 > 0 ? c0 - 1 : c0;
        int er1 = r0 + win < full_h ? r0 + win + 1 : r0 + win;
        int ec1 = c0 + win < full_w ? c0 + win + 1 : c0 + win;
        int eh = er1 - er0, ew = ec1 - ec0;
        int tr0 = r0 - er0, tc0 = c0 - ec0;

        std::vector<int> halo ((size_t)eh * ew);
        for (int r = 0; r < eh; r++) {
            for (int c = 0; c < ew; c++)
                halo[r * ew + c] = ids[(er0 + r) * full_w + (ec0 + c)];
        }
        std::vector<int> local ((size_t)win * win);
        for (int r = 0; r < win; r++) {
            for (int c = 0; c < win; c++) local[r * win + c] = ids[(r0 + r) * full_w + (c0 + c)];
        }

        BlendMap emap;
        if (!emap.ld_ids (eh, ew, halo.data ())) return 1;
        BlendMap map;
        if (!map.ld_ids (win, win, local.data ())) return 1;

        auto t0 = clock::now ();
        SubGrid esg;
        esg.build (emap);
        JuncRun run;
        run.bind (&pats);
        run.run (emap, esg);
        blit.fill_corn (esg);

        SubGrid sg;
        sg.copy_tiles (esg, tr0, tc0, win, win);

        DiaGrid grid;
        grid.build (map.rows (), map.cols (), MARGIN);
        int dw = grid.img_w (), dh = grid.img_h ();
        Img tex (dw, dh);
        blit.tex (tex.px.data (), dw, dh, map, sg, grid, banks, TERRAIN_N);
        edge.apply (tex.px.data (), dw, dh, map, sg, grid);
        auto t1 = clock::now ();
        double ms = std::chrono::duration<double, std::milli> (t1 - t0).count ();

        std::snprintf (path, sizeof (path), "%s/edge_%02zu_r%03d_c%03d.png", out_dir, wi, r0, c0);
        if (!save_png (tex, path)) return 1;
        std::printf ("screen %zu  origin=(%d,%d)  halo=%dx%d  img=%dx%d  junc=%d  %.2f ms\n",
                     wi, r0, c0, eh, ew, dw, dh, run.n_junc (), ms);
        std::printf ("  wrote %s\n", path);

        Img brd = tex;
        draw_tile_brd (brd, grid, BORDER_A);
        std::snprintf (path, sizeof (path), "%s/edge_%02zu_r%03d_c%03d_brd.png", out_dir, wi, r0, c0);
        if (!save_png (brd, path)) return 1;
        std::printf ("  wrote %s\n", path);
    }

    std::printf ("mk7 dia_edge_tester  screens=%zu  out=%s\n", wins.size (), out_dir);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
