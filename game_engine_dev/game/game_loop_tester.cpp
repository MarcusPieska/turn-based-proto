//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "game_loop.h"
#include "game_loop_cache.h"
#include "game_io.h"
#include "game_setup.h"
#include "game_state.h"
#include "city.h"
#include "map_config.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "profile_time_opt.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_CACHE_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_SAVES_ROOT = "/home/w/Projects/game-saves";
static const char* G_TRACE_PATH = "/home/w/Projects/simple-map-gen/game_loop.trace";
static u32 g_seed = 101u;
static u16 g_players = 0;
static u32 g_turn_limit = 0;
static u32 g_save_every = 0;

static char g_map_path[384];
static char g_starts_path[384];
static char g_end_map_path[384];
static char g_end_units_path[384];
static char g_end_cities_path[384];
static char g_end_players_path[384];
static char g_turn_ms_path[384];

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

static bool parse_u32 (cstr s, u32* out) {
    if (s == nullptr || out == nullptr) {
        return false;
    }
    errno = 0;
    char* end = nullptr;
    const unsigned long v = std::strtoul(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v > 0xfffffffful) {
        return false;
    }
    *out = static_cast<u32>(v);
    return true;
}

static void print_usage (cstr prog) {
    std::printf("usage: %s <players> <turns> <save_interval>\n", prog != nullptr ? prog : "game_loop_tester");
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

static bool build_state_paths (u32 turn) {
    if (std::snprintf(g_end_map_path, sizeof(g_end_map_path),
            "%s/game-loop-seed-%u-p%u-%04u.bin", G_SAVES_ROOT, g_seed, g_players, turn) <= 0) {
        return false;
    }
    if (std::snprintf(g_end_units_path, sizeof(g_end_units_path),
            "%s/game-loop-seed-%u-p%u-%04u-units.bin", G_SAVES_ROOT, g_seed, g_players, turn) <= 0) {
        return false;
    }
    if (std::snprintf(g_end_cities_path, sizeof(g_end_cities_path),
            "%s/game-loop-seed-%u-p%u-%04u-cities.bin", G_SAVES_ROOT, g_seed, g_players, turn) <= 0) {
        return false;
    }
    if (std::snprintf(g_end_players_path, sizeof(g_end_players_path),
            "%s/game-loop-seed-%u-p%u-%04u-players.bin", G_SAVES_ROOT, g_seed, g_players, turn) <= 0) {
        return false;
    }
    if (std::snprintf(g_turn_ms_path, sizeof(g_turn_ms_path),
            "%s/game-loop-seed-%u-p%u-turn-ms.txt", G_SAVES_ROOT, g_seed, g_players) <= 0) {
        return false;
    }
    return true;
}

static bool ensure_saves_dir () {
    return ::mkdir(G_SAVES_ROOT, 0755) == 0 || errno == EEXIST;
}

static bool save_turn_state (GameState& state, u32 turn) {
    if (!ensure_saves_dir()) {
        return false;
    }
    if (!build_state_paths(turn)) {
        return false;
    }
    if (!GameIo::save_map_tiles(g_end_map_path, state.m_map)) {
        return false;
    }
    if (!GameIo::save_units(g_end_units_path, state.m_units)) {
        return false;
    }
    if (!GameIo::save_cities(g_end_cities_path, state.m_cities)) {
        return false;
    }
    if (!GameIo::save_players(g_end_players_path, state)) {
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
        std::printf("prepare_state: setup_new_game failed\n");
        return false;
    }
    setup.release_map_gen();
    SpgPickCoords starts = {};
    starts.n = static_cast<u32>(g_players);
    for (u16 p = 0; p < g_players; ++p) {
        starts.pts[p].x = U16_KEY_NULL;
        starts.pts[p].y = U16_KEY_NULL;
    }
    const u16 cn = state->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state->m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        const u16 seat = c->get_owner();
        if (seat >= g_players) {
            continue;
        }
        if (starts.pts[seat].x != U16_KEY_NULL) {
            continue;
        }
        starts.pts[seat].x = c->get_x();
        starts.pts[seat].y = c->get_y();
    }
    for (u16 p = 0; p < g_players; ++p) {
        if (starts.pts[p].x == U16_KEY_NULL || starts.pts[p].y == U16_KEY_NULL) {
            std::printf("prepare_state: missing start city for player %u\n", p);
            state->clear();
            return false;
        }
    }
    if (!GameLoopCache::save_map(g_map_path, state->m_map)
        || !GameLoopCache::save_starts(g_starts_path, starts, g_players)) {
        std::printf("prepare_state: cache save failed\n");
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
    if (!prepare_state(&state)) {
        note_result(false, "prepare game state");
        summarize_test_results();
        return;
    }
    note_result(true, "prepare game state");
    state.m_turn_limit = g_turn_limit;
    note_result(state.m_map.width() > 0, "map loaded");
    note_result(state.m_player_n == g_players, "players ready");
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE_PATH)) {
        note_result(false, "game loop begin");
        state.clear();
        summarize_test_results();
        return;
    }
    note_result(true, "game loop begin");
    note_result(build_state_paths(state.m_turn_limit), "build state save paths");
    std::FILE* ms_fp = std::fopen(g_turn_ms_path, "w");
    note_result(ms_fp != nullptr, "open turn ms file");
    while (state.m_current_turn < state.m_turn_limit) {
        const auto t0 = std::chrono::steady_clock::now();
        if (!loop.step()) {
            break;
        }
        const auto t1 = std::chrono::steady_clock::now();
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
        if (ms_fp != nullptr) {
            std::fprintf(ms_fp, "%lld\n", static_cast<long long>(ms));
        }
        std::printf("\rturn %u / %u", state.m_current_turn, state.m_turn_limit);
        std::fflush(stdout);
        if (g_save_every != 0 && (state.m_current_turn % g_save_every) == 0) {
            note_result(save_turn_state(state, state.m_current_turn), "save interval state");
        }
    }
    std::printf("\n");
    if (ms_fp != nullptr) {
        std::fclose(ms_fp);
    }
    note_result(state.m_current_turn == state.m_turn_limit, "turn limit reached");
    note_result(count_trace_prefix(G_TRACE_PATH, "NEW_TURN:") == state.m_turn_limit, "trace new turn count");
    note_result(save_turn_state(state, state.m_current_turn), "save end state");
    if (print_level > 0) {
        std::printf(" trace: %s\n", G_TRACE_PATH);
        std::printf(" map cache: %s\n", g_map_path);
        std::printf(" starts cache: %s\n", g_starts_path);
        std::printf(" end map: %s\n", g_end_map_path);
        std::printf(" end units: %s\n", g_end_units_path);
        std::printf(" end cities: %s\n", g_end_cities_path);
        std::printf(" end players: %s\n", g_end_players_path);
        std::printf(" turn ms: %s\n", g_turn_ms_path);
    }
    loop.end();
    PTO_PRINT();
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
    u32 players = 0;
    if (argc != 4
        || !parse_u32(argv[1], &players)
        || !parse_u32(argv[2], &g_turn_limit)
        || !parse_u32(argv[3], &g_save_every)
        || players == 0
        || players > 0xffffu
        || g_turn_limit == 0) {
        print_usage(argc > 0 ? argv[0] : nullptr);
        return 1;
    }
    g_players = static_cast<u16>(players);
    std::printf("players=%u turns=%u save_interval=%u\n", g_players, g_turn_limit, g_save_every);
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
