//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_setup.h"

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "game_loop_cache.h"
#include "game_helpers/civ_spawner.h"
#include "map_terrain_data.h"
#include "map_gen_loader.h"
#include "runtime_static_loader.h"
#include "city.h"
#include "unit_movement_mng.h"
#include "player_ledger.h"
#include "tile_yields.h"
#include "tile_working.h"
#include "city_tile_manager.h"
#include "city_border.h"

//================================================================================================================================
//=> - Static runtime data -
//================================================================================================================================

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;
static MapGenLoader g_map_loader;

static const char* G_RT_LIB = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../";
static const char* G_MAP_LIB = "../adv_map_gen/map_gen.so";

static bool ensure_runtime_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load(G_RT_LIB, G_RT_DATA)) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool ensure_map_gen_loader () {
    if (g_map_loader.is_loaded()) {
        return true;
    }
    return g_map_loader.load(G_MAP_LIB);
}

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_latt_div = 10;

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / k_latt_div;
    u16 c = w / k_latt_div;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static bool fill_tile_layers_from_map (const GameArraySimple& map, MapTerrainData* terr, u8** clim, u8** ov) {
    if (terr == nullptr || clim == nullptr || ov == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    if (w == 0 || h == 0 || n == 0) {
        return false;
    }
    u8* terr_buf = new u8[n];
    u8* clim_buf = new u8[n];
    u8* ov_buf = new u8[n];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            terr_buf[i] = map.get_terrain(x, y);
            clim_buf[i] = map.get_climate(x, y);
            ov_buf[i] = map.get_overlay(x, y);
        }
    }
    if (!terr->assign_copy(w, h, terr_buf)) {
        delete[] terr_buf;
        delete[] clim_buf;
        delete[] ov_buf;
        return false;
    }
    delete[] terr_buf;
    *clim = clim_buf;
    *ov = ov_buf;
    return true;
}

//================================================================================================================================
//=> - GameSetup -
//================================================================================================================================

GameSetup::GameSetup () {
}

GameSetup::~GameSetup () {
}

void GameSetup::release_map_gen () {
    g_map_loader.unload();
}

bool GameSetup::run_start_placement (const GameArraySimple& map, u16 player_n, SpgPickCoords* out_starts) {
    if (out_starts == nullptr) {
        return false;
    }
    out_starts->n = 0;
    if (player_n == 0 || player_n > SPG_MAX_PICK_PTS) {
        return false;
    }
    MapTerrainData terr;
    u8* clim = nullptr;
    u8* ov = nullptr;
    if (!fill_tile_layers_from_map(map, &terr, &clim, &ov)) {
        return false;
    }
    u16 latt_rows = 0;
    u16 latt_cols = 0;
    latt_for_map(map.width(), map.height(), &latt_rows, &latt_cols);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.climate = clim;
    par.overlay = ov;
    par.pick_n = player_n;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;
    StartingPointGenerator gen(par);
    const bool gen_ok = gen.generate();
    delete[] clim;
    delete[] ov;
    if (!gen_ok) {
        return false;
    }
    *out_starts = gen.picks_coords();
    u32 expect = static_cast<u32>(player_n);
    if (gen.candidate_count() < expect) {
        expect = gen.candidate_count();
    }
    if (out_starts->n != expect) {
        out_starts->n = 0;
        return false;
    }
    if (!gen.picks_are_start_land()) {
        out_starts->n = 0;
        return false;
    }
    return true;
}

bool GameSetup::init_players (GameState* state, u16 player_n, u16 small_wonder_n) {
    if (state == nullptr || player_n == 0) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    PlayerState* seats = new PlayerState[player_n];
    if (seats == nullptr) {
        return false;
    }
    for (u16 i = 0; i < player_n; ++i) {
        seats[i].m_ai_controlled = 0;
        seats[i].m_is_active = 1;
        seats[i].m_civ_index = i;
        seats[i].m_explored_overlay = new MapBitOverlay(w, h);
        seats[i].m_techs_researched = nullptr;
        seats[i].m_small_wonder_city = nullptr;
        if (small_wonder_n > 0) {
            seats[i].m_small_wonder_city = new u16[small_wonder_n];
            if (seats[i].m_small_wonder_city == nullptr) {
                for (u16 j = 0; j <= i; ++j) {
                    delete[] seats[j].m_small_wonder_city;
                    seats[j].m_small_wonder_city = nullptr;
                    delete seats[j].m_explored_overlay;
                    seats[j].m_explored_overlay = nullptr;
                }
                delete[] seats;
                return false;
            }
            for (u16 j = 0; j < small_wonder_n; ++j) {
                seats[i].m_small_wonder_city[j] = U16_KEY_NULL;
            }
        }
        if (seats[i].m_explored_overlay == nullptr || seats[i].m_explored_overlay->width() == 0) {
            for (u16 j = 0; j <= i; ++j) {
                delete[] seats[j].m_small_wonder_city;
                seats[j].m_small_wonder_city = nullptr;
                delete seats[j].m_explored_overlay;
                seats[j].m_explored_overlay = nullptr;
            }
            delete[] seats;
            return false;
        }
    }
    state->m_player_states = seats;
    state->m_player_n = player_n;
    state->m_players_remaining = player_n;
    state->m_small_wonder_count = small_wonder_n;
    return true;
}

bool GameSetup::pick_starts (const GameArraySimple& map, u16 player_n, SpgPickCoords* out_starts) {
    return run_start_placement(map, player_n, out_starts);
}

bool GameSetup::finish_with_starts (GameState* state, const SpgPickCoords& starts, u16 player_n) {
    if (state == nullptr || g_rt_statics == nullptr || player_n == 0) {
        return false;
    }
    if (starts.n != static_cast<u32>(player_n)) {
        return false;
    }
    if (!state->m_cities.bind_statics(*g_rt_statics)) {
        return false;
    }
    const u16 wonder_n = g_rt_statics->wonder().get_item_count();
    const u16 sw_n = g_rt_statics->small_wonder().get_item_count();
    if (wonder_n > 0) {
        state->m_wonder_city = new u16[wonder_n];
        if (state->m_wonder_city == nullptr) {
            state->clear();
            return false;
        }
        state->m_wonder_count = wonder_n;
        for (u16 i = 0; i < wonder_n; ++i) {
            state->m_wonder_city[i] = U16_KEY_NULL;
        }
    }
    City::bind_units(&state->m_units);
    City::bind_wonder_cities(state->m_wonder_city);
    UnitMovementMng::bind_state(state);
    PlayerLedger::bind_state(state);
    TileYields::bind_map(&state->m_map);
    TileWorking::bind_map(&state->m_map);
    CityBorder::bind_map(&state->m_map);
    CityTileManager::bind_cities(&state->m_cities);
    if (!init_players(state, player_n, sw_n)) {
        state->clear();
        return false;
    }
    City::bind_player_states(state->m_player_states, state->m_player_n);
    for (u16 i = 0; i < player_n; ++i) {
        if (!CivSpawner::spawn(state, starts.pts[i].x, starts.pts[i].y, i)) {
            state->clear();
            return false;
        }
    }
    state->m_current_turn = 0;
    state->m_age_of_exploration = true;
    return true;
}

bool GameSetup::complete_new_game (GameState* state, u16 player_n) {
    if (state == nullptr || g_rt_statics == nullptr || player_n == 0) {
        return false;
    }
    SpgPickCoords starts = {};
    if (!run_start_placement(state->m_map, player_n, &starts)) {
        state->clear();
        return false;
    }
    return finish_with_starts(state, starts, player_n);
}

bool GameSetup::setup_from_cache (GameState* state, cstr map_path, cstr starts_path, u16 player_n) {
    if (state == nullptr || map_path == nullptr || starts_path == nullptr || player_n == 0) {
        return false;
    }
    if (!ensure_runtime_statics()) {
        return false;
    }
    state->clear();
    state->m_statics = g_rt_statics;
    state->m_civ_relations.reset(g_rt_statics->civ().get_item_count());
    if (!GameLoopCache::load_map(map_path, &state->m_map)) {
        return false;
    }
    SpgPickCoords starts = {};
    if (!GameLoopCache::load_starts(starts_path, &starts, player_n)) {
        state->clear();
        return false;
    }
    return finish_with_starts(state, starts, player_n);
}

bool GameSetup::setup_new_game (GameState* state, const MapGenReq& req, u16 player_n) {
    if (state == nullptr || player_n == 0 || req.m_w == 0 || req.m_h == 0) {
        return false;
    }
    if (req.m_type != MAP_CONTINENTAL) {
        return false;
    }
    if (!ensure_runtime_statics()) {
        return false;
    }
    if (!ensure_map_gen_loader()) {
        return false;
    }
    state->clear();
    state->m_statics = g_rt_statics;
    state->m_civ_relations.reset(g_rt_statics->civ().get_item_count());
    MapGenReq gen_req = req;
    gen_req.m_statics = g_rt_statics;
    MakeMapRslt rslt = g_map_loader.generate(gen_req);
    if (!rslt.m_ok) {
        state->clear();
        return false;
    }
    if (!Factory_GameArraySimple::load_from_rslt(&state->m_map, rslt)) {
        g_map_loader.free_rslt(&rslt);
        state->clear();
        return false;
    }
    g_map_loader.free_rslt(&rslt);
    return complete_new_game(state, player_n);
}

bool GameSetup::setup_new_game (GameState* state, const MapPpmPaths& paths, u16 player_n) {
    if (state == nullptr || paths.m_terr == nullptr || paths.m_clim == nullptr || paths.m_riv == nullptr) {
        return false;
    }
    if (!ensure_runtime_statics()) {
        return false;
    }
    state->clear();
    state->m_statics = g_rt_statics;
    state->m_civ_relations.reset(g_rt_statics->civ().get_item_count());
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, paths.m_terr, paths.m_clim, paths.m_riv, paths.m_ov)) {
        return false;
    }
    if (paths.m_res != nullptr && !Factory_GameArraySimple::load_res_dist_data(&state->m_map, paths.m_res)) {
        state->clear();
        return false;
    }
    return complete_new_game(state, player_n);
}

bool GameSetup::save_game (cstr path, const GameState* state) {
    (void)path;
    (void)state;
    return false;
}

bool GameSetup::load_game (cstr path, GameState* state) {
    (void)path;
    (void)state;
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
