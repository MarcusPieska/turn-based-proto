//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_access_mask.h"

#include "civ_relations.h"
#include "civ_static_key.h"
#include "game_array_simple.h"
#include "game_primitives.h"
#include "game_state.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - GenerateAccessMask -
//================================================================================================================================

bool GenerateAccessMask::generate (const GameState& s, u16 self_seat, Whiteboard_1B& out) {
    if (!out.ok()) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    if (w == 0 || h == 0 || out.w() != w || out.h() != h) {
        return false;
    }
    const u16 pn = s.m_player_n;
    if (s.m_player_states == nullptr || pn == 0 || self_seat >= pn) {
        return false;
    }
    const u16 civ_n = s.m_civ_relations.civ_n();
    if (civ_n == 0) {
        return false;
    }
    const u16 self_civ = s.m_player_states[self_seat].m_civ_index;
    if (self_civ >= civ_n) {
        return false;
    }
    u8 seat_acc[256];
    for (u16 i = 0; i < 256u; ++i) {
        seat_acc[i] = k_block;
    }
    const CivStaticDataKey sk_self = CivStaticDataKey::from_raw(self_civ);
    for (u16 p = 0; p < pn; ++p) {
        if (p == self_seat) {
            seat_acc[p] = k_open;
            continue;
        }
        const u16 oth_civ = s.m_player_states[p].m_civ_index;
        if (oth_civ >= civ_n) {
            continue;
        }
        const CivRel rel = s.m_civ_relations.get(sk_self, CivStaticDataKey::from_raw(oth_civ));
        if (rel != CivRel::CIV_REL_PEACE) {
            seat_acc[p] = k_open;
        }
    }
    const u32 n = s.m_map.tile_n();
    u8* m = out.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u8 own = s.m_map.get_civ_owner(x, y);
        const bool open = own == U8_KEY_NULL || seat_acc[own] == k_open;
        m[i] = open ? k_open : k_block;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
