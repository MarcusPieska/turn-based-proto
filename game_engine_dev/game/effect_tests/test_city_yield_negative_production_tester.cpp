//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h> 

#include "building_static_key.h"
#include "city.h"
#include "city_array.h"
#include "city_tile_manager.h"
#include "factory_game_array_simple.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tile_yields.h"
#include "circular_tile_areas.h"
#include "game_map_defs.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 1u;
static const u16 G_LATT_ORIG = 10u;
static const u16 G_LATT_STEP = 20u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

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

static bool is_water (u8 terr) {
    return terr == TERR_OCEAN[0]
        || terr == TERR_SEA[0]
        || terr == TERR_COASTAL[0]
        || terr == TERR_INLAND_SEA[0]
        || terr == TERR_INLAND_LAKE[0];
}

static bool is_mountain (u8 terr) {
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static u16 find_bld_idx (const RuntimeStatics& st, const char* want) {
    const u16 bld_n = st.building().get_item_count();
    for (u16 i = 0; i < bld_n; ++i) {
        const char* nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, want) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool boot_at_site (
    GameSetup& setup,
    GameState& state,
    const RuntimeStatics* st,
    u16 x,
    u16 y
) {
    state.clear();
    state.m_statics = st;
    state.m_civ_relations.reset(st->civ().get_item_count());
    if (!state.m_combat_mods.setup(*st)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_map_gen_data(&state.m_map, g_terr, g_clim, g_riv, g_ov)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state.m_map, g_res)) {
        return false;
    }
    SpgPickCoords starts = {};
    starts.n = 1;
    starts.pts[0].x = x;
    starts.pts[0].y = y;
    return setup.finish_with_starts(&state, starts, G_PLAYERS);
}

static bool compute_prod_store_for_city (
    GameState& state,
    u16 city_idx,
    bool with_armory,
    u16 armory_bld_idx,
    u16* out_prod_store
) {
    if (state.m_statics == nullptr || state.m_player_states == nullptr || out_prod_store == nullptr) {
        return false;
    }
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr || city->get_owner() == U16_KEY_NULL) {
        return false;
    }
    const u16 player = city->get_owner();

    city->set_population(1u);
    if (with_armory) {
        state.m_cities.set_building_flag(city_idx, armory_bld_idx);
    }

    CityTileManager::maximize_production(player, city_idx);
    const TotalTileYield yld = CityTileManager::gather_yields(player, city_idx);
    const u16 worked_prod = static_cast<u16>(yld.m_production > 65535u ? 65535u : yld.m_production);

    city->add_production(city_idx, worked_prod);
    *out_prod_store = city->get_current_production_store();
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }

    GameSetup setup;
    GameState probe;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&probe, paths, G_PLAYERS)) {
        std::printf("probe setup_new_game failed\n");
        return 1;
    }
    const RuntimeStatics* st = probe.m_statics;
    if (st == nullptr) {
        std::printf("probe statics null\n");
        probe.clear();
        return 1;
    }

    const u16 armory_bld_idx = find_bld_idx(*st, "Armory");
    if (armory_bld_idx == U16_KEY_NULL) {
        std::printf("Armory not found\n");
        probe.clear();
        return 1;
    }

    const u16 map_w = probe.m_map.width();
    const u16 map_h = probe.m_map.height();
    u16 pick_x = U16_KEY_NULL;
    u16 pick_y = U16_KEY_NULL;
    for (u16 y = G_LATT_ORIG; pick_x == U16_KEY_NULL && y < map_h; y = static_cast<u16>(y + G_LATT_STEP)) {
        for (u16 x = G_LATT_ORIG; pick_x == U16_KEY_NULL && x < map_w; x = static_cast<u16>(x + G_LATT_STEP)) {
            const u8 terr = probe.m_map.get_terrain(x, y);
            if (is_water(terr) || is_mountain(terr)) {
                continue;
            }
            if (probe.m_map.get_climate(x, y) == CLIMATE_DESERT) {
                continue;
            }

            GameState state;
            if (!boot_at_site(setup, state, st, x, y)) {
                continue;
            }
            City* city = state.m_cities.get_city(0);
            if (city == nullptr || city->get_owner() == U16_KEY_NULL) {
                state.clear();
                continue;
            }
            city->set_population(1u);
            CityTileManager::maximize_production(city->get_owner(), 0);
            const TotalTileYield yld = CityTileManager::gather_yields(city->get_owner(), 0);
            const u16 worked_prod = static_cast<u16>(yld.m_production > 65535u ? 65535u : yld.m_production);
            const u16 center_prod = TileYields::get(city->get_x(), city->get_y()).m_production;
            const u32 total_prod = static_cast<u32>(worked_prod) + static_cast<u32>(center_prod);
            if (total_prod < 1u) {
                state.clear();
                continue;
            }
            pick_x = x;
            pick_y = y;
            state.clear();
        }
    }

    if (pick_x == U16_KEY_NULL) {
        std::printf("no suitable lattice site found\n");
        probe.clear();
        return 1;
    }

    GameState base;
    GameState with;
    if (!boot_at_site(setup, base, st, pick_x, pick_y)) {
        std::printf("boot base failed\n");
        probe.clear();
        return 1;
    }
    if (!boot_at_site(setup, with, st, pick_x, pick_y)) {
        std::printf("boot with failed\n");
        base.clear();
        probe.clear();
        return 1;
    }

    u16 store_base = 0;
    u16 store_with = 0;
    if (!compute_prod_store_for_city(base, 0, false, armory_bld_idx, &store_base)) {
        std::printf("compute base failed\n");
        base.clear();
        with.clear();
        probe.clear();
        return 1;
    }
    if (!compute_prod_store_for_city(with, 0, true, armory_bld_idx, &store_with)) {
        std::printf("compute with failed\n");
        base.clear();
        with.clear();
        probe.clear();
        return 1;
    }

    std::printf("site (%u,%u) store_base=%u store_with=%u delta=%d\n",
        static_cast<unsigned>(pick_x), static_cast<unsigned>(pick_y),
        static_cast<unsigned>(store_base), static_cast<unsigned>(store_with),
        static_cast<int>(static_cast<i32>(store_with) - static_cast<i32>(store_base)));

    if (store_with + 1u != store_base) {
        std::printf("FAIL: expected armory to reduce production store by 1\n");
        base.clear();
        with.clear();
        probe.clear();
        return 1;
    }

    base.clear();
    with.clear();
    probe.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
