//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "bit_array.h"
#include "building_static_key.h"
#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "general_bit_bank.h"
#include "runtime_statics.h"
#include "settle_rules.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 1u;

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

static bool found_second_city (GameState& state, u16 x, u16 y, u16 owner) {
    if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        return false;
    }
    const u16 city_idx = state.m_cities.get_next_new_city_idx();
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(owner, x, y);
    if (!state.m_map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY)) {
        return false;
    }
    if (!state.city_net_on_found(city_idx)) {
        return false;
    }
    CityBorder::claim_disc(x, y, 1u, static_cast<u8>(owner));
    return true;
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

static void grant_all_resources (GameState& state, u16 city_idx) {
    GeneralBitBank* bank = state.m_cities.get_res_bank();
    if (bank == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u16 res_n = state.m_statics->resource().get_item_count();
    for (u16 i = 0; i < res_n; ++i) {
        bank->set_flag(city_idx, i);
    }
}

static void copy_bits (BitArrayCL& dst, const BitArrayCL& src) {
    const u32 n = dst.get_count();
    dst.clear_all();
    for (u32 i = 0; i < n; ++i) {
        if (src.get_bit(i) != 0) {
            dst.set_bit(i);
        }
    }
}

static bool find_grass_river_site (const GameArraySimple& map, u16* out_x, u16* out_y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_climate(x, y) != CLIMATE_GRASSLAND) {
                continue;
            }
            if (map.get_river(x, y) == 0u) {
                continue;
            }
            if (!SettleRules::tile_ok(map, x, y)) {
                continue;
            }
            *out_x = x;
            *out_y = y;
            return true;
        }
    }
    return false;
}

static bool find_desert_terr_site (const GameArraySimple& map, u16* out_x, u16* out_y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_climate(x, y) != CLIMATE_DESERT) {
                continue;
            }
            if (!SettleRules::terr_ok(map.get_terrain(x, y))) {
                continue;
            }
            *out_x = x;
            *out_y = y;
            return true;
        }
    }
    return false;
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

    u16 grass_x = U16_KEY_NULL;
    u16 grass_y = U16_KEY_NULL;
    u16 desert_x = U16_KEY_NULL;
    u16 desert_y = U16_KEY_NULL;
    if (!find_grass_river_site(probe.m_map, &grass_x, &grass_y)) {
        std::printf("no grassland+river settle site\n");
        probe.clear();
        return 1;
    }
    if (!find_desert_terr_site(probe.m_map, &desert_x, &desert_y)) {
        std::printf("no desert plains/hills site\n");
        probe.clear();
        return 1;
    }

    GameState state;
    if (!boot_at_site(setup, state, st, grass_x, grass_y)) {
        std::printf("boot grassland city failed\n");
        probe.clear();
        return 1;
    }
    City* grass_city = state.m_cities.get_city(0);
    if (grass_city == nullptr || grass_city->get_owner() == U16_KEY_NULL) {
        std::printf("grassland city missing\n");
        state.clear();
        probe.clear();
        return 1;
    }
    const u16 owner = grass_city->get_owner();

    GameTileSimple* desert_tile = state.m_map.tile(desert_x, desert_y);
    if (desert_tile == nullptr) {
        std::printf("desert tile null\n");
        state.clear();
        probe.clear();
        return 1;
    }
    desert_tile->m_riv = 1u;
    if (!SettleRules::tile_ok(state.m_map, desert_x, desert_y)) {
        std::printf("desert+river stamp failed settle check\n");
        state.clear();
        probe.clear();
        return 1;
    }
    if (!found_second_city(state, desert_x, desert_y, owner)) {
        std::printf("found desert city failed\n");
        state.clear();
        probe.clear();
        return 1;
    }
    City* desert_city = state.m_cities.get_city(1);
    if (desert_city == nullptr || desert_city->get_owner() == U16_KEY_NULL) {
        std::printf("desert city missing\n");
        state.clear();
        probe.clear();
        return 1;
    }

    unlock_all_tech(state);
    grant_all_resources(state, 0);
    grant_all_resources(state, 1);

    PlayerState& ps = state.m_player_states[owner];
    BitArrayCL civ(st->civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }

    BitArrayCL* grass_raw = grass_city->get_buildable_buildings(0, ps.m_techs_researched, &civ);
    if (grass_raw == nullptr) {
        std::printf("grassland assess failed\n");
        state.clear();
        probe.clear();
        return 1;
    }
    BitArrayCL grass_bld(grass_raw->get_count());
    copy_bits(grass_bld, *grass_raw);

    BitArrayCL* desert_raw = desert_city->get_buildable_buildings(1, ps.m_techs_researched, &civ);
    if (desert_raw == nullptr) {
        std::printf("desert assess failed\n");
        state.clear();
        probe.clear();
        return 1;
    }
    BitArrayCL desert_bld(desert_raw->get_count());
    copy_bits(desert_bld, *desert_raw);

    const u16 bld_n = st->building().get_item_count();
    u16 desert_only_n = 0;
    u16 grass_only_n = 0;
    std::printf("sites grass=(%u,%u) desert=(%u,%u)\n",
        static_cast<unsigned>(grass_x), static_cast<unsigned>(grass_y),
        static_cast<unsigned>(desert_x), static_cast<unsigned>(desert_y));
    std::printf("building delta:\n");
    for (u16 i = 0; i < bld_n; ++i) {
        const u8 g = grass_bld.get_bit(i) != 0 ? 1u : 0u;
        const u8 d = desert_bld.get_bit(i) != 0 ? 1u : 0u;
        if (g == d) {
            continue;
        }
        const char* nm = st->building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            nm = "?";
        }
        if (d != 0 && g == 0) {
            std::printf("  desert-only: %s\n", nm);
            desert_only_n = static_cast<u16>(desert_only_n + 1u);
        } else {
            std::printf("  grassland-only: %s\n", nm);
            grass_only_n = static_cast<u16>(grass_only_n + 1u);
        }
    }
    std::printf("desert_only=%u grassland_only=%u\n",
        static_cast<unsigned>(desert_only_n), static_cast<unsigned>(grass_only_n));

    if (desert_only_n == 0) {
        std::printf("FAIL: expected at least one desert+river-only building\n");
        state.clear();
        probe.clear();
        return 1;
    }

    state.clear();
    probe.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
