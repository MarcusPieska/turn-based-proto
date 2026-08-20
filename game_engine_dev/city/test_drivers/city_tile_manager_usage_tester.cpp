//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cerrno>
#include <random>
#include <sys/stat.h>
#include <vector>

#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "build_adds_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_usage.h"
#include "tile_working.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;
typedef TotalTileYield (*AssignFn) (u16 player, u16 city_idx);
typedef TotalTileYield (*StableFn) (u16 player, u16 city_idx, u16 start_food, u16 sanitation_boost);

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_LOG = "/home/w/Projects/simple-map-gen/city-tile-manager-usage-test/results.txt";
static const u32 G_SEED = 43u;
static const u32 G_LOC_N = 10u;
static const u16 G_POP_LO = 5u;
static const u16 G_POP_HI = 20u;
static const u16 G_CLAIM_CULT = 150u;
static const u16 G_EDGE_PAD = 4u;
static const u16 G_MIN_REACH = 12u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];

static int g_test_n = 0;
static int g_test_pass = 0;
static FILE* g_log = nullptr;

struct AssignCase {
    cstr m_nm;
    u8 m_food_intent;
    AssignFn m_fn;
    StableFn m_stable_fn;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void log_printf (cstr fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    if (g_log == nullptr) {
        return;
    }
    va_start(ap, fmt);
    std::vfprintf(g_log, fmt, ap);
    va_end(ap);
}

static void note (bool ok, cstr msg) {
    ++g_test_n;
    if (ok) {
        ++g_test_pass;
        log_printf("  PASS: %s\n", msg);
    } else {
        log_printf("  FAIL: %s\n", msg);
    }
}

static cstr terr_nm (u8 id) {
    if (id == TERR_OCEAN[0]) {
        return "TERR_OCEAN";
    }
    if (id == TERR_SEA[0]) {
        return "TERR_SEA";
    }
    if (id == TERR_COASTAL[0]) {
        return "TERR_COASTAL";
    }
    if (id == TERR_PLAINS[0]) {
        return "TERR_PLAINS";
    }
    if (id == TERR_HILLS[0]) {
        return "TERR_HILLS";
    }
    if (id == TERR_MOUNTAINS[0]) {
        return "TERR_MOUNTAINS";
    }
    if (id == TERR_VOLCANO[0]) {
        return "TERR_VOLCANO";
    }
    return "TERR_UNKNOWN";
}

static cstr clim_nm (u8 id) {
    if (id == CLIMATE_NONE) {
        return "CLIMATE_NONE";
    }
    if (id == CLIMATE_PLAINS) {
        return "CLIMATE_PLAINS";
    }
    if (id == CLIMATE_DESERT) {
        return "CLIMATE_DESERT";
    }
    if (id == CLIMATE_GRASSLAND) {
        return "CLIMATE_GRASSLAND";
    }
    if (id == CLIMATE_BLACK_SOIL) {
        return "CLIMATE_BLACK_SOIL";
    }
    return "CLIMATE_UNKNOWN";
}

static cstr ov_nm (u8 id) {
    if (id == OV_NONE[0]) {
        return "none";
    }
    if (id == OV_FOREST[0]) {
        return "OV_FOREST";
    }
    if (id == OV_SWAMP[0]) {
        return "OV_SWAMP";
    }
    if (id == OV_JUNGLE[0]) {
        return "OV_JUNGLE";
    }
    if (id == OV_GLACIER[0]) {
        return "OV_GLACIER";
    }
    return "OV_UNKNOWN";
}

static cstr intent_nm (u8 u) {
    if (u == TILE_ASSIGN_FOOD) {
        return "food";
    }
    if (u == TILE_ASSIGN_PROD) {
        return "prod";
    }
    return "unknown";
}

static cstr res_nm (const RuntimeStatics& st, u16 ri) {
    if (ri == U16_KEY_NULL) {
        return "none";
    }
    const ResourceStaticData& rs = st.resource();
    if (ri >= rs.get_item_count()) {
        return "invalid";
    }
    cstr nm = rs.get_name(ResourceStaticDataKey::from_raw(ri));
    return nm != nullptr ? nm : "unknown";
}

static void log_worked_tiles (
    const GameArraySimple& map,
    const RuntimeStatics& st,
    u16 cx,
    u16 cy,
    u16 city_idx)
{
    if (g_log == nullptr) {
        return;
    }
    const CircArea area = CircularTileAreas::get(4);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= map.width() || uy >= map.height()) {
            continue;
        }
        if (map.get_city_worker(ux, uy) != city_idx) {
            continue;
        }
        const u16 ri = map.get_res(ux, uy);
        std::fprintf(g_log,
            "    (%u,%u) terr=%s clim=%s res=%s ov=%s intent=%s\n",
            (unsigned)ux,
            (unsigned)uy,
            terr_nm(map.get_terrain(ux, uy)),
            clim_nm(map.get_climate(ux, uy)),
            res_nm(st, ri),
            ov_nm(map.get_overlay(ux, uy)),
            intent_nm(map.get_tile_usage(ux, uy)));
    }
}

static bool ensure_log_dir () {
    char dir[384];
    if (std::snprintf(dir, sizeof(dir), "%s", G_OUT_LOG) <= 0) {
        return false;
    }
    char* slash = std::strrchr(dir, '/');
    if (slash == nullptr) {
        return true;
    }
    *slash = '\0';
    return ::mkdir(dir, 0755) == 0 || errno == EEXIST;
}

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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool land_terr (u8 t) {
    return t == TERR_PLAINS[0] || t == TERR_HILLS[0] || t == TERR_MOUNTAINS[0];
}

static u16 count_reach_land (const GameArraySimple& map, u16 cx, u16 cy) {
    const CircArea area = CircularTileAreas::get(4);
    u16 n = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= map.width() || uy >= map.height()) {
            continue;
        }
        if (ux == cx && uy == cy) {
            continue;
        }
        if (land_terr(map.get_terrain(ux, uy))) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static bool pick_locs (const GameArraySimple& map, std::mt19937* rng, u16* ox, u16* oy, u32 n) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 got = 0;
    u32 tries = 0;
    while (got < n && tries < 50000u) {
        ++tries;
        const u16 x = static_cast<u16>((*rng)() % static_cast<u32>(w - 2u * G_EDGE_PAD)) + G_EDGE_PAD;
        const u16 y = static_cast<u16>((*rng)() % static_cast<u32>(h - 2u * G_EDGE_PAD)) + G_EDGE_PAD;
        if (!land_terr(map.get_terrain(x, y))) {
            continue;
        }
        if (count_reach_land(map, x, y) < G_MIN_REACH) {
            continue;
        }
        u8 dup = 0;
        for (u32 i = 0; i < got; ++i) {
            if (ox[i] == x && oy[i] == y) {
                dup = 1;
                break;
            }
        }
        if (dup != 0) {
            continue;
        }
        ox[got] = x;
        oy[got] = y;
        got = got + 1u;
    }
    return got == n;
}

static bool reload_map (GameArraySimple* map) {
    map->clear();
    if (!Factory_GameArraySimple::load_map_gen_data(map, g_terr, g_clim, g_riv, nullptr)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(map, g_res)) {
        return false;
    }
    TileYields::bind_map(map);
    TileWorking::bind_map(map);
    CityBorder::bind_map(map);
    return true;
}

static void preset_usage_sentinel (GameArraySimple& map, u16 cx, u16 cy, u8 sentinel) {
    const CircArea area = CircularTileAreas::get(4);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= map.width() || uy >= map.height()) {
            continue;
        }
        map.set_tile_usage(ux, uy, sentinel);
        map.set_city_worker(ux, uy, U16_KEY_NULL);
    }
}

static bool verify_stamped (const GameArraySimple& map, u16 cx, u16 cy, u16 city_idx, u8 sentinel, u32* worked) {
    *worked = 0;
    const CircArea area = CircularTileAreas::get(4);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= map.width() || uy >= map.height()) {
            continue;
        }
        if (map.get_city_worker(ux, uy) != city_idx) {
            continue;
        }
        *worked = *worked + 1u;
        if (map.get_tile_usage(ux, uy) == sentinel) {
            return false;
        }
    }
    return true;
}

static bool setup_city (
    GameArraySimple& map,
    CityArray& cities,
    u16 player,
    u16 x,
    u16 y,
    u16 pop,
    u16 city_idx)
{
    City* city = cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(player, x, y);
    city->set_population(pop);
    if (!map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY)) {
        return false;
    }
    CityBorder::claim_expand(x, y, 0, G_CLAIM_CULT, static_cast<u8>(player));
    CityTileManager::clear(x, y, city_idx);
    return true;
}

static void run_assign_case (
    GameArraySimple& map,
    CityArray& cities,
    const RuntimeStatics& st,
    u16 player,
    u16 loc_i,
    u16 cx,
    u16 cy,
    u16 city_idx,
    u16 pop,
    const AssignCase& tc)
{
    if (!reload_map(&map)) {
        note(false, "reload map");
        return;
    }
    if (!setup_city(map, cities, player, cx, cy, pop, city_idx)) {
        note(false, "setup city");
        return;
    }
    const u8 sentinel = tc.m_food_intent != 0 ? TILE_ASSIGN_PROD : TILE_ASSIGN_FOOD;
    preset_usage_sentinel(map, cx, cy, sentinel);
    if (tc.m_stable_fn != nullptr) {
        const u16 start_food = TileYields::get(cx, cy).m_food;
        const u16 sanit = 3u;
        tc.m_stable_fn(player, city_idx, start_food, sanit);
    } else {
        tc.m_fn(player, city_idx);
    }
    u32 worked = 0;
    char msg[160];
    const bool sentinel_ok = verify_stamped(map, cx, cy, city_idx, sentinel, &worked);
    const bool full = tc.m_stable_fn == nullptr || worked >= static_cast<u32>(pop);
    const bool stamped = sentinel_ok || !full;
    std::snprintf(msg, sizeof(msg), "loc=%u pop=%u %s worked=%u stamped", (u32)loc_i, (u32)pop, tc.m_nm, (u32)worked);
    note(stamped, msg);
    log_worked_tiles(map, st, cx, cy, city_idx);
    if (worked == 0 && pop > 0) {
        std::snprintf(msg, sizeof(msg), "loc=%u %s worked>0", (unsigned)loc_i, tc.m_nm);
        note(count_reach_land(map, cx, cy) <= pop, msg);
    }
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    if (!ensure_log_dir()) {
        std::printf("fail mkdir log dir\n");
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
    if (!reload_map(&map)) {
        std::printf("fail load map\n");
        return 1;
    }
    CityArray cities;
    if (!cities.bind_statics(st)) {
        std::printf("fail bind city array\n");
        return 1;
    }
    CityTileManager::bind_cities(&cities);
    const u16 city_idx = cities.get_next_new_city_idx();
    if (cities.get_city(city_idx) == nullptr) {
        std::printf("fail alloc city\n");
        return 1;
    }

    std::mt19937 rng(90210u);
    std::uniform_int_distribution<u16> pop_dist(G_POP_LO, G_POP_HI);
    u16 loc_x[G_LOC_N];
    u16 loc_y[G_LOC_N];
    if (!pick_locs(map, &rng, loc_x, loc_y, G_LOC_N)) {
        std::printf("fail pick %u locations\n", (unsigned)G_LOC_N);
        return 1;
    }
    g_log = std::fopen(G_OUT_LOG, "w");
    if (g_log == nullptr) {
        std::printf("fail open log %s\n", G_OUT_LOG);
        return 1;
    }
    log_printf("tile intent stamp test: %u locations seed=%u\n", (unsigned)G_LOC_N, (unsigned)G_SEED);
    log_printf("log: %s\n", G_OUT_LOG);

    const AssignCase cases[] = {
        {"maximize_food", 1, CityTileManager::maximize_food, nullptr},
        {"maximize_production", 0, CityTileManager::maximize_production, nullptr},
        {"maximize_commerce", 0, CityTileManager::maximize_commerce, nullptr},
        {"add_new_food_tile", 1, CityTileManager::add_new_food_tile, nullptr},
        {"add_new_production_tile", 0, CityTileManager::add_new_production_tile, nullptr},
        {"add_new_commerce_tile", 0, CityTileManager::add_new_commerce_tile, nullptr},
        {"stable_food_max_production", 0, nullptr, CityTileManager::stable_food_max_production},
        {"stable_food_max_commerce", 0, nullptr, CityTileManager::stable_food_max_commerce},
        {"stable_food_max_combined", 0, nullptr, CityTileManager::stable_food_max_combined},
    };
    const u32 case_n = static_cast<u32>(sizeof(cases) / sizeof(cases[0]));
    const u16 player = 0;

    for (u32 li = 0; li < G_LOC_N; ++li) {
        const u16 pop = pop_dist(rng);
        log_printf("--- location %u (%u,%u) pop=%u ---\n",
            (unsigned)li, (unsigned)loc_x[li], (unsigned)loc_y[li], (unsigned)pop);
        for (u32 ci = 0; ci < case_n; ++ci) {
            run_assign_case(map, cities, st, player, static_cast<u16>(li), loc_x[li], loc_y[li], city_idx, pop, cases[ci]);
        }
    }

    log_printf("=======================================================\n");
    log_printf(" TILE INTENT STAMP: %d/%d pass\n", g_test_pass, g_test_n);
    log_printf("=======================================================\n");
    std::fclose(g_log);
    g_log = nullptr;
    CityTileManager::bind_cities(nullptr);
    CityBorder::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    TileYields::bind_map(nullptr);
    map.clear();
    return (g_test_pass == g_test_n) ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
