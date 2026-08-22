//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef STD_ADD_HELPER_H
#define STD_ADD_HELPER_H

#include "game_primitives.h"

struct GameTileSimple;

//================================================================================================================================
//=> - StdAddHelper -
//================================================================================================================================
//
//  Interprets GameTileSimple::m_add_idx as a bit field when m_ov is Farm or Forest.
//  Farm occupancy is m_ov itself; irrigation and water mill bits live under Farm; saw mill under Forest.
//
//================================================================================================================================

class StdAddHelper {
public:
    static const u16 m_farm_bit = 1u; // Legacy; Farm occupancy is m_ov == Farm
    static const u16 m_mill_bit = 2u; // Farm or Forest payload bit: water mill / saw mill
    static const u16 m_irr_bit = 4u; // Farm payload bit: irrigation present

    static bool has_farm (const GameTileSimple* t);
    static void set_farm (GameTileSimple* t);
    static bool has_mill (const GameTileSimple* t);
    static void set_mill (GameTileSimple* t);
    static bool has_irr (const GameTileSimple* t);
    static void set_irr (GameTileSimple* t);

private:
    StdAddHelper () = delete;
};

#endif // STD_ADD_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
