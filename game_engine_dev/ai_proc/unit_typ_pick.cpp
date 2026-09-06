//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_typ_pick.h"

#include "bit_array.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - UnitTypPick -
//================================================================================================================================

u16 UnitTypPick::pick_linear_right (const GameState& state, const BitArrayCL* available, u16 type_idx) {
    if (available == nullptr || state.m_statics == nullptr || type_idx == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    const u32 n = available->get_count();
    if (n == 0u) {
        return U16_KEY_NULL;
    }
    for (u32 i = n; i > 0u; --i) {
        const u16 idx = static_cast<u16>(i - 1u);
        if (available->get_bit(idx) == 0) {
            continue;
        }
        const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(idx);
        if (state.m_statics->unit().get_item(uk).type == type_idx) {
            return idx;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
