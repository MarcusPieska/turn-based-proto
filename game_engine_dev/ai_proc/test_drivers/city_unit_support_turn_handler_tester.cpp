//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "bit_array.h"
#include "city.h"
#include "city_tracer.h"
#include "game_loop.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "tile_yields.h"
#include "tile_work_assessor.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"
#include "unit_type_static_key.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-unit-support-turn-handler-test";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/city-unit-support-turn-handler-test/game_loop.trace";
static const char* G_TOTALS = "/home/w/Projects/simple-map-gen/city-unit-support-turn-handler-test/totals.txt";
static const char* G_TIMING = "/home/w/Projects/simple-map-gen/city-unit-support-turn-handler-test/timing.txt";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100;
static const u32 G_TURN_CAP_DEFAULT = 300u;
static const u16 G_WORKERS_PER_CITY = 2u;
static const u16 G_TRACE_CITY_N = 100u;
static const u16 k_act_is_land = 0u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static u16 g_city_spawned_n = 0;
static FILE* g_city_out[G_TRACE_CITY_N];
static u16 g_city_free_tot[G_TRACE_CITY_N];

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
    return true;
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static u16 find_worker_typ (const RuntimeStatics& st) {
    u16 worker_type = U16_KEY_NULL;
    const u16 type_n = st.unit_type().get_item_count();
    for (u16 i = 0; i < type_n; ++i) {
        cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "LAND_WORKER") == 0) {
            worker_type = i;
            break;
        }
    }
    if (worker_type == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    const u16 unit_n = st.unit().get_item_count();
    for (u16 i = 0; i < unit_n; ++i) {
        if (st.unit().get_item(UnitStaticDataKey::from_raw(i)).type == worker_type) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

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

static void spawn_workers_for_new_cities (GameState& state, u16 worker_typ) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = g_city_spawned_n; i < cn; ++i) {
        spawn_workers_for_city(state, i, worker_typ, G_WORKERS_PER_CITY);
    }
    g_city_spawned_n = cn;
}

static bool unit_is_land (const GameState& state, u16 unit_cat_idx) {
    if (state.m_statics == nullptr || unit_cat_idx == U16_KEY_NULL) {
        return false;
    }
    const UnitStaticDataStruct& u = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_cat_idx));
    return state.m_statics->unit_type_action_map().unit_type_can_do(u.type, k_act_is_land);
}

static u32 count_live_units (GameState& state) {
    u32 n = 0;
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 i = 0; i < scan_n; ++i) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(i)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        n = n + 1u;
    }
    return n;
}

static void print_unit_summary (GameState& state) {
    if (state.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = state.m_statics->unit().get_item_count();
    if (typ_n == 0) {
        return;
    }
    u32* counts = new u32[typ_n];
    for (u16 i = 0; i < typ_n; ++i) {
        counts[i] = 0;
    }
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 i = 0; i < scan_n; ++i) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(i)));
        if (u == nullptr || u->m_unit_typ_idx >= typ_n) {
            continue;
        }
        counts[u->m_unit_typ_idx] = counts[u->m_unit_typ_idx] + 1u;
    }
    std::printf(" unit summary (global):\n");
    for (u16 i = 0; i < typ_n; ++i) {
        if (counts[i] == 0) {
            continue;
        }
        cstr nm = state.m_statics->unit().get_name(UnitStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            nm = "?";
        }
        std::printf("  %s : %u\n", nm, counts[i]);
    }
    delete[] counts;
}

static void dump_support_row (FILE* out, GameState& state, u16 city_idx, u16 free_tot) {
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr || out == nullptr || state.m_player_states == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u16 owner = city->get_owner();
    if (owner >= state.m_player_n) {
        return;
    }
    const PlayerState& ps = state.m_player_states[owner];
    const u16 free_left = city->get_free_land_unit_support();
    std::fprintf(out, "%u, %u, %u, %u/%u", state.m_current_turn, owner, ps.m_free_land_unit_support, free_left, free_tot);
    u16 cur = state.m_map.get_unit_hd(city->get_x(), city->get_y());
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == owner && unit_is_land(state, u->m_unit_typ_idx)) {
            cstr nm = state.m_statics->unit().get_name(UnitStaticDataKey::from_raw(u->m_unit_typ_idx));
            if (nm == nullptr) {
                nm = "?";
            }
            std::fprintf(out, ", %s", nm);
        }
        cur = u->m_next_unit_on_tile;
    }
    std::fprintf(out, "\n");
}

static bool open_city_files () {
    for (u16 i = 0; i < G_TRACE_CITY_N; ++i) {
        g_city_out[i] = nullptr;
        g_city_free_tot[i] = 0;
    }
    for (u16 i = 0; i < G_TRACE_CITY_N; ++i) {
        char path[400];
        if (std::snprintf(path, sizeof(path), "%s/city_%03u.txt", G_OUT_DIR, i) <= 0) {
            return false;
        }
        g_city_out[i] = std::fopen(path, "w");
        if (g_city_out[i] == nullptr) {
            return false;
        }
        std::fprintf(g_city_out[i],
            "# city=%u one row/turn: turn, owner, free_civ, city_free leftover/total (total=pre-city-pass calc), unit...\n", i);
    }
    return true;
}

static void close_city_files () {
    for (u16 i = 0; i < G_TRACE_CITY_N; ++i) {
        if (g_city_out[i] != nullptr) {
            std::fclose(g_city_out[i]);
            g_city_out[i] = nullptr;
        }
    }
}

static void snap_city_free_tot (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    const u16 n = cn < G_TRACE_CITY_N ? cn : G_TRACE_CITY_N;
    for (u16 i = 0; i < n; ++i) {
        City* city = state.m_cities.get_city(i);
        g_city_free_tot[i] = city != nullptr ? city->calc_city_land_unit_support(i) : 0;
    }
    for (u16 i = n; i < G_TRACE_CITY_N; ++i) {
        g_city_free_tot[i] = 0;
    }
}

static void dump_turn_snapshot (FILE* totals, GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    const u16 n = cn < G_TRACE_CITY_N ? cn : G_TRACE_CITY_N;
    for (u16 i = 0; i < n; ++i) {
        dump_support_row(g_city_out[i], state, i, g_city_free_tot[i]);
    }
    if (totals != nullptr) {
        std::fprintf(totals, "%u, %u, %u\n", state.m_current_turn, count_live_units(state), cn);
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    u32 turn_cap = G_TURN_CAP_DEFAULT;
    bool settler_dbg = false;
    for (int a = 1; a < argc; ++a) {
        if (argv[a] == nullptr) {
            continue;
        }
        if (std::strcmp(argv[a], "settler_dbg") == 0) {
            settler_dbg = true;
            continue;
        }
        char* end = nullptr;
        const unsigned long v = std::strtoul(argv[a], &end, 10);
        if (end != argv[a] && *end == '\0' && v > 0ul && v <= 100000ul) {
            turn_cap = static_cast<u32>(v);
            continue;
        }
        std::printf("usage: %s [turn_cap] [settler_dbg]\n",
            argc > 0 && argv[0] != nullptr ? argv[0] : "city_unit_support_turn_handler_tester");
        return 1;
    }
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
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        return 1;
    }
    BitArrayCL tech(state.m_statics->tech().get_item_count());
    for (u32 i = 0; i < tech.get_count(); ++i) {
        tech.set_bit(i);
    }
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileYields::bind_ctx(&yctx);
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    TileWorkAssessor::bind_ctx(&wctx);
    const u16 worker_typ = find_worker_typ(*state.m_statics);
    if (worker_typ == U16_KEY_NULL) {
        std::printf("fail find worker typ\n");
        state.clear();
        setup.release_map_gen();
        return 1;
    }
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

    if (!open_city_files()) {
        std::printf("open city out files failed\n");
        close_city_files();
        state.clear();
        setup.release_map_gen();
        return 1;
    }
    FILE* totals = std::fopen(G_TOTALS, "w");
    FILE* timing = std::fopen(G_TIMING, "w");
    if (totals == nullptr || timing == nullptr) {
        std::printf("open totals/timing failed\n");
        if (totals != nullptr) {
            std::fclose(totals);
        }
        if (timing != nullptr) {
            std::fclose(timing);
        }
        close_city_files();
        state.clear();
        setup.release_map_gen();
        return 1;
    }
    std::fprintf(totals, "# turn, live_units, city_n\n");
    std::fprintf(timing, "# turn, step_ns\n");

    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("GameLoop::begin failed\n");
        std::fclose(totals);
        std::fclose(timing);
        close_city_files();
        state.clear();
        setup.release_map_gen();
        return 1;
    }
    spawn_workers_for_new_cities(state, worker_typ);
    snap_city_free_tot(state);
    dump_turn_snapshot(totals, state);

    i64 step_ns_sum = 0;
    u32 step_n = 0;
    while (state.m_current_turn < turn_cap) {
        snap_city_free_tot(state);
        const auto t0 = std::chrono::steady_clock::now();
        if (!loop.step()) {
            break;
        }
        const auto t1 = std::chrono::steady_clock::now();
        const i64 ns = static_cast<i64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        step_ns_sum = step_ns_sum + ns;
        step_n = step_n + 1u;
        std::fprintf(timing, "%u, %lld\n", state.m_current_turn, static_cast<long long>(ns));
        spawn_workers_for_new_cities(state, worker_typ);
        dump_turn_snapshot(totals, state);
        if (settler_dbg && state.m_player_states != nullptr && state.m_player_n > 0) {
            const PlayerState& ps0 = state.m_player_states[0];
            std::printf("%u %u %u %u %u\n", state.m_current_turn, ps0.m_last_turn_settler_count,
                ps0.m_target_settlements, SettlerTurnHandler::elig_sites(state, 0), ps0.m_last_turn_city_count);
        } else {
            std::printf("\rturn %u / %u", state.m_current_turn, turn_cap);
            std::fflush(stdout);
        }
    }
    if (!settler_dbg) {
        std::printf("\n");
    }
    loop.end();
    std::fclose(totals);
    std::fclose(timing);
    close_city_files();
    TileWorkAssessor::bind_ctx(nullptr);
    TileYields::bind_ctx(nullptr);
    const u16 cn = state.m_cities.get_city_count();
    const double mean_ms = step_n == 0u ? 0.0 : (static_cast<double>(step_ns_sum) / static_cast<double>(step_n)) / 1.0e6;
    std::printf("=======================================================\n");
    std::printf(" CITY UNIT SUPPORT TURN HANDLER: turns=%u cities=%u traced=%u\n",
        state.m_current_turn, cn, G_TRACE_CITY_N);
    std::printf(" out=%s\n", G_OUT_DIR);
    std::printf(" step mean=%.3f ms over %u steps\n", mean_ms, step_n);
    print_unit_summary(state);
    std::printf("=======================================================\n");
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
