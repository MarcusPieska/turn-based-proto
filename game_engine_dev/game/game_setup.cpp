//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include "game_setup.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "game_loop_cache.h"
#include "game_helpers/civ_spawner.h"
#include "map_ov_bridge.h"
#include "map_terrain_data.h"
#include "map_gen_loader.h"
#include "lucky_seat_loader.h"
#include "runtime_static_loader.h"
#include "city.h"
#include "unit_movement_mng.h"
#include "player_ledger.h"
#include "resource_turn_handler.h"
#include "tile_yields.h"
#include "tile_working.h"
#include "tile_imp_helper.h"
#include "worker_guidance.h"
#include "city_tile_manager.h"
#include "city_border.h"
#include "building_trait_orderings.h"
#include "city_job_trait_orderings.h"
#include "tech_trait_orderings.h"
#include "tech_age_mng.h"
#include "assert_log.h"
#include "sector_network.h"
#include "sector_network_router.h"
#include "unit_type_static_key.h"
#include "profile_time_opt.h"
#include "gen_ai_helpers.h"
#include "whiteboard_mng.h"
#include "city_connector.h"

//================================================================================================================================
//=> - Static runtime data -
//================================================================================================================================

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;
static MapGenLoader g_map_loader;
static LuckySeatLoader g_lucky_loader;

static const char* G_RT_LIB_A = "../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_A = "../";
static const char* G_MAP_LIB_A = "../adv_map_gen/map_gen.so";
static const char* G_LUCKY_LIB_A = "lucky_seats/lucky_seats.so";
static const char* G_RT_LIB_B = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA_B = "../../";
static const char* G_MAP_LIB_B = "../../adv_map_gen/map_gen.so";
static const char* G_LUCKY_LIB_B = "../../game/lucky_seats/lucky_seats.so";

static bool ensure_runtime_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (g_rt_loader.load(G_RT_LIB_A, G_RT_DATA_A) || g_rt_loader.load(G_RT_LIB_B, G_RT_DATA_B)) {
        g_rt_statics = &g_rt_loader.statics();
        return true;
    }
    return false;
}

static bool ensure_map_gen_loader () {
    if (g_map_loader.is_loaded()) {
        return true;
    }
    if (g_map_loader.load(G_MAP_LIB_A)) {
        return true;
    }
    char err_a[512];
    {
        const char* e = dlerror();
        if (e == nullptr) {
            err_a[0] = '?';
            err_a[1] = 0;
        } else {
            std::snprintf(err_a, sizeof(err_a), "%s", e);
        }
    }
    if (g_map_loader.load(G_MAP_LIB_B)) {
        return true;
    }
    const char* err_b = dlerror();
    std::printf("ensure_map_gen_loader failed:\n  %s: %s\n  %s: %s\n",
        G_MAP_LIB_A, err_a, G_MAP_LIB_B, err_b != nullptr ? err_b : "?");
    return false;
}

static bool ensure_lucky_loader () {
    if (g_lucky_loader.is_loaded()) {
        return true;
    }
    if (g_lucky_loader.load(G_LUCKY_LIB_A) || g_lucky_loader.load(G_LUCKY_LIB_B)) {
        return true;
    }
    std::printf("ensure_lucky_loader failed:\n  %s\n  %s\n", G_LUCKY_LIB_A, G_LUCKY_LIB_B);
    return false;
}

static bool apply_lucky_seats (
    GameArraySimple& map,
    const RuntimeStatics& st,
    const SpgPickCoords& starts,
    u16* out_seats,
    u16 out_cap,
    u16* out_n)
{
    if (!ensure_lucky_loader()) {
        return false;
    }
    if (starts.n == 0u || starts.n > SPG_MAX_PICK_PTS) {
        return false;
    }
    if (out_seats == nullptr || out_n == nullptr || out_cap == 0u) {
        return false;
    }
    LuckySeatReq req = {};
    req.m_map = &map;
    req.m_statics = &st;
    req.m_starts = starts.pts;
    req.m_start_n = static_cast<u16>(starts.n);
    req.m_lucky_seats = out_seats;
    req.m_lucky_cap = out_cap;
    req.m_lucky_n = 0u;
    req.m_do_select = 1u;
    req.m_do_boost = 1u;
    const LuckySeatRslt r = g_lucky_loader.run(&req);
    if (!r.m_ok) {
        return false;
    }
    *out_n = req.m_lucky_n;
    return true;
}

static void mark_lucky_seats (GameState* state, const u16* seats, u16 n) {
    if (state == nullptr || state->m_player_states == nullptr || seats == nullptr) {
        return;
    }
    for (u16 i = 0; i < state->m_player_n; ++i) {
        state->m_player_states[i].m_lucky = 0u;
    }
    for (u16 i = 0; i < n; ++i) {
        const u16 s = seats[i];
        if (s >= state->m_player_n) {
            continue;
        }
        state->m_player_states[s].m_lucky = 1u;
        state->m_player_states[s].m_ai_controlled = 1u;
        state->m_player_states[s].m_ai_units = AiUnits::AI_UNITS_AGGRESSIVE;
    }
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
            ov_buf[i] = catalog_ov_to_map_gen(map.get_overlay(x, y));
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

static bool begin_sector_pathing (GameState* state) {
    if (state == nullptr) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    const u32 n = state->m_map.tile_n();
    if (w == 0 || h == 0 || n == 0) {
        return false;
    }
    u8* terr = new u8[n];
    if (terr == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            terr[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = state->m_map.get_terrain(x, y);
        }
    }
    const bool ok_net = state->m_sector_net.begin(w, h, terr);
    const bool ok_rt = ok_net && state->m_sector_rt.begin(state->m_sector_net);
    delete[] terr;
    return ok_rt;
}

static bool ensure_wb (u16 w, u16 h) {
    if (w == 0 || h == 0) {
        return false;
    }
    if (WhiteboardMng::width() == 0 && WhiteboardMng::height() == 0) {
        WhiteboardMng::init(w, h);
        return WhiteboardMng::width() == w && WhiteboardMng::height() == h;
    }
    if (WhiteboardMng::width() == w && WhiteboardMng::height() == h) {
        return true;
    }
    if (WhiteboardMng::chkout() != 0u) {
        return false;
    }
    WhiteboardMng::terminate();
    WhiteboardMng::init(w, h);
    return WhiteboardMng::width() == w && WhiteboardMng::height() == h;
}

static bool run_ai_helpers (GameState* state, const SpgPickCoords& starts) {
    if (state == nullptr) {
        return false;
    }
    if (!state->m_ai_help.begin(state->m_map)) {
        return false;
    }
    return state->m_ai_help.build(starts.pts, starts.n, nullptr);
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
    const u16 res_n = (g_rt_statics != nullptr) ? g_rt_statics->resource().get_item_count() : 0u;
    PlayerState* seats = new PlayerState[player_n];
    if (seats == nullptr) {
        return false;
    }
    for (u16 i = 0; i < player_n; ++i) {
        seats[i].m_ai_controlled = 0;
        seats[i].m_is_active = 1;
        {
            const u16 civ_n = (g_rt_statics != nullptr) ? g_rt_statics->civ().get_item_count() : 0u;
            seats[i].m_civ_index = (civ_n > 0u) ? static_cast<u16>(i % civ_n) : i;
        }
        seats[i].m_explored_overlay = new MapBitOverlay(w, h);
        seats[i].m_techs_researched = nullptr;
        seats[i].m_tech_age = nullptr;
        seats[i].m_small_wonder_city = nullptr;
        GAME_EXPECT(TechAgeMng::ready(), "GameSetup init_players TechAgeMng");
        seats[i].m_tech_age = new TechAgeMng();
        if (seats[i].m_tech_age == nullptr) {
            for (u16 j = 0; j <= i; ++j) {
                delete[] seats[j].m_small_wonder_city;
                seats[j].m_small_wonder_city = nullptr;
                delete seats[j].m_explored_overlay;
                seats[j].m_explored_overlay = nullptr;
                delete seats[j].m_tech_age;
                seats[j].m_tech_age = nullptr;
                seats[j].m_res_ledger.clear();
            }
            delete[] seats;
            return false;
        }
        if (!seats[i].m_res_ledger.setup(res_n)) {
            for (u16 j = 0; j <= i; ++j) {
                delete[] seats[j].m_small_wonder_city;
                seats[j].m_small_wonder_city = nullptr;
                delete seats[j].m_explored_overlay;
                seats[j].m_explored_overlay = nullptr;
                delete seats[j].m_tech_age;
                seats[j].m_tech_age = nullptr;
                seats[j].m_res_ledger.clear();
            }
            delete[] seats;
            return false;
        }
        if (small_wonder_n > 0) {
            seats[i].m_small_wonder_city = new u16[small_wonder_n];
            if (seats[i].m_small_wonder_city == nullptr) {
                for (u16 j = 0; j <= i; ++j) {
                    delete[] seats[j].m_small_wonder_city;
                    seats[j].m_small_wonder_city = nullptr;
                    delete seats[j].m_explored_overlay;
                    seats[j].m_explored_overlay = nullptr;
                    delete seats[j].m_tech_age;
                    seats[j].m_tech_age = nullptr;
                    seats[j].m_res_ledger.clear();
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
                delete seats[j].m_tech_age;
                seats[j].m_tech_age = nullptr;
                seats[j].m_res_ledger.clear();
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

void GameSetup::cache_unit_type_idxs (GameState* state) {
    if (state == nullptr || g_rt_statics == nullptr) {
        return;
    }
    state->m_land_settler_type_idx = U16_KEY_NULL;
    state->m_land_worker_type_idx = U16_KEY_NULL;
    state->m_land_scout_type_idx = U16_KEY_NULL;
    state->m_land_defense_type_idx = U16_KEY_NULL;
    state->m_land_attack_type_idx = U16_KEY_NULL;
    state->m_land_mobile_type_idx = U16_KEY_NULL;
    state->m_land_artillery_type_idx = U16_KEY_NULL;
    state->m_land_paradrop_type_idx = U16_KEY_NULL;
    const u16 n = g_rt_statics->unit_type().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = g_rt_statics->unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            continue;
        }
        if (std::strcmp(nm, "LAND_SETTLER") == 0) {
            state->m_land_settler_type_idx = i;
        } else if (std::strcmp(nm, "LAND_WORKER") == 0) {
            state->m_land_worker_type_idx = i;
        } else if (std::strcmp(nm, "LAND_SCOUT") == 0) {
            state->m_land_scout_type_idx = i;
        } else if (std::strcmp(nm, "LAND_DEFENSE") == 0) {
            state->m_land_defense_type_idx = i;
        } else if (std::strcmp(nm, "LAND_ATTACK") == 0) {
            state->m_land_attack_type_idx = i;
        } else if (std::strcmp(nm, "LAND_MOBILE") == 0) {
            state->m_land_mobile_type_idx = i;
        } else if (std::strcmp(nm, "LAND_ARTILLERY") == 0) {
            state->m_land_artillery_type_idx = i;
        } else if (std::strcmp(nm, "LAND_PARADROP") == 0) {
            state->m_land_paradrop_type_idx = i;
        }
    }
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
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    if (!ensure_wb(w, h)) {
        return false;
    }
    if (!state->m_cities.bind_statics(*g_rt_statics)) {
        return false;
    }
    cache_unit_type_idxs(state);
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
    if (!TileYields::setup(*g_rt_statics)) {
        state->clear();
        return false;
    }
    if (!TechAgeMng::setup(*g_rt_statics)) {
        state->clear();
        return false;
    }
    if (!BuildingTraitOrderings::begin(g_rt_statics->building(), g_rt_statics->trait_affinity_map())) {
        state->clear();
        return false;
    }
    if (!TechTraitOrderings::begin(g_rt_statics->tech(), g_rt_statics->building(), g_rt_statics->trait_affinity_map())) {
        state->clear();
        return false;
    }
    if (!CityJobTraitOrderings::begin(g_rt_statics->city_job(), g_rt_statics->trait_affinity_map())) {
        state->clear();
        return false;
    }
    TileYields::bind_map(&state->m_map);
    TileWorking::bind_map(&state->m_map);
    CityBorder::bind_map(&state->m_map);
    CityTileManager::bind_cities(&state->m_cities);
    WorkerGuidance::bind_statics(g_rt_statics);
    WorkerGuidance::bind_map(&state->m_map);
    TileImpHelper::bind_statics(g_rt_statics);
    u16 lucky_seats[SPG_MAX_PICK_PTS];
    u16 lucky_n = 0u;
    if (!apply_lucky_seats(state->m_map, *g_rt_statics, starts, lucky_seats, SPG_MAX_PICK_PTS, &lucky_n)) {
        std::printf("finish_with_starts: apply_lucky_seats failed\n");
        state->clear();
        return false;
    }
    if (!init_players(state, player_n, sw_n)) {
        state->clear();
        return false;
    }
    mark_lucky_seats(state, lucky_seats, lucky_n);
    City::bind_player_states(state->m_player_states, state->m_player_n);
    CityConnector::set_plan_spines(false);
    for (u16 i = 0; i < player_n; ++i) {
        if (!CivSpawner::spawn(state, starts.pts[i].x, starts.pts[i].y, i)) {
            state->clear();
            return false;
        }
    }
    if (!begin_sector_pathing(state)) {
        state->clear();
        return false;
    }
    if (!run_ai_helpers(state, starts)) {
        state->clear();
        return false;
    }
    CityConnector::sync_road_arms(*state);
    if (!ResourceTurnHandler::setup(*state)) {
        state->clear();
        return false;
    }
    state->m_current_turn = 0;
    state->m_age_of_exploration = true;
    PTO_INIT();
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
    if (!state->m_combat_mods.setup(*g_rt_statics)) {
        return false;
    }
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
    if (!state->m_combat_mods.setup(*g_rt_statics)) {
        return false;
    }
    MapGenReq gen_req = req;
    gen_req.m_statics = g_rt_statics;
    MakeMapRslt rslt = g_map_loader.generate(gen_req);
    if (!rslt.m_ok) {
        std::printf("setup_new_game: map generate failed\n");
        state->clear();
        return false;
    }
    if (!Factory_GameArraySimple::load_from_rslt(&state->m_map, rslt)) {
        std::printf("setup_new_game: load_from_rslt failed\n");
        g_map_loader.free_rslt(&rslt);
        state->clear();
        return false;
    }
    g_map_loader.free_rslt(&rslt);
    if (!complete_new_game(state, player_n)) {
        std::printf("setup_new_game: complete_new_game failed\n");
        return false;
    }
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
    if (!state->m_combat_mods.setup(*g_rt_statics)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, paths.m_terr, paths.m_clim, paths.m_riv, paths.m_ov)) {
        return false;
    }
    if (paths.m_res != nullptr && !Factory_GameArraySimple::load_res_dist_data(&state->m_map, paths.m_res)) {
        state->clear();
        return false;
    }
    if (paths.m_flags != nullptr && !Factory_GameArraySimple::load_flags_data(&state->m_map, paths.m_flags)) {
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
