//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_TYPE_ACTION_MAP_H
#define UNIT_TYPE_ACTION_MAP_H

#include "static_bit_bank.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - UnitTypeActionMap class -
//================================================================================================================================

class UnitTypeActionMap {
public:
    static void set_map (StaticBitBank* bit_bank, u16 unit_type_count, u16 action_count);
    static bool unit_type_can_do (u16 unit_type_idx, u16 action_idx);
    static u16 get_unit_type_count ();
    static u16 get_action_count ();

private:
    UnitTypeActionMap () = delete;
    UnitTypeActionMap (const UnitTypeActionMap& other) = delete;
    UnitTypeActionMap (UnitTypeActionMap&& other) = delete;

    static StaticBitBank* m_bit_bank;
    static u16 m_unit_type_count;
    static u16 m_action_count;
};

#endif // UNIT_TYPE_ACTION_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
