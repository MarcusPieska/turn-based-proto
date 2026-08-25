//================================================================================================================================
//=> - Includes -
//================================================================================================================================

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
#include "unit_type_static_key.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-turn-handler-tracer-test";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/city-turn-handler-tracer-test/game_loop.trace";
static const char* G_CITY_LOG = "/home/w/Projects/simple-map-gen/city-turn-handler-tracer-test/cities.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100;
static const u32 G_TURN_CAP = 300u;
static const u16 G_WORKERS_PER_CITY = 2u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static u16 g_city_spawned_n = 0;

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

static void unlock_all_tech (GameState& state) {
    if (state.m_player_states == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u32 tn = state.m_statics->tech().get_item_count();
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_techs_researched == nullptr) {
            ps.m_techs_researched = new BitArrayCL(tn);
        }
        for (u32 i = 0; i < tn; ++i) {
            ps.m_techs_researched->set_bit(i);
        }
    }
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

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    (void)argc;
    (void)argv;
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
    unlock_all_tech(state);
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
    state.m_turn_limit = G_TURN_CAP;
    state.m_current_turn = 0;
    g_city_spawned_n = 0;
    spawn_workers_for_new_cities(state, worker_typ);
    LOG_CITY_SETUP((G_CITY_LOG));
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("GameLoop::begin failed\n");
        LOG_CITY_CLEAR(());
        state.clear();
        setup.release_map_gen();
        return 1;
    }
    spawn_workers_for_new_cities(state, worker_typ);
    while (state.m_current_turn < G_TURN_CAP) {
        if (!loop.step()) {
            break;
        }
        spawn_workers_for_new_cities(state, worker_typ);
        std::printf("\rturn %u / %u", state.m_current_turn, G_TURN_CAP);
        std::fflush(stdout);
    }
    std::printf("\n");
    loop.end();
    LOG_CITY_CLEAR(());
    TileWorkAssessor::bind_ctx(nullptr);
    TileYields::bind_ctx(nullptr);
    const u16 cn = state.m_cities.get_city_count();
    std::printf("=======================================================\n");
    std::printf(" CITY TURN HANDLER TRACER: done turns=%u cities=%u out=%s\n",
        G_TURN_CAP, cn, G_CITY_LOG);
    std::printf("=======================================================\n");
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
