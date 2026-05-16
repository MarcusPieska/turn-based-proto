//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>

#include "game_primitives.h"
#include "generate_map_continents_basic.h"
#include "generate_overlay_water_land.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool ensure_maps_dir () {
    if (mkdir("maps", 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

static const u8* terr_rgb_from_class (u8 c) {
    static const u8* const k_rows[] = {
        TERR_NONE,
        TERR_OCEAN,
        TERR_SEA,
        TERR_COASTAL,
        TERR_PLAINS,
        TERR_HILLS,
        TERR_MOUNTAINS};
    for (unsigned i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == c) {
            return k_rows[i] + 1;
        }
    }
    return TERR_NONE + 1;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    std::fwrite(rgb, 1, nbytes, fp);
    std::fclose(fp);
    return true;
}

static void rgb_set_px (std::vector<u8>& rgb, u16 wi, u16 hi, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(wi) || y >= static_cast<i32>(hi)) {
        return;
    }
    const size_t o = (static_cast<size_t>(y) * static_cast<size_t>(wi) + static_cast<size_t>(x)) * 3u;
    rgb[o] = r;
    rgb[o + 1u] = g;
    rgb[o + 2u] = b;
}

static void rgb_draw_hseg (std::vector<u8>& rgb, u16 wi, u16 hi, i32 x0, i32 x1, i32 y, u8 r, u8 g, u8 b) {
    if (y < 0 || y >= static_cast<i32>(hi)) {
        return;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
    }
    for (i32 x = x0; x <= x1; ++x) {
        rgb_set_px(rgb, wi, hi, x, y, r, g, b);
    }
}

static void rgb_draw_vseg (std::vector<u8>& rgb, u16 wi, u16 hi, i32 x, i32 y0, i32 y1, u8 r, u8 g, u8 b) {
    if (x < 0 || x >= static_cast<i32>(wi)) {
        return;
    }
    if (y0 > y1) {
        std::swap(y0, y1);
    }
    for (i32 y = y0; y <= y1; ++y) {
        rgb_set_px(rgb, wi, hi, x, y, r, g, b);
    }
}

static void rgb_draw_rect_outline (std::vector<u8>& rgb, u16 wi, u16 hi, i32 ax, i32 ay, i32 S, u8 r, u8 g, u8 b) {
    if (S <= 0) {
        return;
    }
    const i32 x1 = ax + S - 1;
    const i32 y1 = ay + S - 1;
    rgb_draw_hseg(rgb, wi, hi, ax, x1, ay, r, g, b);
    rgb_draw_hseg(rgb, wi, hi, ax, x1, y1, r, g, b);
    rgb_draw_vseg(rgb, wi, hi, ax, ay, y1, r, g, b);
    rgb_draw_vseg(rgb, wi, hi, x1, ay, y1, r, g, b);
}

static void rgb_draw_dot (std::vector<u8>& rgb, u16 wi, u16 hi, i32 cx, i32 cy, i32 rad, u8 r, u8 g, u8 b) {
    const i32 r2 = rad * rad;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            if (dx * dx + dy * dy <= r2) {
                rgb_set_px(rgb, wi, hi, cx + dx, cy + dy, r, g, b);
            }
        }
    }
}

static void rgb_fill_from_terrain (const MapArrayTerrain& m, std::vector<u8>& out_rgb) {
    const u16 w = m.width();
    const u16 h = m.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    out_rgb.resize(static_cast<size_t>(n) * 3u);
    const u8* d = m.data();
    for (u32 i = 0; i < n; ++i) {
        const u8* px = terr_rgb_from_class(d[i]);
        out_rgb[i * 3u + 0u] = px[0];
        out_rgb[i * 3u + 1u] = px[1];
        out_rgb[i * 3u + 2u] = px[2];
    }
}

static bool save_layout_overlay_ppm (const Generate_MapContinentsBasic& gen, cstr path) {
    if (!gen.is_valid()) {
        return false;
    }
    const MapArrayTerrain& cv = gen.canvas();
    const u16 wi = cv.width();
    const u16 hi = cv.height();
    std::vector<u8> rgb;
    rgb_fill_from_terrain(cv, rgb);
    const u8 cr = 0;
    const u8 cg = 220;
    const u8 cb = 255;
    const u8 dr = 255;
    const u8 dg = 40;
    const u8 db = 40;
    for (size_t i = 0; i < gen.layer_count(); ++i) {
        u16 ax = 0;
        u16 ay = 0;
        gen.layer_anchor(i, ax, ay);
        const u16 S = gen.layer_size(i);
        rgb_draw_rect_outline(rgb, wi, hi, static_cast<i32>(ax), static_cast<i32>(ay), static_cast<i32>(S), cr, cg, cb);
        rgb_draw_dot(rgb, wi, hi, static_cast<i32>(ax), static_cast<i32>(ay), 3, dr, dg, db);
    }
    return save_rgb_ppm(path, rgb.data(), wi, hi);
}

static i32 test_generate_map_continents_basic (u32 seed) {
    if (!ensure_maps_dir()) {
        std::printf("could not create maps/\n");
        return -1;
    }

    MapContinentsBasicParams params;
    params.m_seed = seed;
    params.m_canvas_w = 1000;
    params.m_canvas_h = 1000;
    params.m_continent_count = 20;
    params.m_min_cont_size_perc = 5;
    params.m_max_cont_size_perc = 25;
    params.m_large_continent_count = 2;
    params.m_large_continent_size_perc = 45;
    params.m_patch_combo_min_size_perc = 20;

    std::printf(
        "params: seed=%u canvas=%u x %u continents=%u size_perc=[%u,%u] large_count=%u large_perc=%u "
        "two_layer_combo when patch_side is at least %u%% of min(canvas_w,h); else p1-only (inner_grad=0)\n",
        params.m_seed,
        params.m_canvas_w,
        params.m_canvas_h,
        (unsigned)params.m_continent_count,
        (unsigned)params.m_min_cont_size_perc,
        (unsigned)params.m_max_cont_size_perc,
        (unsigned)params.m_large_continent_count,
        (unsigned)params.m_large_continent_size_perc,
        (unsigned)params.m_patch_combo_min_size_perc);

    const clock_t t0 = clock();
    Generate_MapContinentsBasic gen(params);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);

    if (!gen.is_valid()) {
        std::printf("outcome: FAIL (invalid generation)\n");
        std::printf("wall time: %.6f s\n", sec);
        return -1;
    }

    std::printf("outcome: OK\n");
    std::printf("wall time: %.6f s\n", sec);
    std::printf("layers: %zu canvas %u x %u\n", gen.layer_count(), gen.canvas_width(), gen.canvas_height());

    std::printf("layer sizes:");
    for (size_t i = 0; i < gen.layer_count(); ++i) {
        std::printf(" %u", (unsigned)gen.layer_size(i));
    }
    std::printf("\n");

    for (size_t i = 0; i < gen.layer_count(); ++i) {
        u16 ax = 0;
        u16 ay = 0;
        gen.layer_anchor(i, ax, ay);
        std::printf("  layer %zu size %u anchor (%u,%u)\n", i, gen.layer_size(i), (unsigned)ax, (unsigned)ay);
        char ppath[160];
        std::snprintf(ppath, sizeof(ppath), "maps/out_map_map_continents_basic_patch_%02zu.ppm", i);
        if (!gen.layer_patch(i).save(ppath)) {
            std::printf("save patch %zu failed\n", i);
            return -1;
        }
    }
    if (!gen.save_output("maps/out_map_map_continents_basic.ppm")) {
        std::printf("save composite canvas failed\n");
        return -1;
    }
    if (!save_layout_overlay_ppm(gen, "maps/out_map_map_continents_basic_layout.ppm")) {
        std::printf("save layout overlay failed\n");
        return -1;
    }
    std::printf("wrote maps/out_map_map_continents_basic_layout.ppm (cyan rects, red anchor dots)\n");

    Generate_OverlayWaterLand wl;
    if (!wl.generate(gen.canvas()) || !wl.is_valid()) {
        std::printf("water-land overlay generate failed\n");
        return -1;
    }
    if (!wl.save_water_land_gray("maps/out_map_map_continents_basic_water_land.pgm")) {
        std::printf("save water-land overlay failed\n");
        return -1;
    }
    std::printf("wrote maps/out_map_map_continents_basic_water_land.pgm\n");
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 0;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }

    return test_generate_map_continents_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
