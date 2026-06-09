//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <set>
#include <tuple>

#include "game_primitives.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "map_loader.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/river-sectors.ppm";

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

static bool save_sectors_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverSectorsResult* res,
    u32 seed) 
{
    if (path == nullptr || terrain == nullptr || res == nullptr || res->overlay == nullptr || w == 0 || h == 0) {
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
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    std::srand(seed);
    std::set<std::tuple<u8, u8, u8>> used;
    u8* clr = new u8[static_cast<size_t>(res->sector_n) * 3u];
    if (clr == nullptr) {
        delete[] rgb;
        return false;
    }
    for (u32 si = 0; si < static_cast<u32>(res->sector_n); ++si) {
        u8 cr = 0;
        u8 cg = 0;
        u8 cb = 0;
        while (true) {
            cr = static_cast<u8>(50 + (std::rand() % 151));
            cg = static_cast<u8>(50 + (std::rand() % 151));
            cb = static_cast<u8>(50 + (std::rand() % 151));
            const std::tuple<u8, u8, u8> t = {cr, cg, cb};
            if (used.find(t) == used.end()) {
                used.insert(t);
                break;
            }
        }
        clr[si * 3u + 0] = cr;
        clr[si * 3u + 1] = cg;
        clr[si * 3u + 2] = cb;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = res->overlay[i];
        if (sid == static_cast<u16>(RIVER_SECTOR_NONE)) {
            continue;
        }
        const u32 p = i * 3u;
        const u32 c = static_cast<u32>(sid) * 3u;
        rgb[p + 0] = clr[c + 0];
        rgb[p + 1] = clr[c + 1];
        rgb[p + 2] = clr[c + 2];
    }
    delete[] clr;
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_generate_river_sectors_basic (u32 seed) {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {
        std::printf("failed to load map: %s\n", g_map_path);
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* terrain = map.data();
    if (terrain == nullptr || w == 0 || h == 0) {
        std::printf("invalid map data\n");
        return -1;
    }
    const clock_t t_pts0 = clock();
    RiverPtsResult* pts = Generate_RiverPts::generate(terrain, w, h, seed);
    const clock_t t_pts1 = clock();
    if (pts == nullptr) {
        std::printf("Generate_RiverPts failed to generate\n");
        return -1;
    }
    const clock_t t0 = clock();
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(terrain, w, h, pts, seed);
    const clock_t t1 = clock();
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        std::printf("Generate_RiverSectors failed to generate\n");
        return -1;
    }
    const double pts_sec = static_cast<double>(t_pts1 - t_pts0) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_RiverPts generate time: %.6f s\n", pts_sec);
    std::printf("Generate_RiverSectors generate time: %.6f s (%u x %u)\n", sec, w, h);
    const bool ok = save_sectors_viz(g_out_path, terrain, w, h, sectors, seed);
    Generate_RiverSectors::free_result(sectors);
    if (!ok) {
        std::printf("failed to save: %s\n", g_out_path);
        return -1;
    }
    std::printf("saved: %s\n", g_out_path);
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
    return test_generate_river_sectors_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
