//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_attack_manager.h"

#include "combat_mng.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_key_cap = 2048u;

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

static bool typ_can (const GameState& s, u16 unit_typ_idx, UnitAction act) {
    const u16 t = utype(s, unit_typ_idx);
    if (t == U16_KEY_NULL) {
        return false;
    }
    return s.m_statics->unit_type_action_map().unit_type_can_do(t, static_cast<u16>(act));
}

static bool can_barrage (const GameState& s, u16 unit_typ_idx) {
    return typ_can(s, unit_typ_idx, UnitAction::canBarrage);
}

static bool can_attack (const GameState& s, u16 unit_typ_idx) {
    return typ_can(s, unit_typ_idx, UnitAction::canAttack);
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
            && can_attack(s, u->m_unit_typ_idx)) {
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

static bool has_melee_cont (const GameState& s, UnitAddKey head) {
    const u16 turn = mp_turn(s);
    if (turn == 0u || s.m_statics == nullptr) {
        return false;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u && can_attack(s, u->m_unit_typ_idx) && u->m_unit_typ_idx < typ_n) {
            const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
            if (pts > 0u) {
                return true;
            }
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return false;
}

static bool purge_dead_foes (GameState& s, u16 x, u16 y, u16 atk_seat) {
    UnitAddKey dead[k_key_cap];
    u16 n = 0;
    u16 cur = s.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL && n < k_key_cap) {
        UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 next_tile = u->m_next_unit_on_tile;
        UnitAddKey g = UnitAddKey::from_raw(cur);
        while (g.is_valid() && n < k_key_cap) {
            UnitAddStruct* gu = s.m_units.get_unit_add(g);
            if (gu == nullptr) {
                break;
            }
            const u16 next_g = gu->m_next_unit_in_group;
            if (gu->m_player_idx != atk_seat && gu->m_health == 0u) {
                dead[n++] = g;
            }
            if (next_g == U16_KEY_NULL) {
                break;
            }
            g = UnitAddKey::from_raw(next_g);
        }
        cur = next_tile;
    }
    for (u16 i = 0; i < n; ++i) {
        if (!UnitMovementMng::destroy_unit(s, dead[i])) {
            return false;
        }
    }
    return true;
}

static void refill_grp_mp (GameState& s, UnitAddKey head, u16 turn_mp) {
    if (s.m_statics == nullptr) {
        return;
    }
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

//================================================================================================================================
//=> - CityAttackManager -
//================================================================================================================================

void CityAttackManager::refill_mp (GameState& s, UnitAddKey army_hd) {
    const u16 turn_mp = mp_turn(s);
    if (turn_mp == 0u || !army_hd.is_valid()) {
        return;
    }
    refill_grp_mp(s, army_hd, turn_mp);
}

bool CityAttackManager::barrage (
    GameState& s,
    UnitAddKey army_hd,
    u16 city_x,
    u16 city_y,
    CityBarrage* out_dmg)
{
    if (out_dmg != nullptr) {
        out_dmg->m_tot = 0u;
        out_dmg->m_last = 0u;
    }
    if (!CombatMng::ready() || s.m_statics == nullptr || !army_hd.is_valid()) {
        return false;
    }
    const u16 turn_mp = mp_turn(s);
    if (turn_mp == 0u) {
        return false;
    }
    UnitAddStruct* hu = s.m_units.get_unit_add(army_hd);
    if (hu == nullptr || hu->m_x == U16_KEY_NULL) {
        return false;
    }
    const u8 atk_seat = hu->m_player_idx;
    UnitAddKey keys[k_key_cap];
    u16 kn = 0;
    UnitAddKey cur = army_hd;
    while (cur.is_valid() && kn < k_key_cap) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u
            && u->m_mvt_points >= static_cast<i16>(turn_mp)
            && can_barrage(s, u->m_unit_typ_idx)) {
            keys[kn++] = cur;
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    u32 tot = 0u;
    u32 last = 0u;
    for (u16 i = 0; i < kn; ++i) {
        UnitAddStruct* u = s.m_units.get_unit_add(keys[i]);
        if (u == nullptr || u->m_health == 0u) {
            continue;
        }
        last = CombatMng::resolve_barrage(*u, s, city_x, city_y);
        tot += last;
        u->m_mvt_points = static_cast<i16>(u->m_mvt_points - static_cast<i16>(turn_mp));
    }
    if (!purge_dead_foes(s, city_x, city_y, atk_seat)) {
        return false;
    }
    if (out_dmg != nullptr) {
        out_dmg->m_tot = tot;
        out_dmg->m_last = last;
    }
    return true;
}

CityAssault CityAttackManager::melee (
    GameState& s,
    UnitAddKey* army_hd,
    u16 city_x,
    u16 city_y,
    UnitAddKey* out_stay,
    UnitAddKey* out_occupy)
{
    if (out_stay != nullptr) {
        *out_stay = UnitAddKey::None();
    }
    if (out_occupy != nullptr) {
        *out_occupy = UnitAddKey::None();
    }
    if (army_hd == nullptr || !CombatMng::ready() || s.m_statics == nullptr || !army_hd->is_valid()) {
        return CityAssault::Fail;
    }
    const u16 turn_mp = mp_turn(s);
    if (turn_mp == 0u) {
        return CityAssault::Fail;
    }
    UnitAddKey head = *army_hd;
    UnitAddStruct* hu = s.m_units.get_unit_add(head);
    if (hu == nullptr || hu->m_x == U16_KEY_NULL) {
        return CityAssault::Fail;
    }
    const u8 atk_seat = hu->m_player_idx;
    UnitAddKey last_atk = UnitAddKey::None();
    for (;;) {
        UnitAddKey def_k = UnitAddKey::None();
        if (!pick_defender(s, city_x, city_y, atk_seat, &def_k)) {
            break;
        }
        UnitAddKey atk_k = UnitAddKey::None();
        if (!pick_attacker(s, head, &atk_k)) {
            *army_hd = head;
            if (out_stay != nullptr) {
                *out_stay = head;
            }
            return has_melee_cont(s, head) ? CityAssault::Stall : CityAssault::Fail;
        }
        UnitAddStruct* au = s.m_units.get_unit_add(atk_k);
        UnitAddStruct* du = s.m_units.get_unit_add(def_k);
        if (au == nullptr || du == nullptr) {
            return CityAssault::Fail;
        }
        CombatMng::resolve_attack(*au, *du, s, city_x, city_y);
        au->m_mvt_points = static_cast<i16>(au->m_mvt_points - static_cast<i16>(turn_mp));
        last_atk = atk_k;
        if (au->m_health == 0u) {
            const bool was_head = (atk_k == head);
            UnitAddKey nxt = UnitAddKey::None();
            if (was_head && au->m_next_unit_in_group != U16_KEY_NULL) {
                nxt = UnitAddKey::from_raw(au->m_next_unit_in_group);
            }
            if (!UnitMovementMng::destroy_unit(s, atk_k)) {
                return CityAssault::Fail;
            }
            last_atk = UnitAddKey::None();
            if (was_head) {
                head = nxt;
                if (!head.is_valid()) {
                    return CityAssault::Fail;
                }
            }
        }
        if (du->m_health == 0u) {
            if (!UnitMovementMng::destroy_unit(s, def_k)) {
                return CityAssault::Fail;
            }
        }
        hu = s.m_units.get_unit_add(head);
        if (hu == nullptr || hu->m_x == U16_KEY_NULL) {
            return CityAssault::Fail;
        }
    }
    *army_hd = head;
    UnitAddKey still = UnitAddKey::None();
    if (pick_defender(s, city_x, city_y, atk_seat, &still)) {
        if (out_stay != nullptr) {
            *out_stay = head;
        }
        return has_melee_cont(s, head) ? CityAssault::Stall : CityAssault::Fail;
    }
    UnitAddKey force = last_atk.is_valid() ? last_atk : head;
    UnitAddKey stay = UnitAddKey::None();
    UnitAddKey go = UnitAddKey::None();
    if (!UnitMovementMng::split_group_half_by_type(s, head, force, &stay, &go)) {
        if (out_stay != nullptr) {
            *out_stay = head;
        }
        return CityAssault::Fail;
    }
    refill_grp_mp(s, go, turn_mp);
    {
        UnitAddStruct* gu = s.m_units.get_unit_add(go);
        const bool on_city = gu != nullptr && gu->m_x == city_x && gu->m_y == city_y;
        if (!on_city) {
            if (!UnitMovementMng::can_step(s, go, city_x, city_y, nullptr)
                || !UnitMovementMng::apply_step(s, go, city_x, city_y)) {
                if (out_stay != nullptr) {
                    *out_stay = stay;
                }
                if (out_occupy != nullptr) {
                    *out_occupy = go;
                }
                return CityAssault::Fail;
            }
        }
    }
    if (out_stay != nullptr) {
        *out_stay = stay;
    }
    if (out_occupy != nullptr) {
        *out_occupy = go;
    }
    return CityAssault::Ok;
}


CityAttackResult CityAttackManager::assault (
    GameState& s,
    UnitAddKey* army_hd,
    u16 city_x,
    u16 city_y)
{
    CityAttackResult out = {};
    out.m_r = CityAssault::Fail;
    out.m_stay = UnitAddKey::None();
    out.m_occupy = UnitAddKey::None();
    out.m_br_tot = 0u;
    out.m_br_last = 0u;
    if (army_hd == nullptr || !army_hd->is_valid()) {
        return out;
    }
    refill_mp(s, *army_hd);
    CityBarrage br = {};
    if (!barrage(s, *army_hd, city_x, city_y, &br)) {
        return out;
    }
    out.m_br_tot = br.m_tot;
    out.m_br_last = br.m_last;
    refill_mp(s, *army_hd);
    if (!barrage(s, *army_hd, city_x, city_y, &br)) {
        return out;
    }
    out.m_br_tot += br.m_tot;
    out.m_br_last = br.m_last;
    out.m_r = melee(s, army_hd, city_x, city_y, &out.m_stay, &out.m_occupy);
    return out;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
