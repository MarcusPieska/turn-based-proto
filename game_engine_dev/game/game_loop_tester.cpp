//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game_loop.h"
#include "game_loop_cache.h"
#include "game_setup.h"
#include "game_state.h"
#include "map_config.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_CACHE_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_TRACE_PATH = "/home/w/Projects/simple-map-gen/game_loop.trace";
static u32 g_seed = 101u;
static const u16 g_players = 4;

static char g_map_path[384];
static char g_starts_path[384];

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
        total_test_fails++;
        if (print_level > 0) {
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

static bool build_cache_paths () {
    if (std::snprintf(g_map_path, sizeof(g_map_path),
            "%s/game-loop-seed-%u-p%u-map.bin", G_CACHE_ROOT, g_seed, g_players) <= 0) {
        return false;
    }
    if (std::snprintf(g_starts_path, sizeof(g_starts_path),
            "%s/game-loop-seed-%u-p%u-starts.bin", G_CACHE_ROOT, g_seed, g_players) <= 0) {
        return false;
    }
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

static bool is_settler_typ (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = st.unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(st.unit_type().get_name(tk), "LAND_SETTLER") == 0;
}

static u16 count_settlers_in_unit_array (const GameState& state) {
    if (state.m_statics == nullptr) {
        return 0;
    }
    u16 n = 0;
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (unit == nullptr) {
            continue;
        }
        if (is_settler_typ(*state.m_statics, unit->m_unit_typ_idx)) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static void print_unit_array_slots (const GameState& state) {
    if (print_level < 2) {
        return;
    }
    std::printf("--- unit array slots ---\n");
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (unit == nullptr) {
            continue;
        }
        std::printf("  slot %u player=%u typ=%u xy=(%u,%u) health=%u level=%u\n",
            idx, unit->m_player_idx, unit->m_unit_typ_idx,
            unit->m_x, unit->m_y, unit->m_health, unit->m_level);
    }
    std::printf("------------------------\n");
}

static bool prepare_state (GameState* state) {
    GameSetup setup;
    if (GameLoopCache::map_exists(g_map_path, g_starts_path)) {
        if (print_level > 0) {
            std::printf("--- loading loop cache ---\n");
        }
        return setup.setup_from_cache(state, g_map_path, g_starts_path, g_players);
    }
    if (print_level > 0) {
        std::printf("--- generating map and saving loop cache ---\n");
    }
    MapGenReq req = {};
    req.m_seed = g_seed;
    req.m_type = MAP_CONTINENTAL;
    req.m_w = 1000u;
    req.m_h = 1000u;
    req.m_cfg = map_config_def();
    if (!setup.setup_new_game(state, req, g_players)) {
        return false;
    }
    setup.release_map_gen();
    SpgPickCoords starts = {};
    if (!setup.pick_starts(state->m_map, g_players, &starts)) {
        state->clear();
        return false;
    }
    if (!GameLoopCache::save_map(g_map_path, state->m_map)
        || !GameLoopCache::save_starts(g_starts_path, starts, g_players)) {
        state->clear();
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_game_loop_fast_path () {
    if (!build_cache_paths()) {
        note_result(false, "build cache paths");
        summarize_test_results();
        return;
    }
    GameState state;
    note_result(prepare_state(&state), "prepare game state");
    note_result(state.m_map.width() > 0, "map loaded");
    note_result(state.m_player_n == g_players, "players ready");
    GameLoop loop;
    note_result(loop.run(&state, G_TRACE_PATH), "game loop run");
    note_result(state.m_current_turn == state.m_turn_limit, "turn limit reached");
    note_result(count_trace_prefix(G_TRACE_PATH, "NEW_TURN:") == state.m_turn_limit, "trace new turn count");
    if (print_level > 0) {
        std::printf(" trace: %s\n", G_TRACE_PATH);
        std::printf(" map cache: %s\n", g_map_path);
        std::printf(" starts cache: %s\n", g_starts_path);
    }
    state.clear();
    summarize_test_results();
}

void test_settler_count_in_unit_array () {
    if (!build_cache_paths()) {
        note_result(false, "build cache paths");
        summarize_test_results();
        return;
    }
    GameState state;
    note_result(prepare_state(&state), "prepare game state for settler count");
    const u16 settlers = count_settlers_in_unit_array(state);
    note_result(settlers == g_players, "settler count matches player count");
    print_unit_array_slots(state);
    if (print_level > 0) {
        std::printf(" settlers=%u players=%u\n", settlers, g_players);
    }
    state.clear();
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
        g_seed = static_cast<u32>(std::strtoul(argv[2], nullptr, 10));
    }
    test_game_loop_fast_path();
    test_settler_count_in_unit_array();
    std::printf("=======================================================\n");
    std::printf(" TESTING GAME LOOP: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
