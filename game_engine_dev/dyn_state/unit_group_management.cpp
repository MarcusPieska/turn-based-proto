//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_group_management.h"

#include <cstring> 

#include "game_map_defs.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "unit_utility_helper.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static UnitAddStruct* ug_get (GameState& s, UnitAddKey key) {
    return s.m_units.get_unit_add(key);
}

static bool ug_in_bounds (const GameState& s, u16 x, u16 y) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    return w > 0 && h > 0 && x < w && y < h;
}

static bool ug_is_tail (const UnitAddStruct& u) {
    return u.m_x == U16_KEY_NULL && u.m_y == U16_KEY_NULL;
}

static bool ug_is_defense (const GameState& s, u16 typ_idx) {
    if (s.m_statics == nullptr) {
        return false;
    }
    const u16 un = s.m_statics->unit().get_item_count();
    if (typ_idx >= un) {
        return false;
    }
    const u16 ut = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ_idx)).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    cstr nm = s.m_statics->unit_type().get_name(tk);
    return nm != nullptr && std::strcmp(nm, "LAND_DEFENSE") == 0;
}

static bool ug_is_land_army (const GameState& s, u16 typ_idx) {
    if (s.m_statics == nullptr) {
        return false;
    }
    const u16 un = s.m_statics->unit().get_item_count();
    if (typ_idx >= un) {
        return false;
    }
    const u16 ut = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ_idx)).type;
    return UnitUtilityHelper::is_type_for_land_army(ut);
}

static bool ug_flat_tile_grps (GameState& s, const UnitAddKey* heads, u16 hn, u16 x, u16 y) {
    for (u16 i = 0; i < hn; ++i) {
        UnitAddStruct* hu = ug_get(s, heads[i]);
        if (hu == nullptr) {
            return false;
        }
        UnitAddKey nxt = UnitAddKey::from_raw(hu->m_next_unit_in_group);
        hu->m_next_unit_in_group = U16_KEY_NULL;
        while (nxt.is_valid()) {
            UnitAddStruct* tu = ug_get(s, nxt);
            if (tu == nullptr) {
                return false;
            }
            const UnitAddKey fol = UnitAddKey::from_raw(tu->m_next_unit_in_group);
            tu->m_next_unit_in_group = U16_KEY_NULL;
            if (!UnitMovementMng::stack_append(s, nxt, x, y)) {
                return false;
            }
            nxt = fol;
        }
    }
    return true;
}

//================================================================================================================================
//=> - UnitGroupManagement -
//================================================================================================================================

bool UnitGroupManagement::muster_collect_depart (
    GameState& s,
    u16 x,
    u16 y,
    u16 player_idx,
    UnitAddKey* out_keys,
    u16 cap,
    u16* out_n) {
    if (out_keys == nullptr || out_n == nullptr || cap == 0u
        || !ug_in_bounds(s, x, y) || player_idx >= s.m_player_n) {
        return false;
    }
    *out_n = 0u;
    static const u16 k_cap = 64u;
    UnitAddKey keys[k_cap];
    u16 n = 0;
    u16 cur = s.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL && n < k_cap) {
        const UnitAddKey k = UnitAddKey::from_raw(cur);
        const UnitAddStruct* u = ug_get(s, k);
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == player_idx && !ug_is_tail(*u)) {
            keys[n++] = k;
        }
        cur = u->m_next_unit_on_tile;
    }
    if (n == 0u) {
        return false;
    }
    for (u16 i = 0; i < n; ++i) {
        if (*out_n >= cap) {
            return false;
        }
        out_keys[*out_n] = keys[i];
        *out_n = static_cast<u16>(*out_n + 1u);
    }
    return true;
}

bool UnitGroupManagement::campaign_collect_depart (
    GameState& s,
    const UnitAddKey* in_keys,
    u16 in_n,
    UnitAddKey* out_keys,
    u16 cap,
    u16* out_n) {
    if (in_keys == nullptr || out_keys == nullptr || out_n == nullptr || cap == 0u) {
        return false;
    }
    *out_n = 0u;
    if (in_n < 2u) {
        return false;
    }
    static const u16 k_leave = 5u;
    static const u16 k_cap = 2048u;
    if (in_n > k_cap) {
        return false;
    }
    bool leave[k_cap];
    for (u16 i = 0; i < in_n; ++i) {
        leave[i] = false;
    }
    u16 left = 0;
    for (u16 i = 0; i < in_n && left < k_leave; ++i) {
        const UnitAddStruct* u = ug_get(s, in_keys[i]);
        if (u != nullptr && ug_is_defense(s, u->m_unit_typ_idx)) {
            leave[i] = true;
            left++;
        }
    }
    for (u16 i = 0; i < in_n; ++i) {
        if (leave[i]) {
            continue;
        }
        if (*out_n >= cap) {
            return false;
        }
        out_keys[*out_n] = in_keys[i];
        *out_n = static_cast<u16>(*out_n + 1u);
    }
    return *out_n > 0u;
}

bool UnitGroupManagement::muster_leave_one_defense (
    GameState& s,
    u16 x,
    u16 y,
    u16 player_idx,
    UnitAddKey* out_head) {
    if (out_head == nullptr) {
        return false;
    }
    static const u16 k_cap = 64u;
    UnitAddKey keys[k_cap];
    u16 n = 0;
    if (!muster_collect_depart(s, x, y, player_idx, keys, k_cap, &n) || n == 0u) {
        return false;
    }
    UnitAddKey head = keys[0];
    for (u16 i = 1; i < n; ++i) {
        if (!UnitMovementMng::link_group(s, head, keys[i])) {
            return false;
        }
    }
    *out_head = head;
    return true;
}

bool UnitGroupManagement::campaign_leave_five_defense (
    GameState& s,
    u16 x,
    u16 y,
    u16 player_idx,
    UnitAddKey* out_head) {
    if (out_head == nullptr || !ug_in_bounds(s, x, y) || player_idx >= s.m_player_n) {
        return false;
    }
    static const u16 k_cap = 2048u;
    UnitAddKey heads[k_cap];
    u16 hn = 0;
    u16 cur = s.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL && hn < k_cap) {
        const UnitAddKey k = UnitAddKey::from_raw(cur);
        const UnitAddStruct* u = ug_get(s, k);
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == player_idx && !ug_is_tail(*u)) {
            heads[hn++] = k;
        }
        cur = u->m_next_unit_on_tile;
    }
    if (hn == 0u) {
        return false;
    }
    UnitAddKey keys[k_cap];
    u16 n = 0;
    for (u16 i = 0; i < hn && n < k_cap; ++i) {
        keys[n++] = heads[i];
    }
    for (u16 i = 0; i < hn && n < k_cap; ++i) {
        const UnitAddStruct* hu = ug_get(s, heads[i]);
        if (hu == nullptr) {
            return false;
        }
        UnitAddKey nxt = UnitAddKey::from_raw(hu->m_next_unit_in_group);
        while (nxt.is_valid() && n < k_cap) {
            keys[n++] = nxt;
            const UnitAddStruct* tu = ug_get(s, nxt);
            if (tu == nullptr || tu->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            nxt = UnitAddKey::from_raw(tu->m_next_unit_in_group);
        }
    }
    UnitAddKey army[k_cap];
    u16 an = 0;
    if (!campaign_collect_depart(s, keys, n, army, k_cap, &an) || an == 0u) {
        return false;
    }
    if (!ug_flat_tile_grps(s, heads, hn, x, y)) {
        return false;
    }
    return UnitMovementMng::form_group_chain(s, army, an, out_head);
}

bool UnitGroupManagement::campaign_form_land_army (
    GameState& s,
    u16 x,
    u16 y,
    u16 player_idx,
    UnitAddKey* out_head) {
    if (out_head == nullptr || !ug_in_bounds(s, x, y) || player_idx >= s.m_player_n || s.m_statics == nullptr) {
        return false;
    }
    static const u16 k_cap = 2048u;
    static const u16 k_leave = 5u;
    UnitAddKey heads[k_cap];
    u16 hn = 0;
    u16 cur = s.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL && hn < k_cap) {
        const UnitAddKey k = UnitAddKey::from_raw(cur);
        const UnitAddStruct* u = ug_get(s, k);
        if (u == nullptr) {
            break;
        }
        if (u->m_player_idx == player_idx && !ug_is_tail(*u)) {
            heads[hn++] = k;
        }
        cur = u->m_next_unit_on_tile;
    }
    if (hn == 0u) {
        return false;
    }
    UnitAddKey keys[k_cap];
    u16 n = 0;
    for (u16 i = 0; i < hn && n < k_cap; ++i) {
        keys[n++] = heads[i];
    }
    for (u16 i = 0; i < hn && n < k_cap; ++i) {
        const UnitAddStruct* hu = ug_get(s, heads[i]);
        if (hu == nullptr) {
            return false;
        }
        UnitAddKey nxt = UnitAddKey::from_raw(hu->m_next_unit_in_group);
        while (nxt.is_valid() && n < k_cap) {
            keys[n++] = nxt;
            const UnitAddStruct* tu = ug_get(s, nxt);
            if (tu == nullptr || tu->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            nxt = UnitAddKey::from_raw(tu->m_next_unit_in_group);
        }
    }
    if (n < 2u) {
        return false;
    }
    bool leave[k_cap];
    for (u16 i = 0; i < n; ++i) {
        leave[i] = false;
    }
    u16 left = 0;
    for (u16 i = 0; i < n && left < k_leave; ++i) {
        const UnitAddStruct* u = ug_get(s, keys[i]);
        if (u != nullptr && ug_is_defense(s, u->m_unit_typ_idx)) {
            leave[i] = true;
            left++;
        }
    }
    UnitAddKey army[k_cap];
    u16 an = 0;
    for (u16 i = 0; i < n; ++i) {
        if (leave[i]) {
            continue;
        }
        const UnitAddStruct* u = ug_get(s, keys[i]);
        if (u == nullptr || !ug_is_land_army(s, u->m_unit_typ_idx)) {
            continue;
        }
        army[an++] = keys[i];
    }
    if (an == 0u) {
        return false;
    }
    if (!ug_flat_tile_grps(s, heads, hn, x, y)) {
        return false;
    }
    return UnitMovementMng::form_group_chain(s, army, an, out_head);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
