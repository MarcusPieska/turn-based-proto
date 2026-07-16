//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game_setup.h"
#include "game_state.h"
#include "game_loop.h"
#include "config_settings_static.h"
#include "map_config.h"
#include "runtime_statics.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_TRACE_PATH = "game_setup_loop.trace";
static u32 g_setup_seed = 101u;
static char g_setup_terr[320];
static char g_setup_clim[320];
static char g_setup_riv[320];
static char g_setup_res[320];

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        if (print_level > 0) {
            total_test_fails++;
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

void summarize_test_results () {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

static bool build_setup_paths (u32 seed) {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, seed) <= 0) {
        return false;
    }
    if (std::snprintf(g_setup_terr, sizeof(g_setup_terr), "%s/terrain.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_setup_clim, sizeof(g_setup_clim), "%s/climate.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_setup_riv, sizeof(g_setup_riv), "%s/rivers.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_setup_res, sizeof(g_setup_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static MapPpmPaths make_paths () {
    MapPpmPaths paths = {};
    if (!build_setup_paths(g_setup_seed)) {
        return paths;
    }
    paths.m_terr = g_setup_terr;
    paths.m_clim = g_setup_clim;
    paths.m_riv = g_setup_riv;
    paths.m_ov = nullptr;
    paths.m_res = g_setup_res;
    return paths;
}

static bool count_start_units (const GameState& state, u16 expect_players) {
    if (state.m_statics == nullptr) {
        return false;
    }
    const ConfigListUnit& su = state.m_statics->config().get_start_units();
    if (su.n == 0) {
        return false;
    }
    const u16 tile_typ = su.keys[0].value();
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u16 unit_tiles = 0;
    u16 seen[256] = {};
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 hd = state.m_map.get_unit_hd(x, y);
            if (hd == U16_KEY_NULL) {
                continue;
            }
            unit_tiles = static_cast<u16>(unit_tiles + 1);
            const UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(hd));
            if (unit == nullptr || unit->m_player_idx >= expect_players) {
                return false;
            }
            if (unit->m_unit_typ_idx != tile_typ) {
                return false;
            }
            seen[unit->m_player_idx] = 1;
        }
    }
    if (unit_tiles != expect_players) {
        return false;
    }
    for (u16 i = 0; i < expect_players; ++i) {
        if (seen[i] == 0) {
            return false;
        }
    }
    return true;
}

static void print_unit_vector_state (const GameState& state) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::printf("--- unit vector (map-linked) ---\n");
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 hd = state.m_map.get_unit_hd(x, y);
            if (hd == U16_KEY_NULL) {
                continue;
            }
            const UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(hd));
            if (unit == nullptr) {
                std::printf("  tile (%u,%u) key=%u MISSING\n", x, y, hd);
                continue;
            }
            std::printf("  tile (%u,%u) key=%u player=%u typ=%u health=%u level=%u\n",
                x, y, hd, unit->m_player_idx, unit->m_unit_typ_idx, unit->m_health, unit->m_level);
        }
    }
    std::printf("--------------------------------\n");
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_setup_new_game () {
    GameSetup setup;
    GameState state;
    MapGenReq req = {};
    req.m_seed = g_setup_seed;
    req.m_type = MAP_CONTINENTAL;
    req.m_w = 1000u;
    req.m_h = 1000u;
    req.m_cfg = map_config_def();
    const u16 players = 4;
    const bool ok = setup.setup_new_game(&state, req, players);
    note_result(ok, "setup_new_game generated");
    note_result(state.m_map.width() > 0, "map width");
    note_result(state.m_map.height() > 0, "map height");
    note_result(state.m_player_n == players, "player count");
    note_result(state.m_player_states != nullptr, "player states");
    if (state.m_player_states != nullptr && state.m_player_n > 0) {
        note_result(state.m_player_states[0].m_explored_overlay != nullptr, "explore overlay");
    }
    note_result(state.m_statics != nullptr, "runtime statics");
    note_result(count_start_units(state, players), "start units from config");
    if (ok && print_level > 0) {
        print_unit_vector_state(state);
    }
    state.clear();
    summarize_test_results();
}

void test_setup_new_game_sequential () {
    GameSetup setup;
    GameState state_a;
    GameState state_b;
    MapGenReq req = {};
    req.m_seed = g_setup_seed;
    req.m_type = MAP_CONTINENTAL;
    req.m_w = 1000u;
    req.m_h = 1000u;
    req.m_cfg = map_config_def();
    const u16 players = 2;
    note_result(setup.setup_new_game(&state_a, req, players), "first sequential setup");
    note_result(setup.setup_new_game(&state_b, req, players), "second sequential setup");
    state_a.clear();
    state_b.clear();
    setup.release_map_gen();
    summarize_test_results();
}

static bool ppm_setup_paths_exist () {
    FILE* fp = std::fopen(g_setup_terr, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static u32 count_trace_prefix (cstr path, cstr prefix) {
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        return 0;
    }
    char buf[256];
    u32 n = 0;
    const size_t pre_len = std::strlen(prefix);
    while (std::fgets(buf, static_cast<int>(sizeof(buf)), f) != nullptr) {
        if (std::strncmp(buf, prefix, pre_len) == 0) {
            n = n + 1u;
        }
    }
    std::fclose(f);
    return n;
}

void test_setup_and_game_loop () {
    std::remove(G_TRACE_PATH);
    GameSetup setup;
    GameState state;
    MapGenReq req = {};
    req.m_seed = g_setup_seed;
    req.m_type = MAP_CONTINENTAL;
    req.m_w = 1000u;
    req.m_h = 1000u;
    req.m_cfg = map_config_def();
    const u16 players = 4;
    note_result(setup.setup_new_game(&state, req, players), "setup before loop");
    setup.release_map_gen();
    GameLoop loop;
    note_result(loop.run(&state, G_TRACE_PATH), "game loop run");
    note_result(state.m_current_turn == state.m_turn_limit, "loop reached turn limit");
    note_result(count_trace_prefix(G_TRACE_PATH, "NEW_TURN:") == state.m_turn_limit, "trace new turn count");
    state.clear();
    summarize_test_results();
}

void test_setup_new_game_from_ppm () {
    if (!build_setup_paths(g_setup_seed) || !ppm_setup_paths_exist()) {
        if (print_level > 0) {
            std::printf("--- skip ppm setup test (files missing) ---\n");
        }
        return;
    }
    GameSetup setup;
    GameState state;
    const MapPpmPaths paths = make_paths();
    const u16 players = 4;
    const bool ok = setup.setup_new_game(&state, paths, players);
    note_result(ok, "setup_new_game from ppm");
    state.clear();
    summarize_test_results();
}

void test_setup_new_game_needs_paths () {
    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    note_result(!setup.setup_new_game(&state, paths, 4), "setup rejects null paths");
    summarize_test_results();
}

void test_save_load_stubs () {
    GameSetup setup;
    GameState state;
    note_result(!setup.save_game("/tmp/x", &state), "save_game stub");
    note_result(!setup.load_game("/tmp/x", &state), "load_game stub");
    summarize_test_results();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    if (argc > 2) {
        g_setup_seed = static_cast<u32>(std::strtoul(argv[2], nullptr, 10));
    }
    test_setup_new_game_needs_paths();
    test_setup_new_game();
    test_setup_new_game_sequential();
    test_setup_and_game_loop();
    test_setup_new_game_from_ppm();
    test_save_load_stubs();
    std::printf("=======================================================\n");
    std::printf(" TESTING GAME SETUP: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
