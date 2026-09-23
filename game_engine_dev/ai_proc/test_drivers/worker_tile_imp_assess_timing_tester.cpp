//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <sys/stat.h>

#include "assert_log.h"
#include "bit_array.h"
#include "city.h"
#include "city_border.h"
#include "city_tracer.h"
#include "city_turn_handler.h"
#include "defensive_unit_turn_handler.h"
#include "game_loop.h"
#include "game_setup.h"
#include "game_state.h"
#include "research_turn_handler.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "whiteboard_mng.h"
#include "worker_build_progress.h"
#include "worker_helper.h"
#include "worker_turn_handler_mk2.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/worker-imp-assess-timing";
static char g_trace[384];
static char g_city_log[384];
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100;
static const u32 G_TURN_CAP = 300u;
static const u16 G_CLAIM_CULT = 25u;
static const u16 G_WORKERS_PER_CITY = 2u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

//================================================================================================================================
//=> - Assess timing -
//================================================================================================================================

struct AssessTiming {
    u64 m_n; // Samples
    u64 m_sum_ns; // Total ns
    u64 m_min_ns; // Min sample ns
    u64 m_max_ns; // Max sample ns
};

static AssessTiming g_assess = {0, 0, 0xffffffffffffffffull, 0};

static void assess_note (u64 ns) {
    g_assess.m_n = g_assess.m_n + 1u;
    g_assess.m_sum_ns = g_assess.m_sum_ns + ns;
    if (ns < g_assess.m_min_ns) {
        g_assess.m_min_ns = ns;
    }
    if (ns > g_assess.m_max_ns) {
        g_assess.m_max_ns = ns;
    }
}

static void assess_report () {
    if (g_assess.m_n == 0u) {
        std::printf("assess timing: no samples\n");
        return;
    }
    const double min_us = static_cast<double>(g_assess.m_min_ns) / 1000.0;
    const double max_us = static_cast<double>(g_assess.m_max_ns) / 1000.0;
    const double avg_us = static_cast<double>(g_assess.m_sum_ns) / static_cast<double>(g_assess.m_n) / 1000.0;
    std::printf("WorkerTurnHandlerMk2::assess us: min=%.2f max=%.2f avg=%.2f (n=%llu)\n",
        min_us, max_us, avg_us, (unsigned long long)g_assess.m_n);
}

//================================================================================================================================
//=> - Setup helpers -
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
    return true;
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool set_out_paths () {
    if (std::snprintf(g_trace, sizeof(g_trace), "%s/game_loop.trace", G_OUT_DIR) <= 0) {
        return false;
    }
    if (std::snprintf(g_city_log, sizeof(g_city_log), "%s/cities.trace", G_OUT_DIR) <= 0) {
        return false;
    }
    return true;
}

static u16 find_worker_typ (const RuntimeStatics& st) {
    u16 worker_type = U16_KEY_NULL;
    const u16 tn = st.unit_type().get_item_count();
    for (u16 i = 0; i < tn; ++i) {
        cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "LAND_WORKER") == 0) {
            worker_type = i;
            break;
        }
    }
    if (worker_type == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    const u16 un = st.unit().get_item_count();
    for (u16 i = 0; i < un; ++i) {
        if (st.unit().get_item(UnitStaticDataKey::from_raw(i)).type == worker_type) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 g_city_spawned_n = 0;

static u16 spawn_workers_for_city (GameState& state, u16 city_idx, u16 worker_typ, u16 n) {
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr || city->get_owner() == U16_KEY_NULL) {
        return 0;
    }
    u16 spawned = 0;
    for (u16 k = 0; k < n; ++k) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, city->get_x(), city->get_y(), city->get_owner(), worker_typ, &key)) {
            break;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            break;
        }
        WorkerHelper::set_data(u, city_idx);
        spawned = static_cast<u16>(spawned + 1u);
    }
    return spawned;
}

static u16 spawn_workers_for_new_cities (GameState& state, u16 worker_typ) {
    const u16 cn = state.m_cities.get_city_count();
    u16 spawned = 0;
    for (u16 i = g_city_spawned_n; i < cn; ++i) {
        spawned = static_cast<u16>(spawned + spawn_workers_for_city(state, i, worker_typ, G_WORKERS_PER_CITY));
    }
    g_city_spawned_n = cn;
    return spawned;
}

static void unlock_all_tech (GameState& state) {
    const u16 tech_n = state.m_statics->tech().get_item_count();
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_techs_researched == nullptr) {
            ps.m_techs_researched = new BitArrayCL(tech_n);
        }
        if (ps.m_techs_researched == nullptr) {
            continue;
        }
        for (u32 i = 0; i < ps.m_techs_researched->get_count(); ++i) {
            ps.m_techs_researched->set_bit(i);
        }
    }
}

static void claim_city_borders (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        CityBorder::claim_expand(c->get_x(), c->get_y(), 0, G_CLAIM_CULT, static_cast<u8>(c->get_owner()));
    }
}

static void refill_mp (GameState& state, u16 unit_idx) {
    UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
    if (ut == state.m_land_worker_type_idx) {
        WorkerBuildProgress::refill_mp(state, unit_idx);
        return;
    }
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * state.m_statics->config().get_mov_pt_per_turn());
}

static void after_city_turns (GameState& state) {
    for (u16 p = 0; p < state.m_player_n; ++p) {
        ResearchTurnHandler::handle(state, p);
        City::refresh_city_worker_flags(state, p);
        PlayerState& ps = state.m_player_states[p];
        ps.m_last_turn_population_count = ps.m_this_turn_population_count;
        ps.m_last_turn_city_count = ps.m_this_turn_city_count;
        ps.m_this_turn_population_count = 0;
        ps.m_this_turn_city_count = 0;
        ps.m_last_turn_new_land_unit_build_support = ps.m_this_turn_new_land_unit_build_support;
        ps.m_last_turn_new_naval_unit_build_support = ps.m_this_turn_new_naval_unit_build_support;
        ps.m_this_turn_new_land_unit_build_support = 0;
        ps.m_this_turn_new_naval_unit_build_support = 0;
        ps.m_last_turn_settler_build_n = ps.m_this_turn_settler_build_n;
        ps.m_this_turn_settler_build_n = 0;
        ps.m_last_turn_settler_count = 0;
        ps.m_last_turn_worker_build_n = ps.m_this_turn_worker_build_n;
        ps.m_this_turn_worker_build_n = 0;
        ps.m_last_turn_worker_count = 0;
        ps.m_defensive_unit_count = 0;
    }
}

static void run_city_turns (GameState& state) {
    SettlerTurnHandler::refresh_targets(state);
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        ps.m_free_land_unit_support = 0;
        ps.m_free_naval_unit_support = 0;
        ps.m_land_unit_upkeep_needed = 0;
        ps.m_naval_unit_upkeep_needed = 0;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        if (state.m_cities.get_city(i) == nullptr) {
            continue;
        }
        CityTurnHandler::handle(state, i);
    }
    LOG_CITY_FLUSH(());
    after_city_turns(state);
}

static void run_assess_all (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* city = state.m_cities.get_city(i);
        if (city == nullptr) {
            continue;
        }
        const auto t0 = std::chrono::steady_clock::now();
        const u32 raw = WorkerTurnHandlerMk2::assess(state, i);
        const auto t1 = std::chrono::steady_clock::now();
        assess_note(static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        const u8 got = city->get_tile_imp_count();
        if (raw != static_cast<u32>(got)) {
            GAME_EXPECT(got == 255u, "assess clamp: raw!=city implies city==255");
        }
    }
}

static void run_unit_turns (GameState& state) {
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const u16 unit_idx = static_cast<u16>(idx);
        UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (ut == state.m_land_settler_type_idx) {
            refill_mp(state, unit_idx);
            SettlerTurnHandler::handle(state, unit_idx);
        } else if (ut == state.m_land_worker_type_idx) {
            refill_mp(state, unit_idx);
            WorkerTurnHandlerMk2::handle(state, unit_idx);
        } else if (ut == state.m_land_defense_type_idx) {
            DefensiveUnitTurnHandler::handle(state, unit_idx);
        }
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    u32 turn_cap = G_TURN_CAP;
    if (argc > 1) {
        turn_cap = static_cast<u32>(std::atoi(argv[1]));
        if (turn_cap == 0u) {
            turn_cap = G_TURN_CAP;
        }
    }
#ifndef CITY_TRACER_ENABLE
    std::printf("CITY_TRACER_ENABLE not set; rebuild with -DCITY_TRACER_ENABLE\n");
    return 1;
#endif
    if (!set_out_paths() || !build_paths() || !ensure_out_dir()) {
        std::printf("path/out setup failed\n");
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
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        return 1;
    }
    if (state.m_statics == nullptr) {
        std::printf("missing statics\n");
        state.clear();
        return 1;
    }
    const RuntimeStatics& st = *state.m_statics;
    unlock_all_tech(state);
    BitArrayCL tech(st.tech().get_item_count());
    for (u32 i = 0; i < tech.get_count(); ++i) {
        tech.set_bit(i);
    }
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileYields::bind_ctx(&yctx);
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    wctx.m_resource = nullptr;
    TileWorkAssessor::bind_ctx(&wctx);

    const u16 worker_typ = find_worker_typ(st);
    if (worker_typ == U16_KEY_NULL) {
        std::printf("fail find worker typ\n");
        state.clear();
        return 1;
    }

    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    claim_city_borders(state);
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        ps.m_target_settlements = SETTLER_MISSION_SLOTS;
        ps.m_worker_tile_opt_scan = 1;
        ps.m_worker_tile_opt_reassign = 0;
    }

    state.m_turn_limit = turn_cap;
    state.m_current_turn = 0;
    g_city_spawned_n = 0;
    spawn_workers_for_new_cities(state, worker_typ);

    GameLoop loop;
    LOG_CITY_SETUP((g_city_log));
    if (!loop.begin(&state, g_trace)) {
        std::printf("GameLoop::begin failed\n");
        LOG_CITY_CLEAR(());
        state.clear();
        return 1;
    }
    spawn_workers_for_new_cities(state, worker_typ);

    std::printf("*** assess timing loop players=%u turn_cap=%u\n", state.m_player_n, turn_cap);
    while (state.m_current_turn < turn_cap) {
        state.m_current_turn = state.m_current_turn + 1u;
        run_city_turns(state);
        run_assess_all(state);
        run_unit_turns(state);
        spawn_workers_for_new_cities(state, worker_typ);
        if ((state.m_current_turn % 50u) == 0u) {
            std::printf("\rturn %u / %u cities=%u", state.m_current_turn, turn_cap,
                state.m_cities.get_city_count());
            std::fflush(stdout);
        }
    }
    std::printf("\n");
    assess_report();

    loop.end();
    LOG_CITY_CLEAR(());
    SettlerTurnHandler::clear();
    TileWorkAssessor::bind_ctx(nullptr);
    TileYields::bind_ctx(nullptr);
    WhiteboardMng::terminate();
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
