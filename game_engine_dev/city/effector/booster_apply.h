//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BOOSTER_APPLY_H
#define BOOSTER_APPLY_H

#include "booster_effect_register.h"
#include "game_primitives.h"
#include "unit_add_struct.h"

//================================================================================================================================
//=> - Booster apply helpers -
//================================================================================================================================

inline u16 apply_booster_u16 (u16 base, const BoosterRegisterResult& b) {
    i32 v = base;
    if (b.m_perc != 0) {
        v = (v * (100 + static_cast<i32>(b.m_perc))) / 100;
    }
    v += static_cast<i32>(b.m_unit);
    if (v <= 0) {
        return 0;
    }
    if (v > 65535) {
        return 65535;
    }
    return static_cast<u16>(v);
}

inline u8 apply_unit_level_boost (u8 base, const BoosterRegisterResult& b) {
    i32 v = static_cast<i32>(base) + static_cast<i32>(b.m_unit);
    if (v < VERY_GREEN) {
        return VERY_GREEN;
    }
    if (v > ELITE) {
        return ELITE;
    }
    return static_cast<u8>(v);
}

#endif // BOOSTER_APPLY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
