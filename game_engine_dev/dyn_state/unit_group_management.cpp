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
#include "unit_action_enum.h"
#include "unit_role_enum.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"
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

static u16 ug_utype (const GameState& s, u16 typ_idx) {
    if (s.m_statics == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 un = s.m_statics->unit().get_item_count();
    if (typ_idx >= un) {
        return U16_KEY_NULL;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ_idx)).type;
}

static bool ug_is_role_none (const GameState& s, u16 typ_idx) {
    if (s.m_statics == nullptr) {
        return false;
    }
    const u16 un = s.m_statics->unit().get_item_count();
    if (typ_idx >= un) {
        return false;
    }
    const u16 role = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(typ_idx)).role;
    return role == static_cast<u16>(UnitRole::NONE);
}

static bool ug_is_arty (const GameState& s, u16 typ_idx) {
    const u16 ut = ug_utype(s, typ_idx);
    if (ut == U16_KEY_NULL || s.m_statics == nullptr) {
        return false;
    }
    return s.m_statics->unit_type_action_map().unit_type_can_do(ut, static_cast<u16>(UnitAction::canBarrage));
}

static void ug_tile_unlink (GameState& s, UnitAddKey key) {
    UnitAddStruct* u = ug_get(s, key);
    if (u == nullptr || ug_is_tail(*u)) {
        return;
    }
    const u16 ox = u->m_x;
    const u16 oy = u->m_y;
    if (!ug_in_bounds(s, ox, oy)) {
        u->m_next_unit_on_tile = U16_KEY_NULL;
        return;
    }
    const u16 hd = s.m_map.get_unit_hd(ox, oy);
    if (hd == key.value()) {
        s.m_map.set_unit_hd(ox, oy, u->m_next_unit_on_tile);
    } else {
        UnitAddKey cur = UnitAddKey::from_raw(hd);
        while (cur.is_valid()) {
            UnitAddStruct* cu = ug_get(s, cur);
            if (cu == nullptr) {
                break;
            }
            if (cu->m_next_unit_on_tile == key.value()) {
                cu->m_next_unit_on_tile = u->m_next_unit_on_tile;
                break;
            }
            if (cu->m_next_unit_on_tile == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(cu->m_next_unit_on_tile);
        }
    }
    u->m_next_unit_on_tile = U16_KEY_NULL;
}

static bool ug_tile_prepend (GameState& s, UnitAddKey key, u16 x, u16 y) {
    UnitAddStruct* u = ug_get(s, key);
    if (u == nullptr || !ug_in_bounds(s, x, y)) {
        return false;
    }
    u->m_x = x;
    u->m_y = y;
    u->m_next_unit_on_tile = s.m_map.get_unit_hd(x, y);
    return s.m_map.set_unit_hd(x, y, key.value());
}

static void ug_chain_add (GameState& s, UnitAddKey* hd, UnitAddKey* tl, UnitAddKey k) {
    UnitAddStruct* u = ug_get(s, k);
    if (u == nullptr || hd == nullptr || tl == nullptr) {
        return;
    }
    u->m_next_unit_in_group = U16_KEY_NULL;
    if (!hd->is_valid()) {
        *hd = k;
        *tl = k;
        return;
    }
    UnitAddStruct* t = ug_get(s, *tl);
    if (t != nullptr) {
        t->m_next_unit_in_group = k.value();
    }
    *tl = k;
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

bool UnitGroupManagement::partition_land_armies_by_arty (
    GameState& s,
    UnitAddKey head,
    UnitAddKey* out_heads,
    u16 cap,
    u16* out_n) 
{
    if (out_heads == nullptr || out_n == nullptr || cap == 0u || !head.is_valid()
        || s.m_statics == nullptr) {
        return false;
    }
    *out_n = 0u;
    static const u16 k_cap = 8192u;
    static const u16 k_ut_max = 256u;
    const u16 tn = s.m_statics->unit_type().get_item_count();
    if (tn == 0u || tn > k_ut_max) {
        return false;
    }
    UnitAddStruct* hu = ug_get(s, head);
    if (hu == nullptr || ug_is_tail(*hu) || hu->m_x == U16_KEY_NULL) {
        return false;
    }
    const u16 x = hu->m_x;
    const u16 y = hu->m_y;
    UnitAddKey keys[k_cap];
    u16 nxt_i[k_cap];
    u16 n = 0;
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        if (n >= k_cap) {
            return false;
        }
        keys[n] = cur;
        nxt_i[n] = U16_KEY_NULL;
        UnitAddStruct* u = ug_get(s, cur);
        if (u == nullptr) {
            return false;
        }
        const u16 fol = u->m_next_unit_in_group;
        u->m_next_unit_in_group = U16_KEY_NULL;
        n = static_cast<u16>(n + 1u);
        if (fol == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(fol);
    }
    if (n == 0u) {
        return false;
    }
    u16 typ_hd[k_ut_max];
    u16 typ_tl[k_ut_max];
    for (u16 t = 0; t < tn; ++t) {
        typ_hd[t] = U16_KEY_NULL;
        typ_tl[t] = U16_KEY_NULL;
    }
    u16 none_hd = U16_KEY_NULL;
    u16 none_tl = U16_KEY_NULL;
    u16 arty_n = 0;
    u16 use_n = 0;
    u16 none_n = 0;
    for (u16 i = 0; i < n; ++i) {
        const UnitAddStruct* u = ug_get(s, keys[i]);
        if (u == nullptr) {
            return false;
        }
        const u16 ut = ug_utype(s, u->m_unit_typ_idx);
        if (ut == U16_KEY_NULL || ut >= tn || ug_is_role_none(s, u->m_unit_typ_idx)) {
            if (none_hd == U16_KEY_NULL) {
                none_hd = i;
            } else {
                nxt_i[none_tl] = i;
            }
            none_tl = i;
            none_n = static_cast<u16>(none_n + 1u);
            continue;
        }
        use_n = static_cast<u16>(use_n + 1u);
        if (ug_is_arty(s, u->m_unit_typ_idx)) {
            arty_n = static_cast<u16>(arty_n + 1u);
        }
        if (typ_hd[ut] == U16_KEY_NULL) {
            typ_hd[ut] = i;
        } else {
            nxt_i[typ_tl[ut]] = i;
        }
        typ_tl[ut] = i;
    }
    if (use_n == 0u && none_n == 0u) {
        return false;
    }
    u16 g = 0;
    if (use_n > 0u) {
        g = static_cast<u16>(arty_n / k_arty_lim);
        if (g == 0u) {
            g = 1u;
        }
        const u16 g_max = none_n > 0u ? static_cast<u16>(cap - 1u) : cap;
        if (g_max == 0u) {
            return false;
        }
        if (g > g_max) {
            g = g_max;
        }
    }
    if (g > k_arty_lim) {
        g = k_arty_lim;
    }
    UnitAddKey arm_hd[k_arty_lim];
    UnitAddKey arm_tl[k_arty_lim];
    for (u16 gi = 0; gi < g; ++gi) {
        arm_hd[gi] = UnitAddKey::None();
        arm_tl[gi] = UnitAddKey::None();
    }
    for (u16 t = 0; t < tn; ++t) {
        u16 ix = typ_hd[t];
        u16 gi = 0;
        while (ix != U16_KEY_NULL) {
            const u16 fol = nxt_i[ix];
            ug_chain_add(s, &arm_hd[gi], &arm_tl[gi], keys[ix]);
            gi = static_cast<u16>(gi + 1u);
            if (gi >= g) {
                gi = 0u;
            }
            ix = fol;
        }
    }
    UnitAddKey none_out = UnitAddKey::None();
    UnitAddKey none_out_tl = UnitAddKey::None();
    {
        u16 ix = none_hd;
        while (ix != U16_KEY_NULL) {
            const u16 fol = nxt_i[ix];
            ug_chain_add(s, &none_out, &none_out_tl, keys[ix]);
            ix = fol;
        }
    }
    ug_tile_unlink(s, head);
    for (u16 i = 0; i < n; ++i) {
        UnitAddStruct* u = ug_get(s, keys[i]);
        if (u == nullptr) {
            return false;
        }
        u->m_x = U16_KEY_NULL;
        u->m_y = U16_KEY_NULL;
        u->m_next_unit_on_tile = U16_KEY_NULL;
    }
    for (u16 gi = 0; gi < g; ++gi) {
        if (!arm_hd[gi].is_valid()) {
            continue;
        }
        if (!ug_tile_prepend(s, arm_hd[gi], x, y)) {
            return false;
        }
        if (*out_n >= cap) {
            return false;
        }
        out_heads[*out_n] = arm_hd[gi];
        *out_n = static_cast<u16>(*out_n + 1u);
    }
    if (none_out.is_valid()) {
        if (!ug_tile_prepend(s, none_out, x, y)) {
            return false;
        }
        if (*out_n >= cap) {
            return false;
        }
        out_heads[*out_n] = none_out;
        *out_n = static_cast<u16>(*out_n + 1u);
    }
    return *out_n > 0u;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
