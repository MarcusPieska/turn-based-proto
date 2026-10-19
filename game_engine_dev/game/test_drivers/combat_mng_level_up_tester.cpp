//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>
#include <limits>

#include "combat_mng.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "map_overlay_enum.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_level_static_data.h"
#include "unit_level_static_key.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;
static GameState g_st;
static const u32 k_n = 1000u;
static const cstr k_atk = "Swordsman";
static const cstr k_def = "Warrior";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool mk_map () {
    if (!Factory_GameArraySimple::init_test_grid(&g_st.m_map, 2u, 1u)) {
        return false;
    }
    GameTileSimple* a = g_st.m_map.tile(0u, 0u);
    GameTileSimple* b = g_st.m_map.tile(1u, 0u);
    a->m_clim = CLIMATE_NONE;
    b->m_clim = CLIMATE_NONE;
    a->m_road_typ = ROAD_NONE;
    b->m_road_typ = ROAD_NONE;
    a->m_terr = TERR_PLAINS[0];
    b->m_terr = TERR_PLAINS[0];
    a->m_ov = static_cast<u16>(MapOverlay::None);
    b->m_ov = static_cast<u16>(MapOverlay::None);
    a->m_riv = 0u;
    b->m_riv = 0u;
    return true;
}

static UnitAddStruct mk_unit (u16 typ_idx, u16 x, u16 y, u8 lv) {
    UnitAddStruct u = {};
    u.m_x = x;
    u.m_y = y;
    u.m_unit_typ_idx = typ_idx;
    u.m_next_unit_on_tile = U16_KEY_NULL;
    u.m_next_unit_in_group = U16_KEY_NULL;
    u.m_mvt_points = 0;
    u.m_player_idx = 0u;
    u.m_health = UNIT_HEALTH;
    u.m_level = lv;
    u.m_misc = 0u;
    return u;
}

static u16 find_unit (cstr name) {
    const UnitStaticData& ud = g_rt_statics->unit();
    const u16 n = ud.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(ud.get_name(UnitStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!load_statics()) {
        std::printf("FAILED load statics\n");
        return 1;
    }
    if (!TileAttrTables::setup(*g_rt_statics)) {
        std::printf("FAILED TileAttrTables::setup\n");
        return 1;
    }
    if (!CombatMng::setup(*g_rt_statics)) {
        std::printf("FAILED CombatMng::setup\n");
        return 1;
    }
    g_st.m_statics = g_rt_statics;
    if (!g_st.m_combat_mods.setup(*g_rt_statics)) {
        std::printf("FAILED CombatMods::setup\n");
        return 1;
    }
    if (!mk_map()) {
        std::printf("FAILED map\n");
        return 1;
    }

    const u16 atk_typ = find_unit(k_atk);
    const u16 def_typ = find_unit(k_def);
    if (atk_typ == U16_KEY_NULL || def_typ == U16_KEY_NULL) {
        std::printf("FAILED find units %s / %s\n", k_atk, k_def);
        return 1;
    }

    const UnitLevelStaticData& lvls = g_rt_statics->unit_level();
    const u16 lvl_n = lvls.get_item_count();
    if (lvl_n == 0u) {
        std::printf("FAILED no unit levels\n");
        return 1;
    }

    const UnitStaticData& ud = g_rt_statics->unit();
    std::printf("level-up sampler: %s (atk=%u) vs %s (def=%u), plains, n=%u\n",
        k_atk,
        static_cast<u32>(ud.get_item(UnitStaticDataKey::from_raw(atk_typ)).attack),
        k_def,
        static_cast<u32>(ud.get_item(UnitStaticDataKey::from_raw(def_typ)).defense),
        k_n);
    std::printf("%-14s %8s %8s %8s %10s %10s %10s %10s\n",
        "level", "cfg%", "ups", "wins", "deriv%", "min_us", "max_us", "avg_us");

    for (u16 lv = 0u; lv < lvl_n; ++lv) {
        const cstr lname = lvls.get_name(UnitLevelStaticDataKey::from_raw(lv));
        const u16 cfg = lvls.get_item(UnitLevelStaticDataKey::from_raw(lv)).up_prob;
        CombatMng::set_dials(85u, 25u, 1u);

        u32 ups = 0u;
        u32 wins = 0u;
        f64 t_sum = 0.0;
        f64 t_min = std::numeric_limits<f64>::max();
        f64 t_max = 0.0;

        for (u32 i = 0u; i < k_n; ++i) {
            UnitAddStruct atk = mk_unit(atk_typ, 0u, 0u, static_cast<u8>(lv));
            UnitAddStruct def = mk_unit(def_typ, 1u, 0u, 0u);
            const auto t0 = std::chrono::steady_clock::now();
            CombatMng::resolve_attack(atk, def, g_st, 1u, 0u);
            const auto t1 = std::chrono::steady_clock::now();
            const f64 us = static_cast<f64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()) / 1000.0;
            t_sum += us;
            if (us < t_min) {
                t_min = us;
            }
            if (us > t_max) {
                t_max = us;
            }
            if (atk.m_health > 0u && def.m_health == 0u) { 
                wins++;
            }
            if (atk.m_level > lv) {
                ups++;
            }
        }

        const f64 deriv = 100.0 * static_cast<f64>(ups) / static_cast<f64>(k_n);
        const f64 avg = t_sum / static_cast<f64>(k_n);
        std::printf("%-14s %7u%% %8u %8u %9.2f%% %10.3f %10.3f %10.3f\n",
            lname, static_cast<u32>(cfg), ups, wins, deriv, t_min, t_max, avg);
    }

    CombatMng::clear();
    TileAttrTables::clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
