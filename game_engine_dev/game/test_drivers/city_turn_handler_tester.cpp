//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "city.h"
#include "city_tile_manager.h"
#include "city_tracer.h"
#include "city_turn_handler.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-turn-handler-test";
static const char* G_CITY_LOG = "/home/w/Projects/simple-map-gen/city-turn-handler-test/cities.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 4;
static const u32 G_TURNS = 1000u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];

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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
#ifndef CITY_TRACER_ENABLE
    std::printf("CITY_TRACER_ENABLE not set; rebuild with -DCITY_TRACER_ENABLE\n");
    return 1;
#endif
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("out dir failed\n");
        return 1;
    }

    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = nullptr;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        return 1;
    }
    const u16 city_n = state.m_cities.get_city_count();
    for (u16 i = 0; i < city_n; ++i) {
        City* city = state.m_cities.get_city(i);
        if (city == nullptr) {
            continue;
        }
        CityTileManager::maximize_food(city->get_owner(), i);
    }
    for (u16 p = 0; p < state.m_player_n; ++p) {
        state.m_player_states[p].m_target_settlements = 0;
    }
    WhiteboardMng::terminate();
    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    if (!SettlerTurnHandler::begin(state)) {
        std::printf("SettlerTurnHandler::begin failed\n");
        state.clear();
        setup.release_map_gen();
        return 1;
    }

    LOG_CITY_SETUP((G_CITY_LOG));
    i64 handle_ns = 0;
    for (u32 turn = 0; turn < G_TURNS; ++turn) {
        state.m_current_turn = turn + 1;
        for (u16 i = 0; i < city_n; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            CityTurnHandler::handle(state, i);
            const auto t1 = std::chrono::steady_clock::now();
            handle_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        }
        LOG_CITY_FLUSH(());
    }
    LOG_CITY_CLEAR(());
    SettlerTurnHandler::clear();
    WhiteboardMng::terminate();

    state.clear();
    setup.release_map_gen();

    const f64 handle_ms = static_cast<f64>(handle_ns) / 1.0e6;
    const f64 per_call_ns = city_n > 0 ? static_cast<f64>(handle_ns) / static_cast<f64>(G_TURNS * static_cast<u32>(city_n)) : 0.0;
    std::printf("CityTurnHandler::handle total: %.3f ms over %u turns x %u cities\n", handle_ms, G_TURNS, city_n);
    std::printf("CityTurnHandler::handle mean: %.1f ns/call\n", per_call_ns);
    std::printf("cities.trace: %s\n", G_CITY_LOG);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
