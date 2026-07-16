//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "city.h"
#include "city_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "tile_working.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_POP_MAX = 40;
static const u16 G_REACH_R = 4;
static const u16 G_EDGE_PAD = 4;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool build_paths () {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool is_city_terr (u8 terr) {
    return terr == TERR_PLAINS[0] || terr == TERR_HILLS[0];
}

static bool is_city_clim (u8 clim) {
    return clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS || clim == CLIMATE_BLACK_SOIL;
}

static u32 score_spot (u16 cx, u16 cy) {
    const CircArea area = CircularTileAreas::get(G_REACH_R);
    u32 score = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!TileYields::in_bounds(ux, uy)) {
            continue;
        }
        const TileYield yld = TileYields::get(ux, uy);
        score += static_cast<u32>(yld.m_food) + static_cast<u32>(yld.m_production) + static_cast<u32>(yld.m_commerce);
    }
    return score;
}

struct SpotCand {
    u16 m_x;
    u16 m_y;
    u32 m_score;
};

static bool cmp_spot (const SpotCand& a, const SpotCand& b) {
    return a.m_score > b.m_score;
}

static bool pick_spot (const GameArraySimple& map, u16* out_x, u16* out_y) {
    std::vector<SpotCand> spots;
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = G_EDGE_PAD; y + G_EDGE_PAD < h; ++y) {
        for (u16 x = G_EDGE_PAD; x + G_EDGE_PAD < w; ++x) {
            if (!is_city_terr(map.get_terrain(x, y)) || !is_city_clim(map.get_climate(x, y))) {
                continue;
            }
            spots.push_back({x, y, score_spot(x, y)});
        }
    }
    if (spots.empty()) {
        return false;
    }
    std::sort(spots.begin(), spots.end(), cmp_spot);
    *out_x = spots[0].m_x;
    *out_y = spots[0].m_y;
    return true;
}

static f64 avg_us (const std::vector<f64>& v) {
    if (v.empty()) {
        return 0.0;
    }
    f64 sum = 0.0;
    for (f64 t : v) {
        sum += t;
    }
    return sum / static_cast<f64>(v.size());
}

static void run_add_test (cstr lbl, u16 player, u16 city_idx, City* city, u16 spot_x, u16 spot_y,
    TotalTileYield (*fn)(u16, u16)) {
    city->init(player, spot_x, spot_y);
    CityTileManager::clear(spot_x, spot_y, city_idx);
    std::vector<f64> times;
    times.reserve(G_POP_MAX);
    for (u16 pop = 1; pop <= G_POP_MAX; ++pop) {
        city->set_population(pop);
        const auto t0 = std::chrono::high_resolution_clock::now();
        const TotalTileYield yld = fn(player, city_idx);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
        times.push_back(us);
        const u32 worked = CityTileManager::count_worked(spot_x, spot_y, city_idx);
        std::printf("  pop=%2u us=%.2f food=%u production=%u commerce=%u worked=%u\n",
            (unsigned)pop, us, yld.m_food, yld.m_production, yld.m_commerce, worked);
    }
    std::printf("%s avg_us=%.2f samples=%zu\n", lbl, avg_us(times), times.size());
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, nullptr)) {
        std::printf("fail load map\n");
        return 1;
    }
    CityArray cities;
    if (!cities.bind_statics(st)) {
        std::printf("fail bind city array\n");
        return 1;
    }
    TileYields::bind_map(&map);
    TileWorking::bind_map(&map);
    CityTileManager::bind_cities(&cities);
    u16 spot_x = 0;
    u16 spot_y = 0;
    if (!pick_spot(map, &spot_x, &spot_y)) {
        std::printf("fail pick land spot\n");
        return 1;
    }
    const u16 player = 0;
    const u16 city_idx = cities.get_next_new_city_idx();
    City* city = cities.get_city(city_idx);
    if (city == nullptr) {
        std::printf("fail get city\n");
        return 1;
    }
    std::printf("spot=(%u,%u) pop=1..%u\n", (unsigned)spot_x, (unsigned)spot_y, (unsigned)G_POP_MAX);
    run_add_test("add_new_food_tile", player, city_idx, city, spot_x, spot_y, CityTileManager::add_new_food_tile);
    run_add_test("add_new_production_tile", player, city_idx, city, spot_x, spot_y, CityTileManager::add_new_production_tile);
    run_add_test("add_new_commerce_tile", player, city_idx, city, spot_x, spot_y, CityTileManager::add_new_commerce_tile);
    TileYields::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    CityTileManager::bind_cities(nullptr);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
