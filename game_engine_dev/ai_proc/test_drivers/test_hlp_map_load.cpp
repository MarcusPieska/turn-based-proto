//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_hlp_map_load.h"

#include <cstdio>
#include <cstring>

#include "city_border.h"
#include "city_tile_manager.h"
#include "combat_mng.h"
#include "factory_game_array_simple.h"
#include "game_state.h"
#include "runtime_static_loader.h"
#include "unit_movement_mng.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

static RuntimeStaticLoader g_loader;
static RuntimeStatics* g_st = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void join_path (char* dst, size_t n, cstr base, cstr leaf) {
    if (base == nullptr || leaf == nullptr || n == 0u) {
        if (n > 0u) {
            dst[0] = '\0';
        }
        return;
    }
    const size_t bl = std::strlen(base);
    if (bl > 0u && base[bl - 1u] == '/') {
        std::snprintf(dst, n, "%s%s", base, leaf);
    } else {
        std::snprintf(dst, n, "%s/%s", base, leaf);
    }
}

static void cache_unit_types (GameState& s) {
    if (g_st == nullptr) {
        return;
    }
    s.m_land_settler_type_idx = U16_KEY_NULL;
    s.m_land_worker_type_idx = U16_KEY_NULL;
    s.m_land_scout_type_idx = U16_KEY_NULL;
    s.m_land_defense_type_idx = U16_KEY_NULL;
    s.m_land_attack_type_idx = U16_KEY_NULL;
    s.m_land_mobile_type_idx = U16_KEY_NULL;
    s.m_land_artillery_type_idx = U16_KEY_NULL;
    s.m_land_paradrop_type_idx = U16_KEY_NULL;
    const u16 n = g_st->unit_type().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = g_st->unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            continue;
        }
        if (std::strcmp(nm, "LAND_SETTLER") == 0) {
            s.m_land_settler_type_idx = i;
        } else if (std::strcmp(nm, "LAND_WORKER") == 0) {
            s.m_land_worker_type_idx = i;
        } else if (std::strcmp(nm, "LAND_SCOUT") == 0) {
            s.m_land_scout_type_idx = i;
        } else if (std::strcmp(nm, "LAND_DEFENSE") == 0) {
            s.m_land_defense_type_idx = i;
        } else if (std::strcmp(nm, "LAND_ATTACK") == 0) {
            s.m_land_attack_type_idx = i;
        } else if (std::strcmp(nm, "LAND_MOBILE") == 0) {
            s.m_land_mobile_type_idx = i;
        } else if (std::strcmp(nm, "LAND_ARTILLERY") == 0) {
            s.m_land_artillery_type_idx = i;
        } else if (std::strcmp(nm, "LAND_PARADROP") == 0) {
            s.m_land_paradrop_type_idx = i;
        }
    }
}

//================================================================================================================================
//=> - TestHlpMapLoad -
//================================================================================================================================

RuntimeStatics* TestHlpMapLoad::statics () {
    return g_st;
}

bool TestHlpMapLoad::load (GameState& s, cstr in_base, cstr lib, cstr data) {
    if (in_base == nullptr || lib == nullptr || data == nullptr) {
        return false;
    }
    if (g_st == nullptr) {
        if (!g_loader.load(lib, data)) {
            return false;
        }
        g_st = &g_loader.statics();
    }
    char terr[512];
    char clim[512];
    char riv[512];
    char res[512];
    join_path(terr, sizeof(terr), in_base, "terrain.ppm");
    join_path(clim, sizeof(clim), in_base, "climate.ppm");
    join_path(riv, sizeof(riv), in_base, "rivers.ppm");
    join_path(res, sizeof(res), in_base, "resources.ppm");
    s.clear();
    s.m_statics = g_st;
    if (!UnitMovementMng::setup_mvt_costs(*g_st)) {
        return false;
    }
    if (!CombatMng::setup(*g_st)) {
        return false;
    }
    if (!s.m_combat_mods.setup(*g_st)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_map_gen_data(&s.m_map, terr, clim, riv)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&s.m_map, res)) {
        return false;
    }
    if (s.m_map.width() == 0 || s.m_map.height() == 0) {
        return false;
    }
    if (!s.m_cities.bind_statics(*g_st)) {
        return false;
    }
    CityBorder::bind_map(&s.m_map);
    CityTileManager::bind_cities(&s.m_cities);
    UnitMovementMng::bind_state(&s);
    cache_unit_types(s);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
