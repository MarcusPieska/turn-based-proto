//================================================================================================================================
//=> - Includes (mk02: noisy odds + 2p/(2(1-p)) split of current HP) -
//================================================================================================================================

#include <cmath>
#include <ctime>

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
//=> - Dials -
//================================================================================================================================

static constexpr double k_noise_sig = 8.0;
static constexpr double k_noise_cap = 30.0;

static constexpr double k_d_base = 0.25;
static constexpr double k_d_terr = 0.25;
static constexpr double k_d_role = 0.25;
static constexpr double k_d_city = 0.25;
static constexpr double k_d_lvl = 0.50;
static constexpr double k_d_hp = 0.25;
static constexpr double k_d_size = 0.45;
static constexpr double k_d_sharp = 1.0 + 85.0 / 12.0;

static double s_mix (double neut, double val, double w) {
    if (w <= 0.0) {
        return neut;
    }
    if (w >= 1.0) {
        return val;
    }
    return neut + w * (val - neut);
}

static u32 s_pwr (u16 base, i32 terr, i32 role, i32 city, u16 lvl_p, u8 health, u8 size) {
    const double b = s_mix(1.0, static_cast<double>(base), k_d_base);
    const double t = s_mix(0.0, static_cast<double>(terr), k_d_terr);
    const double r = s_mix(0.0, static_cast<double>(role), k_d_role);
    const double c = s_mix(0.0, static_cast<double>(city), k_d_city);
    const double lv = s_mix(100.0, static_cast<double>(lvl_p), k_d_lvl);
    const double hp = s_mix(static_cast<double>(UNIT_HEALTH), static_cast<double>(health), k_d_hp);
    const double sz = s_mix(1.0, 1.0 + static_cast<double>(size), k_d_size);
    if (b <= 0.0 || hp <= 0.0) {
        return 0u;
    }
    double scale = 100.0 + t + r + c;
    if (scale < 0.0) {
        scale = 0.0;
    }
    const double p = b * scale * lv * hp * sz / 10000.0;
    return static_cast<u32>(p);
}

//================================================================================================================================
//=> - CombatMng (mk02) -
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
    if (map.get_add_typ(x, y) != BUILD_ADD_CITY) {
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

u32 CombatMng::pwr (u16 base, i32 pct_mod, u8 level, u8 health, u8 size) {
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
        * static_cast<u64>(1u + static_cast<u32>(size))
        / 10000ull;
    return static_cast<u32>(p);
}

void CombatMng::apply_odds (UnitAddStruct& atk, UnitAddStruct& def, u32 ap, u32 dp) {
    double p = odds_raw(ap, dp);
    p += noise_draw() / 100.0;
    if (p < 0.0) {
        p = 0.0;
    } else if (p > 1.0) {
        p = 1.0;
    }
    if (p == 0.5) {
        p = 0.51;
    }
    const double ra = 2.0 * p - 1.0;
    const double rd = 1.0 - 2.0 * p;
    if (ra <= 0.0) {
        atk.m_health = 0u;
    } else {
        atk.m_health = static_cast<u8>(static_cast<double>(atk.m_health) * ra);
    }
    if (rd <= 0.0) {
        def.m_health = 0u;
    } else {
        def.m_health = static_cast<u8>(static_cast<double>(def.m_health) * rd);
    }
}

double CombatMng::noise_sig () {
    return k_noise_sig;
}

double CombatMng::noise_cap () {
    return k_noise_cap;
}

double CombatMng::noise_draw () {
    const double u1 = (static_cast<double>(rnd()) + 1.0) / 65536.0;
    const double u2 = static_cast<double>(rnd()) / 65536.0;
    const double z = std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * 3.14159265358979323846 * u2);
    double n = z * k_noise_sig;
    if (n > k_noise_cap) {
        n = k_noise_cap;
    } else if (n < -k_noise_cap) {
        n = -k_noise_cap;
    }
    return n;
}

void CombatMng::noise_run (u32 n, double* mean, double* sd, double* us) {
    if (mean != nullptr) {
        *mean = 0.0;
    }
    if (sd != nullptr) {
        *sd = 0.0;
    }
    if (us != nullptr) {
        *us = 0.0;
    }
    if (n == 0u) {
        return;
    }
    double sum = 0.0;
    double sum2 = 0.0;
    const clock_t t0 = clock();
    for (u32 i = 0u; i < n; ++i) {
        const double v = noise_draw();
        sum += v;
        sum2 += v * v;
    }
    const clock_t t1 = clock();
    const double m = sum / static_cast<double>(n);
    if (mean != nullptr) {
        *mean = m;
    }
    if (sd != nullptr) {
        const double var = sum2 / static_cast<double>(n) - m * m;
        *sd = (var > 0.0) ? std::sqrt(var) : 0.0;
    }
    if (us != nullptr) {
        *us = 1000000.0 * static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    }
}

double CombatMng::odds_raw (u32 ap, u32 dp) {
    if (ap == 0u && dp == 0u) {
        return 0.5;
    }
    if (ap == 0u) {
        return 0.0;
    }
    if (dp == 0u) {
        return 1.0;
    }
    const double g = k_d_sharp;
    const double ag = std::pow(static_cast<double>(ap), g);
    const double dg = std::pow(static_cast<double>(dp), g);
    return ag / (ag + dg);
}

double CombatMng::odds_split (const UnitAddStruct& atk, const UnitAddStruct& def, const GameState& st, u16 x, u16 y) {
    if (!ready()) {
        return 0.0;
    }
    const UnitStaticData& ud = m_st->unit();
    const u16 n = ud.get_item_count();
    if (atk.m_unit_typ_idx >= n || def.m_unit_typ_idx >= n) {
        return 0.0;
    }
    const UnitStaticDataStruct& as = ud.get_item(UnitStaticDataKey::from_raw(atk.m_unit_typ_idx));
    const UnitStaticDataStruct& ds = ud.get_item(UnitStaticDataKey::from_raw(def.m_unit_typ_idx));
    const GameArraySimple& map = st.m_map;
    bool on_city = false;
    if (x < map.width() && y < map.height()) {
        on_city = (map.get_add_typ(x, y) == BUILD_ADD_CITY);
    }
    const CombatBoost roles = st.m_combat_mods.get(as.role, ds.role, on_city);
    const i32 a_terr = static_cast<i32>(atk_mod(st, x, y));
    const i32 d_terr = static_cast<i32>(def_mod(st, x, y));
    const i32 a_role = static_cast<i32>(roles.m_atk);
    const i32 d_role = static_cast<i32>(roles.m_def);
    const i32 d_city = static_cast<i32>(city_def_pct(st, x, y));
    const u32 ap = s_pwr(as.attack, a_terr, a_role, 0, lvl_pct(atk.m_level), atk.m_health, static_cast<u8>(atk.m_unit_size));
    const u32 dp = s_pwr(ds.defense, d_terr, d_role, d_city, lvl_pct(def.m_level), def.m_health, static_cast<u8>(def.m_unit_size));
    return odds_raw(ap, dp);
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
    const i32 a_terr = static_cast<i32>(atk_mod(st, x, y));
    const i32 d_terr = static_cast<i32>(def_mod(st, x, y));
    const i32 a_role = static_cast<i32>(roles.m_atk);
    const i32 d_role = static_cast<i32>(roles.m_def);
    const i32 d_city = static_cast<i32>(city_def_pct(st, x, y));
    const u32 ap = s_pwr(as.attack, a_terr, a_role, 0, lvl_pct(atk.m_level), atk.m_health, static_cast<u8>(atk.m_unit_size));
    const u32 dp = s_pwr(ds.defense, d_terr, d_role, d_city, lvl_pct(def.m_level), def.m_health, static_cast<u8>(def.m_unit_size));
    apply_odds(atk, def, ap, dp);
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
    const i32 a_role = static_cast<i32>(roles.m_atk);
    const i32 d_role = static_cast<i32>(roles.m_def);
    const u32 ap = s_pwr(as.attack, 0, a_role, 0, lvl_pct(atk.m_level), atk.m_health, static_cast<u8>(atk.m_unit_size));
    const u32 dp = s_pwr(ds.defense, 0, d_role, 0, lvl_pct(def.m_level), def.m_health, static_cast<u8>(def.m_unit_size));
    apply_odds(atk, def, ap, dp);
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
                ctx.m_tech = st.m_player_states[ctx.m_owner].m_techs_researched;
                ctx.m_small_wonder_city = st.m_player_states[ctx.m_owner].m_small_wonder_city;
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
