//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "combat_mng.h"

#include <cmath>

#include "build_adds_array.h"
#include "city_array.h"
#include "city_defense_booster_register.h"
#include "combat_mods.h"
#include "effect_ctx.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_static_data.h"
#include "unit_static_key.h" 

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

const RuntimeStatics* CombatMng::m_st = nullptr;
bool CombatMng::m_ready = false;
u16 CombatMng::m_pred = 85u;
u16 CombatMng::m_dmg_spread = 25u;
u32 CombatMng::m_seed = 1u;

//================================================================================================================================
//=> - CombatMng -
//================================================================================================================================

void CombatMng::set_dials (u16 pred, u16 dmg_spread, u32 seed) {
    m_pred = (pred > 100u) ? 100u : pred;
    m_dmg_spread = (dmg_spread > 100u) ? 100u : dmg_spread;
    m_seed = (seed == 0u) ? 1u : seed;
}

bool CombatMng::setup (const RuntimeStatics& st) {
    if (!TileAttrTables::ready()) {
        m_ready = false;
        m_st = nullptr;
        return false;
    }
    m_st = &st;
    m_ready = true;
    return true;
}

void CombatMng::clear () {
    m_st = nullptr;
    m_ready = false;
}

bool CombatMng::ready () {
    return m_ready && m_st != nullptr;
}

u16 CombatMng::rnd () {
    m_seed = m_seed * 1664525u + 1013904223u;
    return static_cast<u16>((m_seed >> 16) & 0xFFFFu);
}

u16 CombatMng::lvl_pct (u8 level) {
    switch (level) {
        case VERY_GREEN: return 75u;
        case GREEN: return 100u;
        case REGULAR: return 125u;
        case DISCIPLINED: return 150u;
        case HARDENED: return 175u;
        case VETERAN: return 200u;
        case COMMANDO: return 225u;
        case ELITE: return 250u;
        default: return 100u;
    }
}

u16 CombatMng::atk_mod (const GameState& st, u16 x, u16 y) {
    const GameArraySimple& map = st.m_map;
    if (x >= map.width() || y >= map.height()) {
        return 0u;
    }
    const GameTileSimple* t = map.tile(x, y);
    u16 m = TileAttrTables::terr(static_cast<u8>(t->m_terr)).attack_mod;
    m = static_cast<u16>(m + TileAttrTables::clim(static_cast<u8>(t->m_clim)).attack_mod);
    m = static_cast<u16>(m + TileAttrTables::ov(static_cast<u8>(t->m_ov)).attack_mod);
    m = static_cast<u16>(m + TileAttrTables::road(static_cast<u8>(t->m_road_typ)).attack_mod);
    if (t->m_riv != 0u) {
        m = static_cast<u16>(m + TileAttrTables::riv().attack_mod);
    }
    return m;
}

u16 CombatMng::def_mod (const GameState& st, u16 x, u16 y) {
    const GameArraySimple& map = st.m_map;
    if (x >= map.width() || y >= map.height()) {
        return 0u;
    }
    const GameTileSimple* t = map.tile(x, y);
    u16 m = TileAttrTables::terr(static_cast<u8>(t->m_terr)).defense_mod;
    m = static_cast<u16>(m + TileAttrTables::clim(static_cast<u8>(t->m_clim)).defense_mod);
    m = static_cast<u16>(m + TileAttrTables::ov(static_cast<u8>(t->m_ov)).defense_mod);
    m = static_cast<u16>(m + TileAttrTables::road(static_cast<u8>(t->m_road_typ)).defense_mod);
    if (t->m_riv != 0u) {
        m = static_cast<u16>(m + TileAttrTables::riv().defense_mod);
    }
    return m;
}

i16 CombatMng::city_def_pct (const GameState& st, u16 x, u16 y) {
    const GameArraySimple& map = st.m_map;
    if (x >= map.width() || y >= map.height()) {
        return 0;
    }
    const GameTileSimple* t = map.tile(x, y);
    if (t->m_add_typ != BUILD_ADD_CITY) {
        return 0;
    }
    const u16 city_idx = static_cast<u16>(t->m_add_idx);
    EffectCtx ctx = {};
    ctx.m_owner = static_cast<u16>(t->m_civ_owner);
    ctx.m_city_idx = city_idx;
    if (st.m_player_states != nullptr && ctx.m_owner < st.m_player_n) {
        ctx.m_tech = st.m_player_states[ctx.m_owner].m_techs_researched;
        ctx.m_small_wonder_city = st.m_player_states[ctx.m_owner].m_small_wonder_city;
    }
    ctx.m_bld_bank = st.m_cities.get_bld_bank();
    ctx.m_wonder_city = st.m_wonder_city;
    ctx.m_wonder_n = st.m_wonder_count;
    ctx.m_small_wonder_n = st.m_small_wonder_count;
    return CityDefenseBoosterRegister::determine_effect(ctx).m_perc;
}

u32 CombatMng::pwr (u16 base, i32 pct_mod, u8 level, u8 health) {
    if (base == 0u || health == 0u) {
        return 0u;
    }
    i32 scale = 100 + pct_mod;
    if (scale < 0) {
        scale = 0;
    }
    const u64 p = static_cast<u64>(base)
        * static_cast<u64>(scale)
        * static_cast<u64>(lvl_pct(level))
        * static_cast<u64>(health)
        / 10000ull;
    return static_cast<u32>(p);
}

void CombatMng::resolve (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y) {
    if (!ready()) {
        return;
    }
    const UnitStaticData& ud = m_st->unit();
    const u16 n = ud.get_item_count();
    if (atk.m_unit_typ_idx >= n || def.m_unit_typ_idx >= n) {
        return;
    }
    const UnitStaticDataStruct& as = ud.get_item(UnitStaticDataKey::from_raw(atk.m_unit_typ_idx));
    const UnitStaticDataStruct& ds = ud.get_item(UnitStaticDataKey::from_raw(def.m_unit_typ_idx));
    const GameArraySimple& map = st.m_map;
    bool on_city = false;
    if (x < map.width() && y < map.height()) {
        on_city = (map.tile(x, y)->m_add_typ == BUILD_ADD_CITY);
    }
    const CombatBoost roles = st.m_combat_mods.get(as.role, ds.role, on_city);
    const i32 atk_pct = static_cast<i32>(atk_mod(st, x, y)) + static_cast<i32>(roles.m_atk);
    const i32 def_pct = static_cast<i32>(def_mod(st, x, y)) + static_cast<i32>(roles.m_def) + static_cast<i32>(city_def_pct(st, x, y));
    const u32 ap = pwr(as.attack, atk_pct, atk.m_level, atk.m_health);
    const u32 dp = pwr(ds.defense, def_pct, def.m_level, def.m_health);
    bool atk_wins = false;
    if (ap == 0u && dp == 0u) {
        atk_wins = (rnd() & 1u) != 0u;
    } else if (ap == 0u) {
        atk_wins = false;
    } else if (dp == 0u) {
        atk_wins = true;
    } else {
        const double g = 1.0 + static_cast<double>(m_pred) / 12.0; // This is good for tweaking win odds
        const double ag = std::pow(static_cast<double>(ap), g);
        const double dg = std::pow(static_cast<double>(dp), g);
        u32 thr = static_cast<u32>(10000.0 * ag / (ag + dg) + 0.5);
        if (thr > 10000u) {
            thr = 10000u;
        }
        atk_wins = (static_cast<u32>(rnd()) % 10000u) < thr;
    }
    const u32 win_p = atk_wins ? ap : dp;
    const u32 lose_p = atk_wins ? dp : ap;
    const u8 win_hp = atk_wins ? atk.m_health : def.m_health;
    u32 base_dmg = win_hp;
    if (win_p + lose_p > 0u) {
        base_dmg = static_cast<u32>(win_hp) * lose_p / (win_p + lose_p);
    }
    if (m_dmg_spread > 0u && base_dmg > 0u) {
        const u32 span = base_dmg * static_cast<u32>(m_dmg_spread) / 100u;
        if (span > 0u) {
            const u32 roll = static_cast<u32>(rnd()) % (span * 2u + 1u);
            const i32 delta = static_cast<i32>(roll) - static_cast<i32>(span);
            const i32 adj = static_cast<i32>(base_dmg) + delta;
            base_dmg = (adj < 0) ? 0u : static_cast<u32>(adj);
        }
    }
    if (base_dmg > win_hp) {
        base_dmg = win_hp;
    }
    if (atk_wins) {
        def.m_health = 0u;
        atk.m_health = static_cast<u8>(static_cast<u32>(atk.m_health) - base_dmg);
    } else {
        atk.m_health = 0u;
        def.m_health = static_cast<u8>(static_cast<u32>(def.m_health) - base_dmg);
    }
}

u16 CombatMng::resolve_win_prob (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y) {
    u16 wins = 0u;
    for (u16 i = 0; i < k_prob_n; ++i) {
        UnitAddStruct a = atk;
        UnitAddStruct d = def;
        resolve(a, d, st, x, y);
        if (d.m_health == 0u && a.m_health > 0u) {
            wins++;
        }
    }
    return wins;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
