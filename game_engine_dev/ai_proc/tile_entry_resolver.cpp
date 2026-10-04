//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "tile_entry_resolver.h"

#include "game_state.h"
#include "log_dbg.h"
#include "runtime_statics.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//=> - State -
//================================================================================================================================

GameState* TileEntryResolver::m_st = nullptr;

//================================================================================================================================
//=> - Bind -
//================================================================================================================================

void TileEntryResolver::bind_state (GameState* state) {
    m_st = state;
}

//================================================================================================================================
//=> - Stack helpers -
//================================================================================================================================

static bool typ_can (const GameState& s, u16 typ, UnitAction act) {
    if (s.m_statics == nullptr) {
        return false;
    }
    return s.m_statics->unit_type_action_map().unit_type_can_do(typ, static_cast<u16>(act));
}

static void flip_unit (UnitAddStruct* u, u16 self_seat) {
    if (u == nullptr) {
        return;
    }
    u->m_player_idx = self_seat;
    u->m_in_campaign = 0u;
}

static void flip_group (GameState& s, UnitAddKey head, u16 self_seat) {
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        flip_unit(u, self_seat);
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
}

bool TileEntryResolver::stack_all_capturable (u16 x, u16 y, u16 foe_seat) {
    if (m_st == nullptr) {
        return false;
    }
    GameState& s = *m_st;
    const u16 hd = s.m_map.get_unit_hd(x, y);
    if (hd == U16_KEY_NULL) {
        return false;
    }
    UnitAddKey cur = UnitAddKey::from_raw(hd);
    bool any = false;
    while (cur.is_valid()) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            return false;
        }
        if (u->m_player_idx != foe_seat) {
            return false;
        }
        if (!typ_can(s, static_cast<u16>(u->m_unit_typ_idx), UnitAction::canBeCaptured)) {
            return false;
        }
        any = true;
        UnitAddKey g = cur;
        while (g.is_valid()) {
            const UnitAddStruct* gu = s.m_units.get_unit_add(g);
            if (gu == nullptr) {
                return false;
            }
            if (gu != u) {
                if (gu->m_player_idx != foe_seat) {
                    return false;
                }
                if (!typ_can(s, static_cast<u16>(gu->m_unit_typ_idx), UnitAction::canBeCaptured)) {
                    return false;
                }
            }
            if (gu->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            g = UnitAddKey::from_raw(gu->m_next_unit_in_group);
        }
        if (u->m_next_unit_on_tile == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_on_tile);
    }
    return any;
}

void TileEntryResolver::stack_flip_owner (u16 x, u16 y, u16 self_seat) {
    if (m_st == nullptr) {
        return;
    }
    GameState& s = *m_st;
    const u16 hd = s.m_map.get_unit_hd(x, y);
    if (hd == U16_KEY_NULL) {
        return;
    }
    UnitAddKey cur = UnitAddKey::from_raw(hd);
    while (cur.is_valid()) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        const u16 nxt = static_cast<u16>(u->m_next_unit_on_tile);
        flip_group(s, cur, self_seat);
        if (nxt == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(nxt);
    }
}

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

TileEntryRes TileEntryResolver::try_capture (u16 self_seat, u16 unit_idx, u16 x, u16 y, u16 foe_seat) {
    // TODO: connect any captured worker to a job process (or similar) so work does not linger idle.
    // Capture/expel may later offer a "ransom" path where the AI lets the player buy units back.
    if (!stack_all_capturable(x, y, foe_seat)) {
        return TileEntryRes::BlockRemains;
    }
    stack_flip_owner(x, y, self_seat);
    LOG_WAR_TILE_ENTRY_RESOLVE::LOG(
        SC_U32(self_seat), SC_U32(unit_idx), SC_U32(x), SC_U32(y), 1u,
        SC_U32(static_cast<u8>(TileEntryRes::Cleared)), SC_U32(m_st->m_current_turn));
    return TileEntryRes::Cleared;
}

TileEntryRes TileEntryResolver::try_attack (u16 self_seat, u16 unit_idx, u16 x, u16 y, u16 foe_seat) {
    (void)foe_seat;
    // TODO: hostile non-capturable stack — attack and clear the tile.
    LOG_WAR_TILE_ENTRY_RESOLVE::LOG(
        SC_U32(self_seat), SC_U32(unit_idx), SC_U32(x), SC_U32(y), 2u,
        SC_U32(static_cast<u8>(TileEntryRes::BlockRemains)), SC_U32(m_st->m_current_turn));
    return TileEntryRes::BlockRemains;
}

TileEntryRes TileEntryResolver::try_expel (u16 self_seat, u16 unit_idx, u16 x, u16 y, u16 foe_seat) {
    (void)foe_seat;
    // TODO: expel blocker to its capital (coord move). Capture/expel may later offer a "ransom" buy-back.
    LOG_WAR_TILE_ENTRY_RESOLVE::LOG(
        SC_U32(self_seat), SC_U32(unit_idx), SC_U32(x), SC_U32(y), 3u,
        SC_U32(static_cast<u8>(TileEntryRes::BlockRemains)), SC_U32(m_st->m_current_turn));
    return TileEntryRes::BlockRemains;
}

//================================================================================================================================
//=> - Resolve -
//================================================================================================================================

TileEntryRes TileEntryResolver::resolve (u16 unit_idx, u16 x, u16 y) {
    if (m_st == nullptr || m_st->m_statics == nullptr) {
        return TileEntryRes::BlockRemains;
    }
    GameState& s = *m_st;
    const UnitAddStruct* mover = s.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (mover == nullptr) {
        return TileEntryRes::BlockRemains;
    }
    const u16 self_seat = static_cast<u16>(mover->m_player_idx);
    const u16 hd = s.m_map.get_unit_hd(x, y);
    if (hd == U16_KEY_NULL) {
        return TileEntryRes::BlockRemains;
    }
    const UnitAddStruct* blk = s.m_units.get_unit_add(UnitAddKey::from_raw(hd));
    if (blk == nullptr || blk->m_player_idx == self_seat) {
        return TileEntryRes::BlockRemains;
    }
    const u16 foe_seat = static_cast<u16>(blk->m_player_idx);
    if (stack_all_capturable(x, y, foe_seat)) {
        return try_capture(self_seat, unit_idx, x, y, foe_seat);
    }
    bool armed = false;
    UnitAddKey cur = UnitAddKey::from_raw(hd);
    while (cur.is_valid()) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        const u16 typ = static_cast<u16>(u->m_unit_typ_idx);
        if (typ_can(s, typ, UnitAction::canAttack) || typ_can(s, typ, UnitAction::canBarrage)) {
            armed = true;
            break;
        }
        if (u->m_next_unit_on_tile == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_on_tile);
    }
    if (armed) {
        return try_attack(self_seat, unit_idx, x, y, foe_seat);
    }
    return try_expel(self_seat, unit_idx, x, y, foe_seat);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
