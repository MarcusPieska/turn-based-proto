//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "army_reassembler.h"

#include "army_general.h"
#include "game_state.h"
#include "generate_access_mask.h"
#include "generate_distance_p2p.h"
#include "tile_entry_resolver.h"
#include "unit_add_struct.h"
#include "unit_movement_mng.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - ArmyReassembler -
//================================================================================================================================

ArmyReassembler::ArmyReassembler () :
    m_mob(),
    m_tx(U16_KEY_NULL),
    m_ty(U16_KEY_NULL),
    m_ok(false) {
}

ArmyReassembler::~ArmyReassembler () {
}

void ArmyReassembler::clr () {
    m_tx = U16_KEY_NULL;
    m_ty = U16_KEY_NULL;
    m_ok = false;
}

bool ArmyReassembler::ok () const {
    return m_ok;
}

u16 ArmyReassembler::tx () const {
    return m_tx;
}

u16 ArmyReassembler::ty () const {
    return m_ty;
}

bool ArmyReassembler::begin (GameState& st, u16 seat, u16 tx, u16 ty) {
    clr();
    if (st.m_statics == nullptr || seat >= st.m_player_n) {
        return false;
    }
    Whiteboard_1B acc("ArmyReassembler", "acc", 0u);
    if (!GenerateAccessMask::generate_own_free(st, seat, acc)) {
        return false;
    }
    u16 dmax = 0u;
    if (!GenerateDistanceP2P::generate(st, *st.m_statics, tx, ty, m_mob, &acc, &dmax)) {
        return false;
    }
    m_tx = tx;
    m_ty = ty;
    m_ok = true;
    return true;
}

bool ArmyReassembler::at_goal (GameState& st, const u16* heads, u16 n) const {
    if (!m_ok || heads == nullptr || n == 0u) {
        return false;
    }
    for (u16 i = 0; i < n; ++i) {
        if (heads[i] == U16_KEY_NULL) {
            continue;
        }
        const UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(heads[i]));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            return false;
        }
        if (u->m_x != m_tx || u->m_y != m_ty) {
            return false;
        }
    }
    return true;
}

bool ArmyReassembler::tick (GameState& st, u16 seat, const u16* heads, u16 n) {
    if (!m_ok || heads == nullptr || n == 0u) {
        return false;
    }
    bool any = false;
    for (u16 i = 0; i < n; ++i) {
        if (heads[i] == U16_KEY_NULL) {
            continue;
        }
        const UnitAddKey key = UnitAddKey::from_raw(heads[i]);
        UnitAddStruct* u = st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (u->m_x == m_tx && u->m_y == m_ty) {
            continue;
        }
        for (;;) {
            u = st.m_units.get_unit_add(key);
            if (u == nullptr || u->m_x == U16_KEY_NULL) {
                break;
            }
            if (u->m_x == m_tx && u->m_y == m_ty) {
                break;
            }
            const u16 ux = u->m_x;
            const u16 uy = u->m_y;
            const WalkP2P::StepRes step = m_mob.peek(st, ux, uy);
            if (!step.have) {
                break;
            }
            const CanStepFail csr = UnitMovementMng::can_step_reason(st, key, step.nx, step.ny, nullptr);
            if (csr == CanStepFail::Ok) {
                if (!UnitMovementMng::apply_step(st, key, step.nx, step.ny)) {
                    break;
                }
                ArmyGeneral::stamp_moved(st, seat, heads[i]);
                any = true;
                continue;
            }
            if (csr != CanStepFail::Entry) {
                break;
            }
            if (TileEntryResolver::resolve(heads[i], step.nx, step.ny) != TileEntryRes::BlockRemains) {
                if (UnitMovementMng::can_step(st, key, step.nx, step.ny, nullptr)
                    && UnitMovementMng::apply_step(st, key, step.nx, step.ny)) {
                    ArmyGeneral::stamp_moved(st, seat, heads[i]);
                    any = true;
                    continue;
                }
            }
            break;
        }
    }
    return any;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
