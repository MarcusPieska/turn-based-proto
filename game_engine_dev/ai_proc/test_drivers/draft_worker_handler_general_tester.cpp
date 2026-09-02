//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "build_adds_array.h"
#include "bit_array.h"
#include "building_trait_orderings.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_tile_manager.h"
#include "city_turn_handler.h"
#include "draft_worker_handler_general.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "map_bit_overlay.h"
#include "player_ledger.h"
#include "runtime_static_loader.h"
#include "std_add_helper.h"
#include "tile_working.h"
#include "tile_yields.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_RESULTS = "/home/w/Projects/simple-map-gen/draft_worker_handler_results.txt";
static const char* G_OUT_ROOT = "/home/w/Projects/simple-map-gen/draft-worker-handler-test";
static const u32 G_SEED = 43u;
static const u16 G_CITY_MAX = 5000;
static const u16 G_WORKER_MAX = 5000;
static const u32 G_TURNS = 100u;
static const u32 G_IMG_EVERY = 1u;
static const u16 G_EDGE_PAD = 4;
static const u16 G_DIST_BLACK = 2;
static const u16 G_DIST_GRASS = 6;
static const u16 G_DIST_PLAINS = 10;
static const u16 G_CLAIM_CULT = 100u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];

static u16 g_cx[G_CITY_MAX];
static u16 g_cy[G_CITY_MAX];
static u16 g_city_idx[G_CITY_MAX];
static u16 g_wk[G_WORKER_MAX];
static u16 g_city_n = 0;
static u16 g_wk_n = 0;

#ifndef DRAFT_WORKER_GENERAL_IMPL_TAG
#define DRAFT_WORKER_GENERAL_IMPL_TAG "general_mk01"
#endif
#ifndef DRAFT_WORKER_GENERAL_MK
#define DRAFT_WORKER_GENERAL_MK "mk01"
#endif

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool ensure_out_dir (char* out_dir, size_t out_n) {
    if (mkdir(G_OUT_ROOT, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    if (std::snprintf(out_dir, out_n, "%s/%s", G_OUT_ROOT, DRAFT_WORKER_GENERAL_MK) <= 0) {
        return false;
    }
    return mkdir(out_dir, 0755) == 0 || errno == EEXIST;
}

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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool is_city_terr (u8 terr) {
    return terr == TERR_PLAINS[0] || terr == TERR_HILLS[0];
}

static bool is_city_clim (u8 clim) {
    return clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS || clim == CLIMATE_BLACK_SOIL;
}

static u16 clim_min_dist (u8 clim) {
    if (clim == CLIMATE_BLACK_SOIL) {
        return G_DIST_BLACK;
    }
    if (clim == CLIMATE_GRASSLAND) {
        return G_DIST_GRASS;
    }
    if (clim == CLIMATE_PLAINS) {
        return G_DIST_PLAINS;
    }
    return G_DIST_PLAINS;
}

static u16 cheb_dist (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u16 adx = static_cast<u16>(dx < 0 ? -dx : dx);
    const u16 ady = static_cast<u16>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static bool far_enough (u16 x, u16 y, u16 min_d) {
    for (u16 i = 0; i < g_city_n; ++i) {
        if (cheb_dist(x, y, g_cx[i], g_cy[i]) < min_d) {
            return false;
        }
    }
    return true;
}

static u16 collect_city_pts (const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    g_city_n = 0;
    for (u16 y = G_EDGE_PAD; y + G_EDGE_PAD < h && g_city_n < G_CITY_MAX; ++y) {
        for (u16 x = G_EDGE_PAD; x + G_EDGE_PAD < w && g_city_n < G_CITY_MAX; ++x) {
            if (map.get_river(x, y) == 0) {
                continue;
            }
            if (!is_city_terr(map.get_terrain(x, y)) || !is_city_clim(map.get_climate(x, y))) {
                continue;
            }
            const u16 min_d = clim_min_dist(map.get_climate(x, y));
            if (!far_enough(x, y, min_d)) {
                continue;
            }
            g_cx[g_city_n] = x;
            g_cy[g_city_n] = y;
            g_city_n = static_cast<u16>(g_city_n + 1u);
        }
    }
    return g_city_n;
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

static void refill_mp (GameState& s, u16 unit_idx) {
    UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || s.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    if (u->m_unit_typ_idx >= typ_n) {
        return;
    }
    const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * s.m_statics->config().get_mov_pt_per_turn());
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static bool wr_result_ppm (const GameState& state, cstr path) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            terr_rgb(state.m_map.get_terrain(x, y), &p[0], &p[1], &p[2]);
            if (state.m_map.get_river(x, y) != 0) {
                p[0] = 0;
                p[1] = 180;
                p[2] = 255;
            }
            if (state.m_map.get_add_typ(x, y) == BUILD_ADD_STD &&
                state.m_map.get_add_idx(x, y) != U16_KEY_NULL &&
                StdAddHelper::has_farm(state.m_map.tile(x, y))) {
                p[0] = 255;
                p[1] = 0;
                p[2] = 0;
            } else if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
                p[0] = 0;
                p[1] = 0;
                p[2] = 0;
            }
        }
    }
    for (u16 i = 0; i < g_wk_n; ++i) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(g_wk[i]));
        if (u == nullptr || u->m_x == U16_KEY_NULL || u->m_x >= w || u->m_y >= h) {
            continue;
        }
        u8* p = &rgb[(static_cast<size_t>(u->m_y) * w + u->m_x) * 3u];
        p[0] = 128;
        p[1] = 0;
        p[2] = 0;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool init_state (GameState* state, RuntimeStatics& st) {
    state->clear();
    state->m_statics = &st;
    if (!UnitMovementMng::setup_mvt_costs(st)) {
        return false;
    }
    if (!TileYields::setup(st)) {
        return false;
    }
    if (!BuildingTraitOrderings::begin(st.building(), st.trait_affinity_map())) {
        return false;
    }
    state->m_civ_relations.reset(st.civ().get_item_count());
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, g_terr, g_clim, g_riv)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state->m_map, g_res)) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    if (!state->m_cities.bind_statics(st)) {
        return false;
    }
    const u16 wonder_n = st.wonder().get_item_count();
    if (wonder_n > 0) {
        state->m_wonder_city = new u16[wonder_n];
        if (state->m_wonder_city == nullptr) {
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
    CityBorder::bind_map(&state->m_map);
    CityTileManager::bind_cities(&state->m_cities);
    TileYields::bind_map(&state->m_map);
    TileWorking::bind_map(&state->m_map);
    PlayerState* seat = new PlayerState();
    if (seat == nullptr) {
        return false;
    }
    const u16 civ_n = st.civ().get_item_count();
    seat->m_ai_controlled = 1;
    seat->m_is_active = 1;
    seat->m_civ_index = civ_n == 0 ? 0 : static_cast<u16>(G_SEED % civ_n);
    seat->m_explored_overlay = new MapBitOverlay(w, h);
    seat->m_techs_researched = new BitArrayCL(st.tech().get_item_count());
    if (seat->m_techs_researched != nullptr) {
        const u32 tech_n = seat->m_techs_researched->get_count();
        for (u32 i = 0; i < tech_n; ++i) {
            seat->m_techs_researched->set_bit(i);
        }
    }
    const u16 sw_n = st.small_wonder().get_item_count();
    if (sw_n > 0) {
        seat->m_small_wonder_city = new u16[sw_n];
        if (seat->m_small_wonder_city == nullptr) {
            delete seat->m_techs_researched;
            delete seat->m_explored_overlay;
            delete seat;
            return false;
        }
        for (u16 i = 0; i < sw_n; ++i) {
            seat->m_small_wonder_city[i] = U16_KEY_NULL;
        }
    }
    if (seat->m_explored_overlay == nullptr || seat->m_techs_researched == nullptr) {
        delete[] seat->m_small_wonder_city;
        delete seat->m_techs_researched;
        delete seat->m_explored_overlay;
        delete seat;
        return false;
    }
    state->m_player_states = seat;
    state->m_player_n = 1;
    state->m_players_remaining = 1;
    state->m_small_wonder_count = sw_n;
    City::bind_player_states(state->m_player_states, state->m_player_n);
    state->m_land_worker_type_idx = U16_KEY_NULL;
    const u16 tn = st.unit_type().get_item_count();
    for (u16 i = 0; i < tn; ++i) {
        cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "LAND_WORKER") == 0) {
            state->m_land_worker_type_idx = i;
            break;
        }
    }
    return true;
}

static bool is_draft_wk (u16 unit_idx) {
    for (u16 i = 0; i < g_wk_n; ++i) {
        if (g_wk[i] == unit_idx) {
            return true;
        }
    }
    return false;
}

static void destroy_non_draft_units (GameState& state) {
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const u16 unit_idx = static_cast<u16>(idx);
        if (is_draft_wk(unit_idx)) {
            continue;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        UnitMovementMng::destroy_unit(state, UnitAddKey::from_raw(unit_idx));
    }
}

static void run_city_turns (GameState& state) {
    for (u16 i = 0; i < g_city_n; ++i) {
        CityTurnHandler::handle(state, g_city_idx[i]);
    }
}

static void farm_stats (const GameState& state, u32* farm_n, u64* food_sum) {
    *farm_n = 0;
    *food_sum = 0;
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (state.m_map.get_add_typ(x, y) != BUILD_ADD_STD) {
                continue;
            }
            if (state.m_map.get_add_idx(x, y) == U16_KEY_NULL) {
                continue;
            }
            if (!StdAddHelper::has_farm(state.m_map.tile(x, y))) {
                continue;
            }
            *farm_n = *farm_n + 1u;
            *food_sum = *food_sum + static_cast<u64>(TileYields::get(x, y).m_food);
        }
    }
}

static u32 total_pop (const GameState& state) {
    u32 pop = 0;
    for (u16 i = 0; i < g_city_n; ++i) {
        const City* c = state.m_cities.get_city(g_city_idx[i]);
        if (c != nullptr) {
            pop = pop + static_cast<u32>(c->get_current_population());
        }
    }
    return pop;
}

static u32 home_pop (const GameState& state) {
    u32 pop = 0;
    for (u16 i = 0; i < g_wk_n; ++i) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(g_wk[i]));
        if (u == nullptr) {
            continue;
        }
        const City* c = state.m_cities.get_city(WorkerHelper::get_data(u));
        if (c != nullptr) {
            pop = pop + static_cast<u32>(c->get_current_population());
        }
    }
    return pop;
}

static void city_dev_stats (const GameState& state, u32* bld_n, double* avg_sanit) {
    *bld_n = 0;
    *avg_sanit = 0.0;
    if (state.m_statics == nullptr || g_city_n == 0) {
        return;
    }
    const u16 bn = state.m_statics->building().get_item_count();
    u64 sanit_sum = 0;
    for (u16 i = 0; i < g_city_n; ++i) {
        const City* c = state.m_cities.get_city(g_city_idx[i]);
        if (c == nullptr) {
            continue;
        }
        for (u16 b = 0; b < bn; ++b) {
            if (c->has_building(g_city_idx[i], b)) {
                *bld_n = *bld_n + 1u;
            }
        }
        sanit_sum = sanit_sum + static_cast<u64>(c->get_city_sanitation_boost(g_city_idx[i]));
    }
    *avg_sanit = static_cast<double>(sanit_sum) / static_cast<double>(g_city_n);
}

static void worked_plains_stats (
    const GameState& state,
    u32* plains_farm,
    u32* plains_only,
    u32* plains_desert,
    u32* plains_city)
{
    *plains_farm = 0;
    *plains_only = 0;
    *plains_desert = 0;
    *plains_city = 0;
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (TileWorking::get_worker(x, y) == U16_KEY_NULL) {
                continue;
            }
            if (state.m_map.get_terrain(x, y) != TERR_PLAINS[0]) {
                continue;
            }
            if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
                *plains_city = *plains_city + 1u;
                continue;
            }
            const bool farm = state.m_map.get_add_typ(x, y) == BUILD_ADD_STD
                && state.m_map.get_add_idx(x, y) != U16_KEY_NULL
                && StdAddHelper::has_farm(state.m_map.tile(x, y));
            if (farm) {
                *plains_farm = *plains_farm + 1u;
            } else if (state.m_map.get_climate(x, y) == CLIMATE_DESERT) {
                *plains_desert = *plains_desert + 1u;
            } else {
                *plains_only = *plains_only + 1u;
            }
        }
    }
}

static u16 cheb_u16 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u16 adx = static_cast<u16>(dx < 0 ? -dx : dx);
    const u16 ady = static_cast<u16>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static u16 work_rad_cheb () {
    const CircArea area = CityTileManager::work_area();
    u16 rad = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 ax = area.m_brd[i][0] < 0 ? -area.m_brd[i][0] : area.m_brd[i][0];
        const i32 ay = area.m_brd[i][1] < 0 ? -area.m_brd[i][1] : area.m_brd[i][1];
        const u16 c = static_cast<u16>(ax > ay ? ax : ay);
        if (c > rad) {
            rad = c;
        }
    }
    return rad == 0 ? 2u : rad;
}

static bool terr_walkable_land (const GameState& state, u16 x, u16 y) {
    const u8 t = state.m_map.get_terrain(x, y);
    return t == TERR_PLAINS[0] || t == TERR_HILLS[0];
}

static bool bfs_in_disk (
    const GameState& state,
    u16 sx,
    u16 sy,
    u16 gx,
    u16 gy,
    u16 cx,
    u16 cy,
    u16 rad)
{
    if (sx == gx && sy == gy) {
        return true;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u32 cells = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> seen(cells, 0);
    std::vector<u32> q;
    q.reserve(64);
    const u32 sidx = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    seen[sidx] = 1;
    q.push_back(sidx);
    static const i32 kdx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const i32 kdy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    for (u32 qi = 0; qi < q.size(); ++qi) {
        const u32 cur = q[qi];
        const u16 x = static_cast<u16>(cur % static_cast<u32>(w));
        const u16 y = static_cast<u16>(cur / static_cast<u32>(w));
        for (u8 d = 0; d < 8u; ++d) {
            const i32 nx = static_cast<i32>(x) + kdx[d];
            const i32 ny = static_cast<i32>(y) + kdy[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (cheb_u16(ux, uy, cx, cy) > rad) {
                continue;
            }
            if (!terr_walkable_land(state, ux, uy)) {
                continue;
            }
            const u32 nidx = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
            if (seen[nidx] != 0) {
                continue;
            }
            if (ux == gx && uy == gy) {
                return true;
            }
            seen[nidx] = 1;
            q.push_back(nidx);
        }
    }
    return false;
}

static void leftover_plains_diag (const GameState& state) {
    const u16 rad = work_rad_cheb();
    u32 n = 0;
    u32 no_wk = 0;
    u32 on_tile = 0;
    u32 bad_typ = 0;
    u32 unowned = 0;
    u32 reach = 0;
    u32 unreach = 0;
    u32 greedy_block = 0;
    u32 typ_hist[16] = {};
    for (u16 y = 0; y < state.m_map.height(); ++y) {
        for (u16 x = 0; x < state.m_map.width(); ++x) {
            const u16 city_idx = TileWorking::get_worker(x, y);
            if (city_idx == U16_KEY_NULL) {
                continue;
            }
            if (state.m_map.get_terrain(x, y) != TERR_PLAINS[0]) {
                continue;
            }
            if (state.m_map.get_climate(x, y) == CLIMATE_DESERT) {
                continue;
            }
            const bool farm = state.m_map.get_add_typ(x, y) == BUILD_ADD_STD
                && state.m_map.get_add_idx(x, y) != U16_KEY_NULL
                && StdAddHelper::has_farm(state.m_map.tile(x, y));
            if (farm) {
                continue;
            }
            n = n + 1u;
            const u8 atyp = state.m_map.get_add_typ(x, y);
            if (atyp != BUILD_ADD_STD) {
                bad_typ = bad_typ + 1u;
                if (atyp < 16u) {
                    typ_hist[atyp] = typ_hist[atyp] + 1u;
                }
                continue;
            }
            const City* c = state.m_cities.get_city(city_idx);
            if (c == nullptr) {
                no_wk = no_wk + 1u;
                continue;
            }
            if (state.m_map.get_civ_owner(x, y) != static_cast<u8>(c->get_owner())) {
                unowned = unowned + 1u;
                continue;
            }
            u16 wk_i = U16_KEY_NULL;
            for (u16 i = 0; i < g_wk_n; ++i) {
                const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(g_wk[i]));
                if (u != nullptr && WorkerHelper::get_data(u) == city_idx) {
                    wk_i = g_wk[i];
                    break;
                }
            }
            if (wk_i == U16_KEY_NULL) {
                no_wk = no_wk + 1u;
                continue;
            }
            const UnitAddStruct* wu = state.m_units.get_unit_add(UnitAddKey::from_raw(wk_i));
            if (wu == nullptr) {
                no_wk = no_wk + 1u;
                continue;
            }
            if (wu->m_x == x && wu->m_y == y) {
                on_tile = on_tile + 1u;
                continue;
            }
            const u16 cx = c->get_x();
            const u16 cy = c->get_y();
            if (!bfs_in_disk(state, wu->m_x, wu->m_y, x, y, cx, cy, rad)) {
                unreach = unreach + 1u;
                continue;
            }
            reach = reach + 1u;
            const u16 d0 = cheb_u16(wu->m_x, wu->m_y, x, y);
            bool improv = false;
            static const i32 kdx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
            static const i32 kdy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
            for (u8 d = 0; d < 8u; ++d) {
                const i32 nx = static_cast<i32>(wu->m_x) + kdx[d];
                const i32 ny = static_cast<i32>(wu->m_y) + kdy[d];
                if (nx < 0 || ny < 0) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                if (cheb_u16(ux, uy, cx, cy) > rad) {
                    continue;
                }
                if (!terr_walkable_land(state, ux, uy)) {
                    continue;
                }
                if (cheb_u16(ux, uy, x, y) < d0) {
                    improv = true;
                    break;
                }
            }
            if (!improv) {
                greedy_block = greedy_block + 1u;
            }
        }
    }
    std::printf(
        "leftover_plains_diag n=%u bad_typ=%u unowned=%u no_wk=%u on_tile=%u bfs_unreach=%u bfs_reach=%u greedy_no_improv=%u rad=%u\n",
        (unsigned)n,
        (unsigned)bad_typ,
        (unsigned)unowned,
        (unsigned)no_wk,
        (unsigned)on_tile,
        (unsigned)unreach,
        (unsigned)reach,
        (unsigned)greedy_block,
        (unsigned)rad);
    std::printf("leftover_add_typ_hist");
    for (u8 t = 0; t < 16u; ++t) {
        if (typ_hist[t] != 0) {
            std::printf(" typ%u=%u", (unsigned)t, (unsigned)typ_hist[t]);
        }
    }
    std::printf("\n");
}

static void append_results (cstr line) {
    FILE* f = std::fopen(G_RESULTS, "a");
    if (f == nullptr) {
        std::printf("fail append %s\n", G_RESULTS);
        return;
    }
    std::fputs(line, f);
    std::fputc('\n', f);
    std::fclose(f);
}

static void print_results_file () {
    FILE* f = std::fopen(G_RESULTS, "r");
    if (f == nullptr) {
        std::printf("(no results file yet: %s)\n", G_RESULTS);
        return;
    }
    std::printf("======== %s ========\n", G_RESULTS);
    char buf[512];
    while (std::fgets(buf, sizeof(buf), f) != nullptr) {
        std::fputs(buf, stdout);
    }
    std::fclose(f);
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    char out_dir[384];
    if (!ensure_out_dir(out_dir, sizeof(out_dir))) {
        std::printf("fail mkdir out dir under %s/%s\n", G_OUT_ROOT, DRAFT_WORKER_GENERAL_MK);
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    GameState state;
    if (!init_state(&state, st)) {
        std::printf("fail init state\n");
        return 1;
    }
    const u16 city_n = collect_city_pts(state.m_map);
    if (city_n == 0) {
        std::printf("fail city pts got 0\n");
        return 1;
    }
    for (u16 i = 0; i < city_n; ++i) {
        const u16 idx = state.m_cities.get_next_new_city_idx();
        City* city = state.m_cities.get_city(idx);
        if (city == nullptr) {
            std::printf("fail city alloc at %u\n", (unsigned)i);
            return 1;
        }
        city->init(0, g_cx[i], g_cy[i]);
        if (!state.m_map.set_tile_add(g_cx[i], g_cy[i], idx, BUILD_ADD_CITY)) {
            std::printf("fail city tile at (%u,%u)\n", (unsigned)g_cx[i], (unsigned)g_cy[i]);
            return 1;
        }
        CityBorder::claim_expand(g_cx[i], g_cy[i], 0, G_CLAIM_CULT, 0);
        g_city_idx[i] = idx;
    }
    const u16 worker_typ = find_worker_typ(st);
    if (worker_typ == U16_KEY_NULL) {
        std::printf("fail find worker typ\n");
        return 1;
    }
    g_wk_n = 0;
    for (u16 i = 0; i < city_n && g_wk_n < G_WORKER_MAX; ++i) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, g_cx[i], g_cy[i], 0, worker_typ, &key)) {
            continue;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            continue;
        }
        WorkerHelper::set_data(u, g_city_idx[i]);
        g_wk[g_wk_n] = key.value();
        g_wk_n = static_cast<u16>(g_wk_n + 1u);
    }
    if (g_wk_n == 0) {
        std::printf("fail spawn workers\n");
        return 1;
    }
    std::printf("civ_index=%u workers=%u cities=%u turns=%u\n",
        (unsigned)state.m_player_states[0].m_civ_index,
        (unsigned)g_wk_n,
        (unsigned)city_n,
        (unsigned)G_TURNS);
    if (!DraftWorkerHandlerGeneral::begin(state)) {
        std::printf("fail draft worker begin\n");
        return 1;
    }
    char path[512];
    u64 handle_ns = 0;
    u64 handle_n = 0;
    for (u32 t = 1; t <= G_TURNS; ++t) {
        run_city_turns(state);
        destroy_non_draft_units(state);
        for (u16 i = 0; i < g_wk_n; ++i) {
            refill_mp(state, g_wk[i]);
            const auto t0 = std::chrono::high_resolution_clock::now();
            DraftWorkerHandlerGeneral::handle(state, g_wk[i]);
            const auto t1 = std::chrono::high_resolution_clock::now();
            handle_ns += static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            handle_n = handle_n + 1ull;
        }
        if ((t % G_IMG_EVERY) == 0u) {
            std::snprintf(path, sizeof(path), "%s/farms_cities_t%03u.ppm", out_dir, (unsigned)t);
            if (!wr_result_ppm(state, path)) {
                std::printf("fail write %s\n", path);
                DraftWorkerHandlerGeneral::clear();
                BuildingTraitOrderings::clear();
                state.clear();
                return 1;
            }
            u32 turn_farms = 0;
            u64 turn_food = 0;
            farm_stats(state, &turn_farms, &turn_food);
            std::printf("t=%u farms=%u wrote %s\n", (unsigned)t, (unsigned)turn_farms, path);
        }
    }
    DraftWorkerHandlerGeneral::clear();
    BuildingTraitOrderings::clear();
    u32 farm_n = 0;
    u64 food_sum = 0;
    farm_stats(state, &farm_n, &food_sum);
    const u32 pop = total_pop(state);
    const u32 hpop = home_pop(state);
    u32 bld_n = 0;
    double avg_sanit = 0.0;
    city_dev_stats(state, &bld_n, &avg_sanit);
    u32 plains_farm = 0;
    u32 plains_only = 0;
    u32 plains_desert = 0;
    u32 plains_city = 0;
    worked_plains_stats(state, &plains_farm, &plains_only, &plains_desert, &plains_city);
    leftover_plains_diag(state);
    const double avg_us = (handle_n == 0) ? 0.0 : (static_cast<double>(handle_ns) / static_cast<double>(handle_n)) / 1000.0;
    const double avg_food = (farm_n == 0) ? 0.0 : static_cast<double>(food_sum) / static_cast<double>(farm_n);
    char line[512];
    std::snprintf(line, sizeof(line),
        "impl=%s avg_us=%.3f farms=%u avg_food_per_farm=%.3f pop=%u home_pop=%u blds=%u avg_sanit=%.2f worked_plains_farm=%u worked_plains_only=%u worked_plains_desert=%u worked_plains_city=%u workers=%u cities=%u turns=%u",
        DRAFT_WORKER_GENERAL_IMPL_TAG,
        avg_us,
        (unsigned)farm_n,
        avg_food,
        (unsigned)pop,
        (unsigned)hpop,
        (unsigned)bld_n,
        avg_sanit,
        (unsigned)plains_farm,
        (unsigned)plains_only,
        (unsigned)plains_desert,
        (unsigned)plains_city,
        (unsigned)g_wk_n,
        (unsigned)city_n,
        (unsigned)G_TURNS);
    std::printf("%s\n", line);
    append_results(line);
    print_results_file();
    CityTileManager::bind_cities(nullptr);
    CityBorder::bind_map(nullptr);
    TileYields::bind_map(nullptr);
    TileWorking::bind_map(nullptr);
    City::bind_player_states(nullptr, 0);
    City::bind_units(nullptr);
    City::bind_wonder_cities(nullptr);
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
