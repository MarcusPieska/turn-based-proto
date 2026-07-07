//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "game_setup.h"
#include "game_state.h"
#include "config_settings_static.h"
#include "runtime_statics.h"
#include "unit_add_vector_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_SETUP_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const char* G_SETUP_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const char* G_SETUP_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const char* G_SETUP_RES = "/home/w/Projects/simple-map-gen/p1-seed-042/25_res_overlay.ppm";

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

static MapPpmPaths make_paths () {
    MapPpmPaths paths = {};
    paths.m_terr = G_SETUP_TERR;
    paths.m_clim = G_SETUP_CLIM;
    paths.m_riv = G_SETUP_RIV;
    paths.m_res = G_SETUP_RES;
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
    const MapPpmPaths paths = make_paths();
    const u16 players = 4;
    const bool ok = setup.setup_new_game(&state, paths, players);
    note_result(ok, "setup_new_game");
    note_result(state.m_map.width() > 0, "map width");
    note_result(state.m_map.height() > 0, "map height");
    note_result(state.m_player_n == players, "player count");
    note_result(state.m_player_states != nullptr, "player states");
    note_result(state.m_tile_ownership_array != nullptr, "ownership overlay");
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
    test_setup_new_game_needs_paths();
    test_setup_new_game();
    test_save_load_stubs();
    std::printf("=======================================================\n");
    std::printf(" TESTING GAME SETUP: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
