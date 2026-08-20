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
//  Interprets GameTileSimple::m_add_idx as a local bit field when m_add_typ is BUILD_ADD_STD.
//  Callers must only use these helpers on STD tiles; mistyped tiles trip GAME_EXPECT.
//
//================================================================================================================================

class StdAddHelper {
public:
    static const u16 m_farm_bit = 1u; // First bit of m_add_idx: farm present
    static const u16 m_mill_bit = 2u; // Second bit of m_add_idx: saw mill present
    static const u16 m_irr_bit = 4u; // Third bit of m_add_idx: irrigation present

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
