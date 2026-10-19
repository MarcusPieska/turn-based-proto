//================================================================================================================================
//=> - Includes (mk01: winner-kill + city DEF% on dp + pwr^g) -
//================================================================================================================================

#include <cmath>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_defense_booster_register.h"
#include "combat_mods.h"
#include "effect_ctx.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Level-up RNG (xorshift32) -
//================================================================================================================================

static u32 s_xp = 1u;

static void xp_seed (u32 s) {
    s_xp = (s == 0u) ? 1u : s;
}

static u32 xp_next () {
    u32 x = s_xp;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_xp = x;
    return x;
}

static void try_up (const RuntimeStatics* st, UnitAddStruct& u) {
    if (st == nullptr) {
        return;
    }
    const u16 n = st->unit_level().get_item_count();
    if (n <= 1u || u.m_level >= static_cast<u8>(n - 1u)) {
        return;
    }
    const u16 prob = st->unit_level().get_item(
        UnitLevelStaticDataKey::from_raw(static_cast<u16>(u.m_level))).up_prob;
    if (prob != 0u && ((xp_next() >> 16) * 100u) >> 16 < static_cast<u32>(prob)) {
        u.m_level = static_cast<u8>(u.m_level + 1u);
    }
}

//================================================================================================================================
//=> - CombatMng (mk01) -
//================================================================================================================================

void CombatMng::set_dials (u16 pred, u16 dmg_spread, u32 seed) {
    m_pred = (pred > 100u) ? 100u : pred;
    m_dmg_spread = (dmg_spread > 100u) ? 100u : dmg_spread;
    m_seed = (seed == 0u) ? 1u : seed;
    xp_seed(m_seed);
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

i32 CombatMng::atk_mod (const GameState& st, u16 x, u16 y) {
    const GameArraySimple& map = st.m_map;
    if (x >= map.width() || y >= map.height()) {
        return 0;
    }
    const GameTileSimple* t = map.tile(x, y);
    i32 m = static_cast<i32>(TileAttrTables::terr(static_cast<u8>(t->m_terr)).attack_mod);
    m += static_cast<i32>(TileAttrTables::clim(static_cast<u8>(t->m_clim)).attack_mod);
    m += static_cast<i32>(TileAttrTables::ov(static_cast<u8>(t->m_ov)).attack_mod);
    m += static_cast<i32>(TileAttrTables::road(static_cast<u8>(t->m_road_typ)).attack_mod);
    if (t->m_riv != 0u) {
        m += static_cast<i32>(TileAttrTables::riv().attack_mod);
    }
    return m;
}

i32 CombatMng::def_mod (const GameState& st, u16 x, u16 y) {
    const GameArraySimple& map = st.m_map;
    if (x >= map.width() || y >= map.height()) {
        return 0;
    }
    const GameTileSimple* t = map.tile(x, y);
    i32 m = static_cast<i32>(TileAttrTables::terr(static_cast<u8>(t->m_terr)).defense_mod);
    m += static_cast<i32>(TileAttrTables::clim(static_cast<u8>(t->m_clim)).defense_mod);
    m += static_cast<i32>(TileAttrTables::ov(static_cast<u8>(t->m_ov)).defense_mod);
    m += static_cast<i32>(TileAttrTables::road(static_cast<u8>(t->m_road_typ)).defense_mod);
    if (t->m_riv != 0u) {
        m += static_cast<i32>(TileAttrTables::riv().defense_mod);
    }
    return m;
}

i16 CombatMng::city_def_pts (const GameState& st, u16 x, u16 y) {
    const GameArraySimple& map = st.m_map;
    if (x >= map.width() || y >= map.height()) {
        return 0;
    }
    const GameTileSimple* t = map.tile(x, y);
    if (map.get_add_typ(x, y) != BUILD_ADD_CITY) {
        return 0;
    }
    const u16 city_idx = static_cast<u16>(t->m_add_idx);
    EffectCtx ctx = {};
    ctx.m_owner = static_cast<u16>(t->m_civ_owner);
    ctx.m_city_idx = city_idx;
    if (st.m_player_states != nullptr && ctx.m_owner < st.m_player_n) {
        const PlayerState& ps = st.m_player_states[ctx.m_owner];
        ctx.m_tech = ps.m_techs_researched;
        ctx.m_small_wonder_city = ps.m_small_wonder_city;
        if (st.m_statics != nullptr) {
            effect_ctx_bind_civ_traits(ctx, *st.m_statics, ps.m_civ_index);
        }
    }
    ctx.m_bld_bank = st.m_cities.get_bld_bank();
    ctx.m_wonder_city = st.m_wonder_city;
    ctx.m_wonder_n = st.m_wonder_count;
    ctx.m_small_wonder_n = st.m_small_wonder_count;
    i16 boost = CityDefenseBoosterRegister::determine_effect(ctx).m_perc;
    if (boost <= 0) {
        return 0;
    }
    const City* city = st.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return boost;
    }
    const u16 ded = city->get_defense_deduction();
    const i16 eff = static_cast<i16>(boost - static_cast<i16>(ded));
    return (eff < 0) ? 0 : eff;
}

static i32 s_delta (i32 own, i32 other, i32 f) {
    return (own - other) * f;
}

static u32 s_pwr_pts (i32 pts, u8 health) {
    if (health == 0u) {
        return 0u;
    }
    i32 score = 100 + pts;
    if (score < 1) {
        score = 1;
    }
    return static_cast<u32>(score) * static_cast<u32>(health);
}

u32 CombatMng::pwr (i32 pts, u8 health) {
    return s_pwr_pts(pts, health);
}

static void s_fight_pwr (
    const RuntimeStatics& cfg_st,
    const UnitStaticDataStruct& as,
    const UnitStaticDataStruct& ds,
    const UnitAddStruct& atk,
    const UnitAddStruct& def,
    i32 a_terr,
    i32 d_terr,
    i32 a_role,
    i32 d_role,
    i32 d_city,
    u32* out_ap,
    u32* out_dp)
{
    const GameConfigSettings& cfg = cfg_st.config();
    const i32 sf = static_cast<i32>(cfg.get_unit_stat_damage_factor());
    const i32 lf = static_cast<i32>(cfg.get_unit_level_damage_factor());
    const i32 zf = static_cast<i32>(cfg.get_unit_size_damage_factor());
    const i32 rf = static_cast<i32>(cfg.get_unit_role_damage_factor());
    const i32 tf = static_cast<i32>(cfg.get_unit_terrain_damage_factor());
    const i32 cf = static_cast<i32>(cfg.get_unit_city_damage_factor());
    const i32 a_sz = static_cast<i32>(atk.m_unit_size);
    const i32 d_sz = static_cast<i32>(def.m_unit_size);
    i32 a_pts = 0;
    a_pts += s_delta(static_cast<i32>(as.attack), static_cast<i32>(ds.defense), sf);
    a_pts += s_delta(static_cast<i32>(atk.m_level), static_cast<i32>(def.m_level), lf);
    a_pts += s_delta(a_sz, d_sz, zf);
    a_pts += a_terr * tf;
    a_pts += a_role * rf;
    i32 d_pts = 0;
    d_pts += s_delta(static_cast<i32>(ds.defense), static_cast<i32>(as.attack), sf);
    d_pts += s_delta(static_cast<i32>(def.m_level), static_cast<i32>(atk.m_level), lf);
    d_pts += s_delta(d_sz, a_sz, zf);
    d_pts += d_terr * tf;
    d_pts += d_city * cf;
    d_pts += d_role * rf;
    *out_ap = s_pwr_pts(a_pts, atk.m_health);
    *out_dp = s_pwr_pts(d_pts, def.m_health);
}

void CombatMng::resolve_attack (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y) {
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
        on_city = (map.get_add_typ(x, y) == BUILD_ADD_CITY);
    }
    const CombatBoost roles = st.m_combat_mods.get(as.role, ds.role, on_city);
    u32 ap = 0u;
    u32 dp = 0u;
    s_fight_pwr(*m_st, as, ds, atk, def, atk_mod(st, x, y), def_mod(st, x, y),
        static_cast<i32>(roles.m_atk), static_cast<i32>(roles.m_def),
        static_cast<i32>(city_def_pts(st, x, y)), &ap, &dp);
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
        if (atk.m_health > 0u) {
            try_up(m_st, atk);
        }
    } else {
        atk.m_health = 0u;
        def.m_health = static_cast<u8>(static_cast<u32>(def.m_health) - base_dmg);
        if (def.m_health > 0u) {
            try_up(m_st, def);
        }
    }
}

void CombatMng::resolve_atk_city (UnitAddStruct& atk, UnitAddStruct& def, const GameState& st, u16 x, u16 y) {
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
        on_city = (map.get_add_typ(x, y) == BUILD_ADD_CITY);
    }
    const CombatBoost roles = st.m_combat_mods.get(as.role, ds.role, on_city);
    u32 ap = 0u;
    u32 dp = 0u;
    s_fight_pwr(*m_st, as, ds, atk, def, 0, 0,
        static_cast<i32>(roles.m_atk), static_cast<i32>(roles.m_def), 0, &ap, &dp);
    bool atk_wins = false;
    if (ap == 0u && dp == 0u) {
        atk_wins = (rnd() & 1u) != 0u;
    } else if (ap == 0u) {
        atk_wins = false;
    } else if (dp == 0u) {
        atk_wins = true;
    } else {
        const double g = 1.0 + static_cast<double>(m_pred) / 12.0;
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
        if (atk.m_health > 0u) {
            try_up(m_st, atk);
        }
    } else {
        atk.m_health = 0u;
        def.m_health = static_cast<u8>(static_cast<u32>(def.m_health) - base_dmg);
        if (def.m_health > 0u) {
            try_up(m_st, def);
        }
    }
}

u16 CombatMng::resolve_win_prob (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y) {
    u16 wins = 0u;
    for (u16 i = 0; i < k_prob_n; ++i) {
        UnitAddStruct a = atk;
        UnitAddStruct d = def;
        resolve_attack(a, d, st, x, y);
        if (d.m_health == 0u && a.m_health > 0u) {
            wins++;
        }
    }
    return wins;
}

u32 CombatMng::resolve_barrage (UnitAddStruct& atk, GameState& st, u16 x, u16 y) {
    if (!ready()) {
        return 0u;
    }
    u32 tot = 0u;
    bool hit_units = true;
    if (st.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        const u16 city_idx = st.m_map.get_add_idx(x, y);
        City* city = st.m_cities.get_city(city_idx);
        if (city != nullptr) {
            EffectCtx ctx = {};
            ctx.m_owner = static_cast<u16>(st.m_map.get_civ_owner(x, y));
            ctx.m_city_idx = city_idx;
            if (st.m_player_states != nullptr && ctx.m_owner < st.m_player_n) {
                const PlayerState& ps = st.m_player_states[ctx.m_owner];
                ctx.m_tech = ps.m_techs_researched;
                ctx.m_small_wonder_city = ps.m_small_wonder_city;
                if (st.m_statics != nullptr) {
                    effect_ctx_bind_civ_traits(ctx, *st.m_statics, ps.m_civ_index);
                }
            }
            ctx.m_bld_bank = st.m_cities.get_bld_bank();
            ctx.m_wonder_city = st.m_wonder_city;
            ctx.m_wonder_n = st.m_wonder_count;
            ctx.m_small_wonder_n = st.m_small_wonder_count;
            const i16 boost_i = CityDefenseBoosterRegister::determine_effect(ctx).m_perc;
            if (boost_i > 0) {
                const u16 boost = static_cast<u16>(boost_i);
                u16 ded = city->get_defense_deduction();
                if (ded > boost) {
                    ded = boost;
                }
                if (ded < boost) {
                    hit_units = false;
                    const u16 chip_raw = m_st->unit().get_item(UnitStaticDataKey::from_raw(atk.m_unit_typ_idx)).attack;
                    u16 chip = chip_raw;
                    const u16 room = static_cast<u16>(boost - ded);
                    if (chip > room) {
                        chip = room;
                    }
                    city->set_defense_deduction(static_cast<u16>(ded + chip));
                    tot += chip;
                }
            }
        }
    }
    if (!hit_units) {
        return tot;
    }
    const u16 atk_seat = atk.m_player_idx;
    u16 cur = st.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 next_tile = u->m_next_unit_on_tile;
        UnitAddKey g = UnitAddKey::from_raw(cur);
        while (g.is_valid()) {
            UnitAddStruct* gu = st.m_units.get_unit_add(g);
            if (gu == nullptr) {
                break;
            }
            const u16 next_g = gu->m_next_unit_in_group;
            if (gu->m_player_idx != atk_seat && gu->m_health > 0u) {
                const u8 hp0 = gu->m_health;
                UnitAddStruct a = atk;
                UnitAddStruct d = *gu;
                resolve_atk_city(a, d, st, x, y);
                const u32 lost = static_cast<u32>(hp0) - static_cast<u32>(d.m_health);
                u32 dmg = lost * 20u / 100u;
                if (dmg > gu->m_health) {
                    dmg = gu->m_health;
                }
                gu->m_health = static_cast<u8>(static_cast<u32>(gu->m_health) - dmg);
                tot += dmg;
            }
            if (next_g == U16_KEY_NULL) {
                break;
            }
            g = UnitAddKey::from_raw(next_g);
        }
        cur = next_tile;
    }
    return tot;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
