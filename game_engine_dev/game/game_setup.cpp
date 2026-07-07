//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_setup.h"

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "map_terrain_data.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - Static runtime data -
//================================================================================================================================

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

static const char* G_RT_LIB = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../";

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

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_latt_div = 10;
static const u8 k_own_bpv = 4u;

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

static bool fill_terrain_from_map (const GameArraySimple& map, MapTerrainData* out) {
    if (out == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = map.tile_n();
    if (w == 0 || h == 0 || n == 0) {
        return false;
    }
    u8* terr = new u8[n];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            terr[i] = map.get_terrain(x, y);
        }
    }
    const bool ok = out->assign_copy(w, h, terr);
    delete[] terr;
    return ok;
}

//================================================================================================================================
//=> - GameSetup -
//================================================================================================================================

GameSetup::GameSetup () {
}

GameSetup::~GameSetup () {
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
    if (!fill_terrain_from_map(map, &terr)) {
        return false;
    }
    u16 latt_rows = 0;
    u16 latt_cols = 0;
    latt_for_map(map.width(), map.height(), &latt_rows, &latt_cols);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.pick_n = player_n;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;
    StartingPointGenerator gen(par);
    if (!gen.generate()) {
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

bool GameSetup::init_players (GameState* state, u16 player_n) {
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
        if (seats[i].m_explored_overlay == nullptr || seats[i].m_explored_overlay->width() == 0) {
            for (u16 j = 0; j <= i; ++j) {
                delete seats[j].m_explored_overlay;
                seats[j].m_explored_overlay = nullptr;
            }
            delete[] seats;
            return false;
        }
    }
    MapBitArrayOverlay* own = new MapBitArrayOverlay(w, h, k_own_bpv);
    if (own == nullptr || own->width() == 0) {
        for (u16 i = 0; i < player_n; ++i) {
            delete seats[i].m_explored_overlay;
        }
        delete[] seats;
        delete own;
        return false;
    }
    state->m_player_states = seats;
    state->m_player_n = player_n;
    state->m_players_remaining = player_n;
    state->m_tile_ownership_array = own;
    return true;
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
    SpgPickCoords starts = {};
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, paths.m_terr, paths.m_clim, paths.m_riv)) {
        return false;
    }
    if (paths.m_res != nullptr && !Factory_GameArraySimple::load_res_dist_data(&state->m_map, paths.m_res)) {
        state->clear();
        return false;
    }
    if (!run_start_placement(state->m_map, player_n, &starts)) {
        state->clear();
        return false;
    }
    if (!init_players(state, player_n)) {
        state->clear();
        return false;
    }
    const ConfigListUnit& start_units = g_rt_statics->config().get_start_units();
    if (start_units.n == 0) {
        state->clear();
        return false;
    }
    u16 typ_idxs[MAX_CONFIG_LIST_ITEMS];
    for (u16 k = 0; k < start_units.n; ++k) {
        typ_idxs[k] = start_units.keys[k].value();
    }
    for (u16 i = 0; i < player_n; ++i) {
        if (!state->spawn(starts.pts[i].x, starts.pts[i].y, i, typ_idxs, start_units.n)) {
            state->clear();
            return false;
        }
    }
    state->m_current_turn = 0;
    state->m_age_of_exploration = true;
    return true;
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
