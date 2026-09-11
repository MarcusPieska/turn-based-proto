//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "mock_muster_siege.h"

#include <chrono>
#include <cstring>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_defense_booster_register.h"
#include "combat_mng.h"
#include "effect_ctx.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_action_enum.h"
#include "unit_add_vector.h"
#include "unit_group_management.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//=> - MockMusterSiege -
//================================================================================================================================

MockMusterSiege::MockMusterSiege () :
    m_army_n(0u),
    m_seat(U16_KEY_NULL),
    m_ok(false) {
}

u16 MockMusterSiege::army_n () const {
    return m_army_n;
}

u16 MockMusterSiege::army_live () const {
    u16 n = 0u;
    for (u16 i = 0; i < m_army_n; ++i) {
        if (m_army[i].m_health > 0u) {
            ++n;
        }
    }
    return n;
}

u16 MockMusterSiege::seat () const {
    return m_seat;
}

bool MockMusterSiege::ok () const {
    return m_ok;
}

void MockMusterSiege::prune_dead () {
    u16 w = 0u;
    for (u16 i = 0; i < m_army_n; ++i) {
        if (m_army[i].m_health > 0u) {
            m_army[w++] = m_army[i];
        }
    }
    m_army_n = w;
    m_ok = m_army_n > 0u;
}

bool MockMusterSiege::collect (GameState& s, u16 seat) {
    m_ok = false;
    m_army_n = 0u;
    m_seat = seat;
    if (s.m_statics == nullptr || s.m_player_states == nullptr || seat >= s.m_player_n) {
        return false;
    }
    UnitAddKey pool[k_cap];
    u16 pool_n = 0u;
    const u16 cn = s.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        UnitAddKey dep[64];
        u16 dn = 0u;
        if (!UnitGroupManagement::muster_collect_depart(s, c->get_x(), c->get_y(), seat, dep, 64u, &dn)) {
            continue;
        }
        for (u16 k = 0; k < dn; ++k) {
            UnitAddKey chain[64];
            u16 cn2 = 0u;
            UnitAddKey cur = dep[k];
            while (cur.is_valid() && cn2 < 64u) {
                chain[cn2++] = cur;
                const UnitAddStruct* u = s.m_units.get_unit_add(cur);
                if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
                    break;
                }
                cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
            }
            for (u16 j = 0; j < cn2; ++j) {
                if (pool_n >= k_cap) {
                    return false;
                }
                pool[pool_n++] = chain[j];
            }
        }
    }
    UnitAddKey army_keys[k_cap];
    u16 an = 0u;
    if (!UnitGroupManagement::campaign_collect_depart(s, pool, pool_n, army_keys, k_cap, &an)) {
        return false;
    }
    for (u16 i = 0; i < an; ++i) {
        const UnitAddStruct* u = s.m_units.get_unit_add(army_keys[i]);
        if (u == nullptr) {
            return false;
        }
        if (m_army_n >= k_cap) {
            return false;
        }
        m_army[m_army_n] = *u;
        m_army[m_army_n].m_next_unit_on_tile = U16_KEY_NULL;
        m_army[m_army_n].m_next_unit_in_group = U16_KEY_NULL;
        if (m_army[m_army_n].m_x == U16_KEY_NULL) {
            m_army[m_army_n].m_x = 0u;
            m_army[m_army_n].m_y = 0u;
        }
        ++m_army_n;
    }
    m_ok = m_army_n > 0u;
    return m_ok;
}

bool MockMusterSiege::copy_tile_foes (
    GameState& s,
    u16 x,
    u16 y,
    u16 atk_seat,
    UnitAddStruct* dst,
    u16 dst_cap,
    u16* out_n) {
    if (dst == nullptr || out_n == nullptr) {
        return false;
    }
    *out_n = 0u;
    u16 cur = s.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 next_tile = u->m_next_unit_on_tile;
        if (u->m_player_idx != atk_seat) {
            UnitAddKey g = UnitAddKey::from_raw(cur);
            while (g.is_valid()) {
                const UnitAddStruct* gu = s.m_units.get_unit_add(g);
                if (gu == nullptr) {
                    break;
                }
                if (*out_n >= dst_cap) {
                    return false;
                }
                dst[*out_n] = *gu;
                dst[*out_n].m_next_unit_on_tile = U16_KEY_NULL;
                dst[*out_n].m_next_unit_in_group = U16_KEY_NULL;
                if (dst[*out_n].m_x == U16_KEY_NULL) {
                    dst[*out_n].m_x = x;
                    dst[*out_n].m_y = y;
                }
                *out_n = static_cast<u16>(*out_n + 1u);
                if (gu->m_next_unit_in_group == U16_KEY_NULL) {
                    break;
                }
                g = UnitAddKey::from_raw(gu->m_next_unit_in_group);
            }
        }
        cur = next_tile;
    }
    return true;
}

void MockMusterSiege::refill_mp (GameState& s, UnitAddStruct* u, u16 n) {
    if (u == nullptr || s.m_statics == nullptr) {
        return;
    }
    const u16 turn = s.m_statics->config().get_mov_pt_per_turn();
    for (u16 i = 0; i < n; ++i) {
        u[i].m_mvt_points = static_cast<i16>(turn);
    }
}

bool MockMusterSiege::can_barrage (GameState& s, u16 typ) const {
    if (s.m_statics == nullptr) {
        return false;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (typ >= n) {
        return false;
    }
    const u16 t = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ)).type;
    return s.m_statics->unit_type_action_map().unit_type_can_do(t, static_cast<u16>(UnitAction::canBarrage));
}

bool MockMusterSiege::can_attack (GameState& s, u16 typ) const {
    if (s.m_statics == nullptr) {
        return false;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (typ >= n) {
        return false;
    }
    const u16 t = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ)).type;
    return s.m_statics->unit_type_action_map().unit_type_can_do(t, static_cast<u16>(UnitAction::canAttack));
}

u16 MockMusterSiege::atk_stat (GameState& s, u16 typ) const {
    if (s.m_statics == nullptr) {
        return 0u;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (typ >= n) {
        return 0u;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ)).attack;
}

u16 MockMusterSiege::def_stat (GameState& s, u16 typ) const {
    if (s.m_statics == nullptr) {
        return 0u;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (typ >= n) {
        return 0u;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ)).defense;
}

bool MockMusterSiege::mock_barrage (
    GameState& s,
    UnitAddStruct* atk,
    u16 an,
    UnitAddStruct* def,
    u16 dn,
    u16 cx,
    u16 cy,
    u16* ded_io) {
    if (atk == nullptr || def == nullptr || ded_io == nullptr || !CombatMng::ready() || s.m_statics == nullptr) {
        return false;
    }
    const u16 turn = s.m_statics->config().get_mov_pt_per_turn();
    if (turn == 0u) {
        return false;
    }
    u16 boost = 0u;
    if (s.m_map.get_add_typ(cx, cy) == BUILD_ADD_CITY) {
        const u16 city_idx = s.m_map.get_add_idx(cx, cy);
        City* city = s.m_cities.get_city(city_idx);
        if (city != nullptr) {
            EffectCtx ctx = {};
            ctx.m_owner = static_cast<u16>(s.m_map.get_civ_owner(cx, cy));
            ctx.m_city_idx = city_idx;
            if (s.m_player_states != nullptr && ctx.m_owner < s.m_player_n) {
                ctx.m_tech = s.m_player_states[ctx.m_owner].m_techs_researched;
                ctx.m_small_wonder_city = s.m_player_states[ctx.m_owner].m_small_wonder_city;
            }
            ctx.m_bld_bank = s.m_cities.get_bld_bank();
            ctx.m_wonder_city = s.m_wonder_city;
            ctx.m_wonder_n = s.m_wonder_count;
            ctx.m_small_wonder_n = s.m_small_wonder_count;
            const i16 boost_i = CityDefenseBoosterRegister::determine_effect(ctx).m_perc;
            if (boost_i > 0) {
                boost = static_cast<u16>(boost_i);
            }
        }
    }
    for (u16 i = 0; i < an; ++i) {
        if (atk[i].m_health == 0u || atk[i].m_mvt_points < static_cast<i16>(turn)) {
            continue;
        }
        if (!can_barrage(s, atk[i].m_unit_typ_idx)) {
            continue;
        }
        u16 ded = *ded_io;
        if (ded > boost) {
            ded = boost;
        }
        if (ded < boost) {
            const u16 chip_raw = atk_stat(s, atk[i].m_unit_typ_idx);
            u16 chip = chip_raw;
            const u16 room = static_cast<u16>(boost - ded);
            if (chip > room) {
                chip = room;
            }
            *ded_io = static_cast<u16>(ded + chip);
            atk[i].m_mvt_points = static_cast<i16>(atk[i].m_mvt_points - static_cast<i16>(turn));
            continue;
        }
        for (u16 d = 0; d < dn; ++d) {
            if (def[d].m_health == 0u) {
                continue;
            }
            const u8 hp0 = def[d].m_health;
            UnitAddStruct a = atk[i];
            UnitAddStruct dv = def[d];
            CombatMng::resolve_attack(a, dv, s, cx, cy);
            const u32 lost = static_cast<u32>(hp0) - static_cast<u32>(dv.m_health);
            u32 dmg = lost * 20u / 100u;
            if (dmg > def[d].m_health) {
                dmg = def[d].m_health;
            }
            def[d].m_health = static_cast<u8>(static_cast<u32>(def[d].m_health) - dmg);
        }
        atk[i].m_mvt_points = static_cast<i16>(atk[i].m_mvt_points - static_cast<i16>(turn));
    }
    return true;
}

bool MockMusterSiege::mock_melee (
    GameState& s,
    UnitAddStruct* atk,
    u16 an,
    UnitAddStruct* def,
    u16 dn,
    u16 cx,
    u16 cy,
    bool* out_taken) {
    if (atk == nullptr || def == nullptr || out_taken == nullptr || !CombatMng::ready() || s.m_statics == nullptr) {
        return false;
    }
    *out_taken = false;
    const u16 turn = s.m_statics->config().get_mov_pt_per_turn();
    if (turn == 0u) {
        return false;
    }
    for (;;) {
        i32 di = -1;
        u16 best_d = 0u;
        for (u16 i = 0; i < dn; ++i) {
            if (def[i].m_health == 0u) {
                continue;
            }
            const u16 d = def_stat(s, def[i].m_unit_typ_idx);
            if (di < 0 || d > best_d) {
                di = static_cast<i32>(i);
                best_d = d;
            }
        }
        if (di < 0) {
            *out_taken = true;
            return true;
        }
        i32 ai = -1;
        u16 best_a = 0u;
        for (u16 i = 0; i < an; ++i) {
            if (atk[i].m_health == 0u || atk[i].m_mvt_points < static_cast<i16>(turn)) {
                continue;
            }
            if (!can_attack(s, atk[i].m_unit_typ_idx)) {
                continue;
            }
            const u16 a = atk_stat(s, atk[i].m_unit_typ_idx);
            if (ai < 0 || a > best_a) {
                ai = static_cast<i32>(i);
                best_a = a;
            }
        }
        if (ai < 0) {
            return true;
        }
        CombatMng::resolve_attack(atk[ai], def[di], s, cx, cy);
        atk[ai].m_mvt_points = static_cast<i16>(atk[ai].m_mvt_points - static_cast<i16>(turn));
    }
}

bool MockMusterSiege::siege (GameState& s, u16 city_x, u16 city_y, MockSiegeRslt* out) {
    if (out == nullptr || !m_ok || m_army_n == 0u || !CombatMng::ready()) {
        return false;
    }
    std::memset(out, 0, sizeof(*out));
    UnitAddStruct defs[k_cap];
    u16 dn = 0u;
    if (!copy_tile_foes(s, city_x, city_y, m_seat, defs, k_cap, &dn)) {
        return false;
    }
    out->m_army_n0 = m_army_n;
    out->m_def_n0 = dn;
    u16 ded0 = 0u;
    City* city = nullptr;
    if (s.m_map.get_add_typ(city_x, city_y) == BUILD_ADD_CITY) {
        city = s.m_cities.get_city(s.m_map.get_add_idx(city_x, city_y));
        if (city != nullptr) {
            ded0 = city->get_defense_deduction();
        }
    }
    u16 ded = ded0;
    const auto t0 = std::chrono::steady_clock::now();
    refill_mp(s, m_army, m_army_n);
    if (!mock_barrage(s, m_army, m_army_n, defs, dn, city_x, city_y, &ded)) {
        return false;
    }
    refill_mp(s, m_army, m_army_n);
    if (!mock_barrage(s, m_army, m_army_n, defs, dn, city_x, city_y, &ded)) {
        return false;
    }
    refill_mp(s, m_army, m_army_n);
    bool taken = false;
    if (!mock_melee(s, m_army, m_army_n, defs, dn, city_x, city_y, &taken)) {
        return false;
    }
    const auto t1 = std::chrono::steady_clock::now();
    out->m_us = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    out->m_taken = taken;
    for (u16 i = 0; i < m_army_n; ++i) {
        if (m_army[i].m_health > 0u) {
            ++out->m_army_live;
        }
    }
    for (u16 i = 0; i < dn; ++i) {
        if (defs[i].m_health > 0u) {
            ++out->m_def_live;
        }
    }
    prune_dead();
    (void)city;
    (void)ded0;
    return true;
}

bool MockMusterSiege::campaign (GameState& s, u16 foe_seat, u16* out_taken, u16* out_foe_n, u16* out_turns) {
    if (out_taken == nullptr || out_foe_n == nullptr || out_turns == nullptr
        || !m_ok || m_army_n == 0u || foe_seat >= s.m_player_n) {
        return false;
    }
    *out_taken = 0u;
    *out_foe_n = 0u;
    *out_turns = 0u;
    u16 ox = 0u;
    u16 oy = 0u;
    bool have_o = false;
    const u16 cn = s.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != m_seat) {
            continue;
        }
        ox = c->get_x();
        oy = c->get_y();
        have_o = true;
        break;
    }
    if (!have_o) {
        return false;
    }
    static const u16 k_city_cap = 512u;
    u16 cx[k_city_cap];
    u16 cy[k_city_cap];
    u32 cd[k_city_cap];
    u16 n = 0u;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != foe_seat) {
            continue;
        }
        if (n >= k_city_cap) {
            break;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        const u32 adx = ox > x ? static_cast<u32>(ox - x) : static_cast<u32>(x - ox);
        const u32 ady = oy > y ? static_cast<u32>(oy - y) : static_cast<u32>(y - oy);
        cx[n] = x;
        cy[n] = y;
        cd[n] = adx + ady;
        ++n;
    }
    *out_foe_n = n;
    for (u16 a = 0; a < n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < n; ++b) {
            if (cd[b] < cd[a]) {
                const u32 td = cd[a];
                cd[a] = cd[b];
                cd[b] = td;
                const u16 tx = cx[a];
                cx[a] = cx[b];
                cx[b] = tx;
                const u16 ty = cy[a];
                cy[a] = cy[b];
                cy[b] = ty;
            }
        }
    }
    for (u16 i = 0; i < n; ++i) {
        if (army_live() == 0u) {
            break;
        }
        MockSiegeRslt r = {};
        if (!siege(s, cx[i], cy[i], &r)) {
            return false;
        }
        *out_turns = static_cast<u16>(*out_turns + 1u);
        if (r.m_taken) {
            *out_taken = static_cast<u16>(*out_taken + 1u);
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
