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
    UnitTypeActionMap () = default;
    void set_map (StaticBitBank* bit_bank, u16 unit_type_count, u16 action_count);
    void take_ownership ();
    void release_map ();
    bool unit_type_can_do (u16 unit_type_idx, u16 action_idx) const;
    u16 get_unit_type_count () const;
    u16 get_action_count () const;

private:
    UnitTypeActionMap (const UnitTypeActionMap& other) = delete;
    UnitTypeActionMap (UnitTypeActionMap&& other) = delete;
    UnitTypeActionMap& operator= (const UnitTypeActionMap& other) = delete;
    UnitTypeActionMap& operator= (UnitTypeActionMap&& other) = delete;

    StaticBitBank* m_bit_bank = nullptr;
    u16 m_unit_type_count = 0;
    u16 m_action_count = 0;
};

#endif // UNIT_TYPE_ACTION_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
