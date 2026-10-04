//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_UTILITY_HELPER_H
#define UNIT_UTILITY_HELPER_H

#include "game_primitives.h"

class RuntimeStatics;
class BitArrayCL;

//================================================================================================================================
//=> - UnitUtilityHelper -
//================================================================================================================================
//
//  Static utility bit table over unit types. Rows are named predicates; each row is one bit per
//  unit type. Setup fills ROW_IS_LAND_ARMY_UNIT from UnitTypeActionMap (canEmbark and attack or
//  barrage or anti-air). Query via is_type_for_land_army (unit type index, not catalog unit index).
//
//================================================================================================================================

class UnitUtilityHelper {
public:
    UnitUtilityHelper () = delete;

    static bool setup (const RuntimeStatics& st);
    static void clear ();
    static bool is_type_for_land_army (u16 unit_type);

private:
    enum Row : u16 {
        ROW_IS_LAND_ARMY_UNIT = 0, // canEmbark and (canAttack or canBarrage or canDoAntiAir)
        ROW_N = 1 // Row count; grow storage when this exceeds 1
    };

    static BitArrayCL* m_bits; // Packed utility bits (row * type_n + type)
    static u16 m_type_n; // Unit type count at setup
};

#endif // UNIT_UTILITY_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
