//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "city.h"
#include "city_array.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "assert_log.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "tile_working.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-tile-manager-count-worked";
static const char* G_TILES = "/home/w/Projects/simple-map-gen/city-tile-manager-count-worked/map_tiles.bin";
static const char* G_CITIES = "/home/w/Projects/simple-map-gen/city-tile-manager-count-worked/cities.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/city-tile-manager-count-worked/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100u;
static const u32 G_TURN_CAP = 300u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_flags[320];

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
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_flags, sizeof(g_flags), "%s/flags.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool ensure_out_dir () {
    if (mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static bool file_exists (cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static bool save_cities (const GameState& state) {
    std::FILE* fp = std::fopen(G_CITIES, "w");
    if (fp == nullptr) {
        return false;
    }
    const u16 cn = state.m_cities.get_city_count();
    u32 n = 0;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        std::fprintf(fp, "%u:%u:%u\n",
            static_cast<unsigned>(i),
            static_cast<unsigned>(c->get_x()),
            static_cast<unsigned>(c->get_y()));
        ++n;
    }
    std::fclose(fp);
    std::printf("saved cities=%u\n", static_cast<unsigned>(n));
    return n > 0;
}

static bool load_map_tiles (GameArraySimple* map) {
    if (map == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(G_TILES, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&w, sizeof(w), 1, fp) != 1
        || std::fread(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != 0x534c4954u || ver != 2u || w == 0 || h == 0) {
        std::fclose(fp);
        return false;
    }
    if (w != map->width() || h != map->height()) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    if (std::fread(tiles, sizeof(GameTileSimple), n, fp) != n) {
        delete[] tiles;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            GameTileSimple* dst = map->tile(x, y);
            if (dst == nullptr) {
                delete[] tiles;
                return false;
            }
            *dst = tiles[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
        }
    }
    delete[] tiles;
    return true;
}

struct CityLoc {
    u16 m_idx;
    u16 m_x;
    u16 m_y;
};

static bool load_city_locs (CityLoc* out, u16 cap, u16* out_n) {
    std::FILE* fp = std::fopen(G_CITIES, "r");
    if (fp == nullptr || out == nullptr || out_n == nullptr) {
        return false;
    }
    u16 n = 0;
    unsigned idx = 0;
    unsigned x = 0;
    unsigned y = 0;
    while (n < cap && std::fscanf(fp, "%u:%u:%u\n", &idx, &x, &y) == 3) {
        out[n].m_idx = static_cast<u16>(idx);
        out[n].m_x = static_cast<u16>(x);
        out[n].m_y = static_cast<u16>(y);
        ++n;
    }
    std::fclose(fp);
    *out_n = n;
    return n > 0;
}

static bool run_warmup (GameState& state) {
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        return false;
    }
    while (state.m_current_turn < G_TURN_CAP) {
        if (!loop.step()) {
            break;
        }
        std::printf("\rwarmup turn %u / %u", state.m_current_turn, G_TURN_CAP);
        std::fflush(stdout);
    }
    std::printf("\n");
    loop.end();
    if (state.m_current_turn < G_TURN_CAP) {
        std::printf("warmup stopped early at turn %u\n", state.m_current_turn);
    }
    return GameIo::save_map_tiles(G_TILES, state.m_map) && save_cities(state);
}

static void time_baseline (const CityLoc* locs, u16 n, u32* out_cnt) {
    double min_ns = 1.0e300;
    double max_ns = 0.0;
    double sum_ns = 0.0;
    u32 total_worked = 0;
    for (u16 i = 0; i < n; ++i) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        const u32 worked = CityTileManager::count_worked(locs[i].m_x, locs[i].m_y, locs[i].m_idx);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const double dt = std::chrono::duration<double, std::nano>(t1 - t0).count();
        if (dt < min_ns) {
            min_ns = dt;
        }
        if (dt > max_ns) {
            max_ns = dt;
        }
        sum_ns += dt;
        total_worked += worked;
        if (out_cnt != nullptr) {
            out_cnt[i] = worked;
        }
    }
    const double avg_ns = sum_ns / static_cast<double>(n);
    std::printf("CityTileManager::count_worked cities=%u total_worked=%u\n",
        static_cast<unsigned>(n),
        static_cast<unsigned>(total_worked));
    std::printf("CityTileManager::count_worked ns: min=%.2f max=%.2f avg=%.2f\n", min_ns, max_ns, avg_ns);
}

static void time_map_opt (GameArraySimple& map, const CityLoc* locs, u16 n, const u32* expect) {
    const CircArea area = CityTileManager::work_area();
    const u16 r = 4;
    double min_ns = 1.0e300;
    double max_ns = 0.0;
    double sum_ns = 0.0;
    u32 total_worked = 0;
    GAME_EXPECT(expect != nullptr, "count_worked expect");
    for (u16 i = 0; i < n; ++i) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        const u32 worked = map.count_worked(locs[i].m_x, locs[i].m_y, locs[i].m_idx, area.m_brd, area.m_lim, r);
        const auto t1 = std::chrono::high_resolution_clock::now();
        const double dt = std::chrono::duration<double, std::nano>(t1 - t0).count();
        if (dt < min_ns) {
            min_ns = dt;
        }
        if (dt > max_ns) {
            max_ns = dt;
        }
        sum_ns += dt;
        total_worked += worked;
        GAME_EXPECT(worked == expect[i], "count_worked mismatch");
    }
    const double avg_ns = sum_ns / static_cast<double>(n);
    std::printf("GameArraySimple::count_worked cities=%u total_worked=%u\n",
        static_cast<unsigned>(n),
        static_cast<unsigned>(total_worked));
    std::printf("GameArraySimple::count_worked ns: min=%.2f max=%.2f avg=%.2f\n", min_ns, max_ns, avg_ns);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("path/out failed\n");
        return 1;
    }

    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    paths.m_flags = g_flags;

    CityLoc locs[4096];
    u16 loc_n = 0;

    const bool have_cache = file_exists(G_TILES) && file_exists(G_CITIES);
    if (have_cache) {
        std::printf("cache hit: loading map tiles + cities\n");
        if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
            std::printf("setup_new_game failed\n");
            return 1;
        }
        if (!load_map_tiles(&state.m_map)) {
            std::printf("FAIL: load map tiles\n");
            state.clear();
            return 1;
        }
        TileWorking::bind_map(&state.m_map);
        TileYields::bind_map(&state.m_map);
        if (!load_city_locs(locs, 4096, &loc_n)) {
            std::printf("FAIL: load cities\n");
            state.clear();
            return 1;
        }
    } else {
        std::printf("cache miss: running %u turns with %u players\n", G_TURN_CAP, G_PLAYERS);
        if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
            std::printf("setup_new_game failed\n");
            return 1;
        }
        if (!run_warmup(state)) {
            std::printf("FAIL: warmup/save\n");
            state.clear();
            return 1;
        }
        const u16 cn = state.m_cities.get_city_count();
        for (u16 i = 0; i < cn && loc_n < 4096; ++i) {
            const City* c = state.m_cities.get_city(i);
            if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
                continue;
            }
            locs[loc_n].m_idx = i;
            locs[loc_n].m_x = c->get_x();
            locs[loc_n].m_y = c->get_y();
            ++loc_n;
        }
        TileWorking::bind_map(&state.m_map);
        TileYields::bind_map(&state.m_map);
        std::printf("saved %s and %s\n", G_TILES, G_CITIES);
    }

    std::printf("--- phase 2: time count_worked (baseline then map opt) ---\n");
    if (loc_n == 0) {
        std::printf("no cities to time\n");
        state.clear();
        return 1;
    }
    u32 expect[4096];
    time_baseline(locs, loc_n, expect);
    time_map_opt(state.m_map, locs, loc_n, expect);
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
