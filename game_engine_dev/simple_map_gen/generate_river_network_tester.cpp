//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>
#include <tuple>
#include <utility>

#include "game_primitives.h"
#include "generate_river_network.h"
#include "generate_river_pts.h"
#include "generate_river_sectors.h"
#include "map_loader.h"
#include "map_terrain_validate.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/river-network.ppm";

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

static void draw_thick_line (
    u8* rgb,
    u16 w,
    u16 h,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    u8 r,
    u8 g,
    u8 b,
    i32 thick) 
{
    i32 dx = x1 - x2;
    if (dx < 0) {
        dx = -dx;
    }
    i32 dy = y1 - y2;
    if (dy < 0) {
        dy = -dy;
    }
    i32 sx = (x1 < x2) ? 1 : -1;
    i32 sy = (y1 < y2) ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x1;
    i32 y = y1;
    while (true) {
        for (i32 ty = -thick; ty <= thick; ++ty) {
            for (i32 tx = -thick; tx <= thick; ++tx) {
                if (tx * tx + ty * ty > thick * thick) {
                    continue;
                }
                const i32 px = x + tx;
                const i32 py = y + ty;
                if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 p = (static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)) * 3u;
                rgb[p + 0] = r;
                rgb[p + 1] = g;
                rgb[p + 2] = b;
            }
        }
        if (x == x2 && y == y2) {
            break;
        }
        const i32 e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

static bool save_network_viz (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverSectorsResult* sectors,
    const RiverNetworkResult* network,
    u32 seed) 
{
    if (path == nullptr || terrain == nullptr || sectors == nullptr || network == nullptr
        || sectors->overlay == nullptr || sectors->nodes == nullptr || w == 0 || h == 0) {
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
    std::map<u16, std::tuple<u8, u8, u8>> sys_clr;
    for (u32 si = 0; si < static_cast<u32>(network->sector_n); ++si) {
        const u16 rid = network->river_sys[si];
        if (rid == static_cast<u16>(RIVER_SYS_NONE)) {
            continue;
        }
        if (sys_clr.find(rid) == sys_clr.end()) {
            sys_clr[rid] = {
                static_cast<u8>(50 + (std::rand() % 151)),
                static_cast<u8>(50 + (std::rand() % 151)),
                static_cast<u8>(50 + (std::rand() % 151))};
        }
    }
    const u8 line_r = 0;
    const u8 line_g = 100;
    const u8 line_b = 255;
    if (network->conns != nullptr) {
        for (u16 ci = 0; ci < network->conn_n; ++ci) {
            const u16 ia = network->conns[ci].a;
            const u16 ib = network->conns[ci].b;
            draw_thick_line(
                rgb,
                w,
                h,
                sectors->nodes[ia].cx,
                sectors->nodes[ia].cy,
                sectors->nodes[ib].cx,
                sectors->nodes[ib].cy,
                line_r,
                line_g,
                line_b,
                1);
        }
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sectors->overlay[i];
        if (sid == static_cast<u16>(RIVER_SECTOR_NONE)) {
            continue;
        }
        const u16 rid = network->river_sys[sid];
        if (rid == static_cast<u16>(RIVER_SYS_NONE)) {
            continue;
        }
        auto it = sys_clr.find(rid);
        if (it == sys_clr.end()) {
            continue;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = std::get<0>(it->second);
        rgb[p + 1] = std::get<1>(it->second);
        rgb[p + 2] = std::get<2>(it->second);
    }
    if (network->conns != nullptr) {
        for (u16 ci = 0; ci < network->conn_n; ++ci) {
            const u16 ia = network->conns[ci].a;
            const u16 ib = network->conns[ci].b;
            draw_thick_line(
                rgb,
                w,
                h,
                sectors->nodes[ia].cx,
                sectors->nodes[ia].cy,
                sectors->nodes[ib].cx,
                sectors->nodes[ib].cy,
                line_r,
                line_g,
                line_b,
                1);
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_generate_river_network_basic (u32 seed) {
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
    const clock_t t_sec0 = clock();
    RiverSectorsResult* sectors = Generate_RiverSectors::generate(terrain, w, h, pts, seed);
    const clock_t t_sec1 = clock();
    Generate_RiverPts::free_result(pts);
    if (sectors == nullptr) {
        std::printf("Generate_RiverSectors failed to generate\n");
        return -1;
    }
    const clock_t t0 = clock();
    RiverNetworkResult* network = Generate_RiverNetwork::generate(terrain, w, h, sectors);
    const clock_t t1 = clock();
    if (network == nullptr) {
        Generate_RiverSectors::free_result(sectors);
        std::printf("Generate_RiverNetwork failed to generate\n");
        return -1;
    }
    const double pts_sec = static_cast<double>(t_pts1 - t_pts0) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec_sec = static_cast<double>(t_sec1 - t_sec0) / static_cast<double>(CLOCKS_PER_SEC);
    const double net_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("Generate_RiverPts generate time: %.6f s\n", pts_sec);
    std::printf("Generate_RiverSectors generate time: %.6f s\n", sec_sec);
    std::printf("Generate_RiverNetwork generate time: %.6f s (%u x %u)\n", net_sec, w, h);
    const bool ok = save_network_viz(g_out_path, terrain, w, h, sectors, network, seed);
    Generate_RiverNetwork::free_result(network);
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
    return test_generate_river_network_basic(seed);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
