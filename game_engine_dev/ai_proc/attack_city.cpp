//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "attack_city.h"

#include "combat_mng.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 mp_turn (const GameState& s) {
    if (s.m_statics == nullptr) {
        return 0u;
    }
    return s.m_statics->config().get_mov_pt_per_turn();
}

static u16 utype (const GameState& s, u16 unit_typ_idx) {
    if (s.m_statics == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (unit_typ_idx >= n) {
        return U16_KEY_NULL;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_typ_idx)).type;
}

static bool is_offensive (const GameState& s, u16 unit_typ_idx) {
    const u16 t = utype(s, unit_typ_idx);
    return t != U16_KEY_NULL
        && (t == s.m_land_attack_type_idx || t == s.m_land_artillery_type_idx);
}

static u16 atk_stat (const GameState& s, u16 unit_typ_idx) {
    if (s.m_statics == nullptr) {
        return 0;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (unit_typ_idx >= n) {
        return 0;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_typ_idx)).attack;
}

static u16 def_stat (const GameState& s, u16 unit_typ_idx) {
    if (s.m_statics == nullptr) {
        return 0;
    }
    const u16 n = s.m_statics->unit().get_item_count();
    if (unit_typ_idx >= n) {
        return 0;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(unit_typ_idx)).defense;
}

static bool pick_attacker (GameState& s, UnitAddKey head, UnitAddKey* out) {
    if (out == nullptr || !head.is_valid()) {
        return false;
    }
    UnitAddKey best = UnitAddKey::None();
    u16 best_atk = 0;
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u
            && u->m_mvt_points >= static_cast<i16>(mp_turn(s))
            && is_offensive(s, u->m_unit_typ_idx)) {
            const u16 a = atk_stat(s, u->m_unit_typ_idx);
            if (!best.is_valid() || a > best_atk) {
                best = cur;
                best_atk = a;
            }
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    if (!best.is_valid()) {
        return false;
    }
    *out = best;
    return true;
}

static bool pick_defender (GameState& s, u16 x, u16 y, u16 atk_seat, UnitAddKey* out) {
    if (out == nullptr) {
        return false;
    }
    UnitAddKey best = UnitAddKey::None();
    u16 best_def = 0;
    u16 cur = s.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddKey k = UnitAddKey::from_raw(cur);
        UnitAddStruct* u = s.m_units.get_unit_add(k);
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx != atk_seat && u->m_health > 0u) {
            UnitAddKey gcur = k;
            while (gcur.is_valid()) {
                UnitAddStruct* gu = s.m_units.get_unit_add(gcur);
                if (gu == nullptr) {
                    break;
                }
                if (gu->m_health > 0u && gu->m_player_idx != atk_seat) {
                    const u16 d = def_stat(s, gu->m_unit_typ_idx);
                    if (!best.is_valid() || d > best_def) {
                        best = gcur;
                        best_def = d;
                    }
                }
                if (gu->m_next_unit_in_group == U16_KEY_NULL) {
                    break;
                }
                gcur = UnitAddKey::from_raw(gu->m_next_unit_in_group);
            }
        }
        cur = u->m_next_unit_on_tile;
    }
    if (!best.is_valid()) {
        return false;
    }
    *out = best;
    return true;
}

//================================================================================================================================
//=> - AttackCity -
//================================================================================================================================

bool AttackCity::assault (
    GameState& s,
    UnitAddKey army_hd,
    u16 city_x,
    u16 city_y,
    UnitAddKey* out_stay,
    UnitAddKey* out_occupy) 
{
    if (!CombatMng::ready() || s.m_statics == nullptr || !army_hd.is_valid()) {
        return false;
    }
    const u16 turn_mp = mp_turn(s);
    if (turn_mp == 0u) {
        return false;
    }
    UnitAddKey head = army_hd;
    UnitAddStruct* hu = s.m_units.get_unit_add(head);
    if (hu == nullptr || hu->m_x == U16_KEY_NULL) {
        return false;
    }
    const u8 atk_seat = hu->m_player_idx;
    {
        const u16 typ_n = s.m_statics->unit().get_item_count();
        UnitAddKey cur = head;
        while (cur.is_valid()) {
            UnitAddStruct* u = s.m_units.get_unit_add(cur);
            if (u == nullptr) {
                break;
            }
            if (u->m_unit_typ_idx < typ_n) {
                const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
                u->m_mvt_points = static_cast<i16>(pts * turn_mp);
            }
            if (u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
    }
    UnitAddKey last_atk = UnitAddKey::None();
    for (;;) {
        UnitAddKey def_k = UnitAddKey::None();
        if (!pick_defender(s, city_x, city_y, atk_seat, &def_k)) {
            break;
        }
        UnitAddKey atk_k = UnitAddKey::None();
        if (!pick_attacker(s, head, &atk_k)) {
            return false;
        }
        UnitAddStruct* au = s.m_units.get_unit_add(atk_k);
        UnitAddStruct* du = s.m_units.get_unit_add(def_k);
        if (au == nullptr || du == nullptr) {
            return false;
        }
        CombatMng::resolve(*au, *du, s, city_x, city_y);
        au->m_mvt_points = static_cast<i16>(au->m_mvt_points - static_cast<i16>(turn_mp));
        last_atk = atk_k;
        if (au->m_health == 0u) {
            const bool was_head = (atk_k == head);
            UnitAddKey nxt = UnitAddKey::None();
            if (was_head && au->m_next_unit_in_group != U16_KEY_NULL) {
                nxt = UnitAddKey::from_raw(au->m_next_unit_in_group);
            }
            if (!UnitMovementMng::destroy_unit(s, atk_k)) {
                return false;
            }
            last_atk = UnitAddKey::None();
            if (was_head) {
                head = nxt;
                if (!head.is_valid()) {
                    return false;
                }
            }
        }
        if (du->m_health == 0u) {
            if (!UnitMovementMng::destroy_unit(s, def_k)) {
                return false;
            }
        }
        hu = s.m_units.get_unit_add(head);
        if (hu == nullptr || hu->m_x == U16_KEY_NULL) {
            return false;
        }
    }
    UnitAddKey still = UnitAddKey::None();
    if (pick_defender(s, city_x, city_y, atk_seat, &still)) {
        return false;
    }
    UnitAddKey force = last_atk.is_valid() ? last_atk : head;
    UnitAddKey stay = UnitAddKey::None();
    UnitAddKey go = UnitAddKey::None();
    if (!UnitMovementMng::split_group_half_by_type(s, head, force, &stay, &go)) {
        return false;
    }
    {
        const u16 typ_n = s.m_statics->unit().get_item_count();
        UnitAddKey cur = go;
        while (cur.is_valid()) {
            UnitAddStruct* u = s.m_units.get_unit_add(cur);
            if (u == nullptr) {
                break;
            }
            if (u->m_unit_typ_idx < typ_n) {
                const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
                u->m_mvt_points = static_cast<i16>(pts * turn_mp);
            }
            if (u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
    }
    if (!UnitMovementMng::can_step(s, go, city_x, city_y, nullptr)
        || !UnitMovementMng::apply_step(s, go, city_x, city_y)) {
        return false;
    }
    if (out_stay != nullptr) {
        *out_stay = stay;
    }
    if (out_occupy != nullptr) {
        *out_occupy = go;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
