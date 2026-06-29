//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>

#include "game_primitives.h"
#include "generate_distance_to_river.h"
#include "generate_latitude_overlay.h"
#include "generate_map_climate.h"
#include "generate_open_distance_to_water.h"
#include "generate_plain_distance_to_water.h"
#include "generate_river_lines.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "map_loader.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_dir = "/home/w/Projects/simple-map-gen/map-climate-params";

//================================================================================================================================
//=> - ClimateCtx -
//================================================================================================================================

struct ClimateCtx {
    MapTerrainData map;
    u16 w;
    u16 h;
    const u8* terrain;
    u8* dist_river;
    u8* open_dist_water;
    u8* latitude;
    u8* plain_dist_water;
};

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool save_climate_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const MapClimateResult* res) 
{
    if (path == nullptr || terrain == nullptr || res == nullptr || res->climate == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        const u8 cl = res->climate[i];
        if (cl != CLIMATE_NONE) {
            climate_to_rgb(cl, &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static MapClimateTypePct mk_pct (u8 grassland, u8 plains, u8 desert) {
    MapClimateTypePct p = {};
    p.pct_grassland = grassland;
    p.pct_plains = plains;
    p.pct_desert = desert;
    return p;
}

static i32 run_case (
    ClimateCtx* ctx,
    MapClimateOverlayWts wts,
    MapClimateTypePct pct,
    cstr tag) 
{
    if (ctx == nullptr || tag == nullptr) {
        return -1;
    }
    char path[512];
    std::snprintf(
        path,
        sizeof(path),
        "%s/%s.ppm",
        g_out_dir,
        tag);
    MapClimateOverlays ovs = {};
    ovs.dist_river = ctx->dist_river;
    ovs.open_dist_water = ctx->open_dist_water;
    ovs.latitude = ctx->latitude;
    ovs.plain_dist_water = ctx->plain_dist_water;
    MapClimateParams prm = {};
    prm.wts = wts;
    prm.pct = pct;
    const clock_t t0 = clock();
    MapClimateResult* climate = Generate_MapClimate::generate(ctx->terrain, ctx->w, ctx->h, &ovs, &prm);
    const clock_t t1 = clock();
    if (climate == nullptr) {
        std::printf("Generate_MapClimate failed: %s\n", tag);
        return -1;
    }
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const bool ok = save_climate_viz(path, ctx->terrain, ctx->w, ctx->h, climate);
    Generate_MapClimate::free_result(climate);
    if (!ok) {
        std::printf("failed to save: %s\n", path);
        return -1;
    }
    std::printf(
        "%s  wts=%u,%u,%u,%u  pct=%u,%u,%u  time=%.6f s  saved=%s\n",
        tag,
        wts.w_dist_river,
        wts.w_open_dist_water,
        wts.w_latitude,
        wts.w_plain_dist_water,
        pct.pct_grassland,
        pct.pct_plains,
        pct.pct_desert,
        sec,
        path);
    return 0;
}

static bool build_ctx (ClimateCtx* ctx, u32 seed) {
    if (ctx == nullptr) {
        return false;
    }
    if (!MapLoader::load_terrain_ppm(g_map_path, ctx->map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return false;
    }
    ctx->w = ctx->map.width();
    ctx->h = ctx->map.height();
    ctx->terrain = ctx->map.data();
    if (ctx->terrain == nullptr || ctx->w == 0 || ctx->h == 0) {
        std::printf("invalid map data\n");
        return false;
    }
    RiverPtsResult* pts = Generate_RiverPts::generate(ctx->terrain, ctx->w, ctx->h, seed);
    if (pts == nullptr) {
        return false;
    }
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(ctx->terrain, ctx->w, ctx->h, pts, seed);
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        return false;
    }
    RiverNetworkResult* network = Generate_RiverNetwork::generate(ctx->terrain, ctx->w, ctx->h, sectors);
    if (network == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        return false;
    }
    RiverLinesResult* lines = Generate_RiverLines::generate(ctx->terrain, ctx->w, ctx->h, sectors, network, seed);
    Generate_RiverNetwork::free_result(network);
    if (lines == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        return false;
    }
    ctx->dist_river = Generate_DistanceToRiver::generate(ctx->terrain, ctx->w, ctx->h, lines->overlay);
    Generate_RiverLines::free_result(lines);
    Generate_RiverSectors::free_result(sectors);
    if (ctx->dist_river == nullptr) {
        return false;
    }
    ctx->open_dist_water = Generate_OpenDistanceToWater::generate(ctx->terrain, ctx->w, ctx->h);
    if (ctx->open_dist_water == nullptr) {
        return false;
    }
    ctx->latitude = Generate_LatitudeOverlay::generate(ctx->w, ctx->h);
    if (ctx->latitude == nullptr) {
        return false;
    }
    ctx->plain_dist_water = Generate_PlainDistanceToWater::generate(ctx->terrain, ctx->w, ctx->h);
    return ctx->plain_dist_water != nullptr;
}

static void free_ctx (ClimateCtx* ctx) {
    if (ctx == nullptr) {
        return;
    }
    delete[] ctx->plain_dist_water;
    delete[] ctx->latitude;
    delete[] ctx->open_dist_water;
    delete[] ctx->dist_river;
    ctx->plain_dist_water = nullptr;
    ctx->latitude = nullptr;
    ctx->open_dist_water = nullptr;
    ctx->dist_river = nullptr;
}

static i32 run_isolate_tests (ClimateCtx* ctx, MapClimateTypePct pct) {
    MapClimateOverlayWts w0 = {};
    i32 rc = 0;
    w0.w_dist_river = 1;
    rc |= run_case(ctx, w0, pct, "iso-river");
    w0 = {};
    w0.w_open_dist_water = 1;
    rc |= run_case(ctx, w0, pct, "iso-open");
    w0 = {};
    w0.w_latitude = 1;
    rc |= run_case(ctx, w0, pct, "iso-latitude");
    w0 = {};
    w0.w_plain_dist_water = 1;
    rc |= run_case(ctx, w0, pct, "iso-plain");
    return rc;
}

static i32 run_sweep_tests (ClimateCtx* ctx, MapClimateTypePct pct) {
    i32 rc = 0;
    for (u16 v = 0; v <= CLIMATE_WT_MAX; ++v) {
        const u8 comp = static_cast<u8>(CLIMATE_WT_MAX - v);
        MapClimateOverlayWts w = {};
        w.w_dist_river = static_cast<u8>(v);
        w.w_open_dist_water = comp;
        w.w_latitude = comp;
        w.w_plain_dist_water = comp;
        char tag[64];
        std::snprintf(tag, sizeof(tag), "sweep-river-w%03u", v);
        rc |= run_case(ctx, w, pct, tag);
    }
    for (u16 v = 0; v <= CLIMATE_WT_MAX; ++v) {
        const u8 comp = static_cast<u8>(CLIMATE_WT_MAX - v);
        MapClimateOverlayWts w = {};
        w.w_dist_river = comp;
        w.w_open_dist_water = static_cast<u8>(v);
        w.w_latitude = comp;
        w.w_plain_dist_water = comp;
        char tag[64];
        std::snprintf(tag, sizeof(tag), "sweep-open-w%03u", v);
        rc |= run_case(ctx, w, pct, tag);
    }
    for (u16 v = 0; v <= CLIMATE_WT_MAX; ++v) {
        const u8 comp = static_cast<u8>(CLIMATE_WT_MAX - v);
        MapClimateOverlayWts w = {};
        w.w_dist_river = comp;
        w.w_open_dist_water = comp;
        w.w_latitude = static_cast<u8>(v);
        w.w_plain_dist_water = comp;
        char tag[64];
        std::snprintf(tag, sizeof(tag), "sweep-latitude-w%03u", v);
        rc |= run_case(ctx, w, pct, tag);
    }
    for (u16 v = 0; v <= CLIMATE_WT_MAX; ++v) {
        const u8 comp = static_cast<u8>(CLIMATE_WT_MAX - v);
        MapClimateOverlayWts w = {};
        w.w_dist_river = comp;
        w.w_open_dist_water = comp;
        w.w_latitude = comp;
        w.w_plain_dist_water = static_cast<u8>(v);
        char tag[64];
        std::snprintf(tag, sizeof(tag), "sweep-plain-w%03u", v);
        rc |= run_case(ctx, w, pct, tag);
    }
    return rc;
}

static i32 test_generate_map_climate_params (u32 seed) {
    ClimateCtx ctx = {};
    if (!build_ctx(&ctx, seed)) {
        std::printf("failed to build overlay context\n");
        return -1;
    }
    const MapClimateTypePct pct = mk_pct(40, 35, 25);
    i32 rc = 0;
    rc |= run_isolate_tests(&ctx, pct);
    rc |= run_sweep_tests(&ctx, pct);
    free_ctx(&ctx);
    return rc;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    u32 seed = 0;
    if (argc >= 2) {
        seed = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
    }
    return test_generate_map_climate_params(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
