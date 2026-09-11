//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "city.h"
#include "city_array.h"
#include "combat_mng.h"
#include "game_loop.h"
#include "game_loop_cache.h"
#include "game_setup.h"
#include "game_state.h"
#include "map_config.h"
#include "mock_muster_siege.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_movement_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_CACHE_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/mock-muster-siege/game_loop.trace";
static const u32 G_SEED = 101u;
static const u16 G_PLAYERS = 10u;
static const u32 G_TURNS = 1000u;
static const u32 G_SIM_EVERY = 10u;

static char g_map_path[384];
static char g_starts_path[384];

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool build_cache_paths () {
    if (std::snprintf(g_map_path, sizeof(g_map_path),
            "%s/game-loop-seed-%u-p%u-map.bin", G_CACHE_ROOT, G_SEED, G_PLAYERS) <= 0) {
        return false;
    }
    if (std::snprintf(g_starts_path, sizeof(g_starts_path),
            "%s/game-loop-seed-%u-p%u-starts.bin", G_CACHE_ROOT, G_SEED, G_PLAYERS) <= 0) {
        return false;
    }
    return true;
}

static bool prepare_state (GameState* state) {
    GameSetup setup;
    if (GameLoopCache::map_exists(g_map_path, g_starts_path)) {
        return setup.setup_from_cache(state, g_map_path, g_starts_path, G_PLAYERS);
    }
    MapGenReq req = {};
    req.m_seed = G_SEED;
    req.m_type = MAP_CONTINENTAL;
    req.m_w = 1000u;
    req.m_h = 1000u;
    req.m_cfg = map_config_def();
    if (!setup.setup_new_game(state, req, G_PLAYERS)) {
        return false;
    }
    setup.release_map_gen();
    SpgPickCoords starts = {};
    starts.n = static_cast<u32>(G_PLAYERS);
    for (u16 p = 0; p < G_PLAYERS; ++p) {
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
        if (seat >= G_PLAYERS || starts.pts[seat].x != U16_KEY_NULL) {
            continue;
        }
        starts.pts[seat].x = c->get_x();
        starts.pts[seat].y = c->get_y();
    }
    for (u16 p = 0; p < G_PLAYERS; ++p) {
        if (starts.pts[p].x == U16_KEY_NULL || starts.pts[p].y == U16_KEY_NULL) {
            state->clear();
            return false;
        }
    }
    if (!GameLoopCache::save_map(g_map_path, state->m_map)
        || !GameLoopCache::save_starts(g_starts_path, starts, G_PLAYERS)) {
        state->clear();
        return false;
    }
    return true;
}

static bool seat_cap_xy (const GameState& state, u16 seat, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        *ox = c->get_x();
        *oy = c->get_y();
        return true;
    }
    return false;
}

static bool pick_lucky_and_near (const GameState& state, u16* out_lucky, u16* out_foe) {
    if (out_lucky == nullptr || out_foe == nullptr || state.m_player_states == nullptr) {
        return false;
    }
    u16 lucky = U16_KEY_NULL;
    for (u16 p = 0; p < state.m_player_n; ++p) {
        if (state.m_player_states[p].m_lucky != 0u && state.m_player_states[p].m_is_active != 0u) {
            lucky = p;
            break;
        }
    }
    if (lucky == U16_KEY_NULL) {
        return false;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    if (!seat_cap_xy(state, lucky, &sx, &sy)) {
        return false;
    }
    u32 best_d = 0xFFFFFFFFu;
    u16 best = U16_KEY_NULL;
    for (u16 e = 0; e < state.m_player_n; ++e) {
        if (e == lucky || state.m_player_states[e].m_lucky != 0u) {
            continue;
        }
        if (state.m_player_states[e].m_is_active == 0u) {
            continue;
        }
        u16 ex = 0u;
        u16 ey = 0u;
        if (!seat_cap_xy(state, e, &ex, &ey)) {
            continue;
        }
        const u32 adx = sx > ex ? static_cast<u32>(sx - ex) : static_cast<u32>(ex - sx);
        const u32 ady = sy > ey ? static_cast<u32>(sy - ey) : static_cast<u32>(ey - sy);
        const u32 d = adx + ady;
        if (d < best_d) {
            best_d = d;
            best = e;
        }
    }
    if (best == U16_KEY_NULL) {
        return false;
    }
    *out_lucky = lucky;
    *out_foe = best;
    return true;
}

static u16 count_owner_cities (const GameState& state, u16 seat) {
    u16 n = 0u;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c != nullptr && c->get_owner() == seat) {
            ++n;
        }
    }
    return n;
}

static void print_sim_row (u32 turn, u16 mustered, u16 taken, u16 foe_n, u16 war_turns, double us) {
    const cstr tag = (foe_n > 0u && taken >= foe_n) ? "full victory" : "stall";
    std::printf("%u %u -> %u/%u  [%s after %u turns]  %.1f us\n",
        turn,
        static_cast<unsigned>(mustered),
        static_cast<unsigned>(taken),
        static_cast<unsigned>(foe_n),
        tag,
        static_cast<unsigned>(war_turns),
        us);
}

static void run_sim_row (GameState& state, u16 lucky, u16 foe) {
    const u16 foe_n0 = count_owner_cities(state, foe);
    MockMusterSiege mock;
    const auto t0 = std::chrono::steady_clock::now();
    if (!mock.collect(state, lucky)) {
        const auto t1 = std::chrono::steady_clock::now();
        const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        print_sim_row(state.m_current_turn, 0u, 0u, foe_n0, 0u, us);
        return;
    }
    const u16 mustered = mock.army_n();
    u16 taken = 0u;
    u16 foe_n = 0u;
    u16 war_turns = 0u;
    if (!mock.campaign(state, foe, &taken, &foe_n, &war_turns)) {
        const auto t1 = std::chrono::steady_clock::now();
        const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        print_sim_row(state.m_current_turn, mustered, 0u, foe_n0, 0u, us);
        return;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    print_sim_row(state.m_current_turn, mustered, taken, foe_n, war_turns, us);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    (void)argc;
    (void)argv;
    if (!build_cache_paths()) {
        std::printf("*** FAILED build_cache_paths\n");
        return 1;
    }
    GameState state;
    if (!prepare_state(&state)) {
        std::printf("*** FAILED prepare_state\n");
        return 1;
    }
    if (state.m_statics == nullptr
        || !TileAttrTables::setup(*state.m_statics)
        || !UnitMovementMng::setup_mvt_costs(*state.m_statics)
        || !CombatMng::setup(*state.m_statics)) {
        std::printf("*** FAILED movement/combat setup\n");
        state.clear();
        return 1;
    }
    u16 lucky = U16_KEY_NULL;
    u16 foe = U16_KEY_NULL;
    if (!pick_lucky_and_near(state, &lucky, &foe)) {
        std::printf("*** FAILED pick_lucky_and_near\n");
        state.clear();
        return 1;
    }
    std::printf("lucky=%u foe=%u players=%u turns=%u sim_every=%u\n",
        static_cast<unsigned>(lucky),
        static_cast<unsigned>(foe),
        static_cast<unsigned>(G_PLAYERS),
        static_cast<unsigned>(G_TURNS),
        static_cast<unsigned>(G_SIM_EVERY));
    state.m_turn_limit = G_TURNS;
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("*** FAILED game loop begin\n");
        state.clear();
        return 1;
    }
    std::printf("turn mustered -> taken/foe  [result]\n");
    run_sim_row(state, lucky, foe);
    while (state.m_current_turn < state.m_turn_limit) {
        if (!loop.step()) {
            break;
        }
        if ((state.m_current_turn % G_SIM_EVERY) == 0u) {
            run_sim_row(state, lucky, foe);
        }
    }
    loop.end();
    std::printf("*** PASSED mock_muster_siege turns=%u\n", state.m_current_turn);
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
