//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler_core.h"

#include "bit_array.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - CityTurnHandler_Core -
//================================================================================================================================

u16 CityTurnHandler_Core::find_settler_typ (const GameState& state, const BitArrayCL* units) {
    const u32 n = units->get_count();
    for (u32 i = 0; i < n; ++i) {
        if (units->get_bit(i) == 0) {
            continue;
        }
        const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(static_cast<u16>(i));
        if (state.m_statics->unit().get_item(uk).type == state.m_land_settler_type_idx) {
            return static_cast<u16>(i);
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
