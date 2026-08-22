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
//  Farm occupancy is m_ov == Farm; m_add_idx bits follow WorkerJobImpIndex slot (1 << slot).
//
//================================================================================================================================

class StdAddHelper {
public:
    static const u16 m_farm_bit = 1u; // Legacy; Farm occupancy is m_ov == Farm
    static const u16 m_mill_bit = 2u; // Farm slot-1 payload bit (Water Mill)
    static const u16 m_irr_bit = 1u; // Farm slot-0 payload bit (Irrigation)

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
