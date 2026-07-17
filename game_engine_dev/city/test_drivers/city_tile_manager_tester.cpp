//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
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
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-tile-manager-test";
static const u32 G_SEED = 43u;
static const u16 G_POP = 15;
static const u16 G_IMG = 201;
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

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static void map_rgb (const GameArraySimple& map, u16 x, u16 y, u8* r, u8* g, u8* b) {
    terr_rgb(map.get_terrain(x, y), r, g, b);
    if (map.get_river(x, y) != 0) {
        *r = 0;
        *g = 180;
        *b = 255;
    }
}

static bool wr_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static const u16 G_TRY_N = 1000;
static const u16 G_IMG_R = 100;

struct SpotCand {
    u16 m_x; // Map column
    u16 m_y; // Map row
    u32 m_score; // Sum of yields in work reach
};

static bool cmp_spot (const SpotCand& a, const SpotCand& b) {
    return a.m_score > b.m_score;
}

static bool tot_eq (const TotalTileYield& a, const TotalTileYield& b) {
    return a.m_food == b.m_food && a.m_production == b.m_production && a.m_commerce == b.m_commerce;
}

static bool tots_differ (const TotalTileYield& a, const TotalTileYield& b, const TotalTileYield& c) {
    return !tot_eq(a, b) || !tot_eq(a, c) || !tot_eq(b, c);
}

static u8 diff_n (const TotalTileYield& a, const TotalTileYield& b) {
    u8 n = 0;
    if (a.m_food != b.m_food) {
        ++n;
    }
    if (a.m_production != b.m_production) {
        ++n;
    }
    if (a.m_commerce != b.m_commerce) {
        ++n;
    }
    return n;
}

static bool strong_tots_differ (const TotalTileYield& a, const TotalTileYield& b, const TotalTileYield& c) {
    if (!tots_differ(a, b, c)) {
        return false;
    }
    return diff_n(a, b) >= 2 || diff_n(a, c) >= 2 || diff_n(b, c) >= 2;
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

static void collect_spots (const GameArraySimple& map, std::vector<SpotCand>& out) {
    const u16 w = map.width();
    const u16 h = map.height();
    out.clear();
    for (u16 y = G_EDGE_PAD; y + G_EDGE_PAD < h; ++y) {
        for (u16 x = G_EDGE_PAD; x + G_EDGE_PAD < w; ++x) {
            if (!is_city_terr(map.get_terrain(x, y)) || !is_city_clim(map.get_climate(x, y))) {
                continue;
            }
            out.push_back({x, y, score_spot(x, y)});
        }
    }
    std::sort(out.begin(), out.end(), cmp_spot);
}

static bool in_reach (u16 cx, u16 cy, u16 x, u16 y) {
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(cx);
    const i32 dy = static_cast<i32>(y) - static_cast<i32>(cy);
    return dx * dx + dy * dy < static_cast<i32>(G_REACH_R) * static_cast<i32>(G_REACH_R);
}

static void wr_sel_img (const GameArraySimple& map, u16 cx, u16 cy, u16 city_idx, cstr path) {
    u8 rgb[G_IMG * G_IMG * 3];
    for (u16 py = 0; py < G_IMG; ++py) {
        for (u16 px = 0; px < G_IMG; ++px) {
            const i32 mx = static_cast<i32>(cx) + static_cast<i32>(px) - static_cast<i32>(G_IMG_R);
            const i32 my = static_cast<i32>(cy) + static_cast<i32>(py) - static_cast<i32>(G_IMG_R);
            u8* p = &rgb[(static_cast<u32>(py) * G_IMG + static_cast<u32>(px)) * 3u];
            if (mx < 0 || my < 0 || mx >= static_cast<i32>(map.width()) || my >= static_cast<i32>(map.height())) {
                p[0] = 255;
                p[1] = 255;
                p[2] = 255;
                continue;
            }
            const u16 ux = static_cast<u16>(mx);
            const u16 uy = static_cast<u16>(my);
            if (ux == cx && uy == cy) {
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
            } else if (in_reach(cx, cy, ux, uy) && TileWorking::get_worker(ux, uy) == city_idx) {
                p[0] = 255;
                p[1] = 0;
                p[2] = 0;
            } else {
                map_rgb(map, ux, uy, &p[0], &p[1], &p[2]);
            }
        }
    }
    wr_ppm(path, rgb, G_IMG, G_IMG);
}

static void run_sel (cstr lbl, cstr img, u16 player, u16 city_idx, u16 pop,
    const GameArraySimple& map, u16 cx, u16 cy, TotalTileYield (*fn)(u16, u16)) {
    CityTileManager::clear(cx, cy, city_idx);
    const auto t0 = std::chrono::high_resolution_clock::now();
    const TotalTileYield tot = fn(player, city_idx);
    const auto t1 = std::chrono::high_resolution_clock::now();
    const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    const u32 worked = CityTileManager::count_worked(cx, cy, city_idx);
    std::printf("%s: %.2f us food=%u production=%u commerce=%u worked=%u pop=%u\n",
        lbl, us, tot.m_food, tot.m_production, tot.m_commerce, worked, (unsigned)pop);
    assert(worked == static_cast<u32>(pop));
    wr_sel_img(map, cx, cy, city_idx, img);
    std::printf("wrote %s\n", img);
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main (int argc, char** argv) {
    bool use_rnd = false;
    u32 rnd_seed = 0;
    if (argc >= 2) {
        char* end = nullptr;
        const unsigned long v = std::strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0') {
            std::printf("usage: %s [seed]\n", argv[0]);
            return 1;
        }
        use_rnd = true;
        rnd_seed = static_cast<u32>(v);
    }
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
    if (!TileYields::setup(st)) {
        std::printf("fail setup tile yields\n");
        return 1;
    }
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
    std::vector<SpotCand> spots;
    collect_spots(map, spots);
    if (spots.empty()) {
        std::printf("fail collect city spots\n");
        return 1;
    }
    const u16 player = 0;
    const u16 city_idx = cities.get_next_new_city_idx();
    City* city = cities.get_city(city_idx);
    if (city == nullptr) {
        std::printf("fail get city\n");
        return 1;
    }
    u16 hit_x = 0;
    u16 hit_y = 0;
    u32 hit_score = 0;
    TotalTileYield hit_f = {};
    TotalTileYield hit_s = {};
    TotalTileYield hit_c = {};
    if (use_rnd) {
        std::srand(rnd_seed);
        const size_t i = static_cast<size_t>(std::rand()) % spots.size();
        const SpotCand& sp = spots[i];
        hit_x = sp.m_x;
        hit_y = sp.m_y;
        hit_score = sp.m_score;
        city->init(player, hit_x, hit_y);
        city->set_population(G_POP);
        hit_f = CityTileManager::maximize_food(player, city_idx);
        hit_s = CityTileManager::maximize_production(player, city_idx);
        hit_c = CityTileManager::maximize_commerce(player, city_idx);
        std::printf("rnd_seed=%u spot_i=%zu spot=(%u,%u) reach_score=%u\n",
            (unsigned)rnd_seed, i, (unsigned)hit_x, (unsigned)hit_y, hit_score);
    } else {
        const u16 try_n = static_cast<u16>(spots.size() < G_TRY_N ? spots.size() : G_TRY_N);
        std::printf("spots=%zu trying=%u\n", spots.size(), (unsigned)try_n);
        u16 hit_run = try_n;
        bool found_diff = false;
        for (u16 run = 0; run < try_n; ++run) {
            const SpotCand& sp = spots[run];
            city->init(player, sp.m_x, sp.m_y);
            city->set_population(G_POP);
            const TotalTileYield tot_f = CityTileManager::maximize_food(player, city_idx);
            const TotalTileYield tot_s = CityTileManager::maximize_production(player, city_idx);
            const TotalTileYield tot_c = CityTileManager::maximize_commerce(player, city_idx);
            if (!strong_tots_differ(tot_f, tot_s, tot_c)) {
                continue;
            }
            found_diff = true;
            hit_run = run;
            hit_x = sp.m_x;
            hit_y = sp.m_y;
            hit_score = sp.m_score;
            hit_f = tot_f;
            hit_s = tot_s;
            hit_c = tot_c;
            break;
        }
        if (!found_diff) {
            std::printf("no selector diff after %u spots\n", (unsigned)try_n);
            TileYields::bind_map(nullptr);
            TileWorking::bind_map(nullptr);
            CityTileManager::bind_cities(nullptr);
            return 0;
        }
        std::printf("diff at run=%u spot=(%u,%u) reach_score=%u\n",
            (unsigned)hit_run, (unsigned)hit_x, (unsigned)hit_y, hit_score);
    }
    std::printf("  food_sel     food=%u production=%u commerce=%u\n", hit_f.m_food, hit_f.m_production, hit_f.m_commerce);
    std::printf("  production_sel  food=%u production=%u commerce=%u\n", hit_s.m_food, hit_s.m_production, hit_s.m_commerce);
    std::printf("  commerce_sel food=%u production=%u commerce=%u\n", hit_c.m_food, hit_c.m_production, hit_c.m_commerce);
    city->init(player, hit_x, hit_y);
    city->set_population(G_POP);
    char path[512];
    std::snprintf(path, sizeof(path), "%s/food.ppm", G_OUT_DIR);
    run_sel("maximize_food", path, player, city_idx, G_POP, map, hit_x, hit_y, CityTileManager::maximize_food);
    std::snprintf(path, sizeof(path), "%s/production.ppm", G_OUT_DIR);
    run_sel("maximize_production", path, player, city_idx, G_POP, map, hit_x, hit_y, CityTileManager::maximize_production);
    std::snprintf(path, sizeof(path), "%s/commerce.ppm", G_OUT_DIR);
    run_sel("maximize_commerce", path, player, city_idx, G_POP, map, hit_x, hit_y, CityTileManager::maximize_commerce);
    TileYields::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    CityTileManager::bind_cities(nullptr);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
