//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_GROUP_MANAGEMENT_H
#define UNIT_GROUP_MANAGEMENT_H

#include "game_primitives.h"
#include "unit_add_vector_key.h"

class GameState;

//================================================================================================================================
//=> - UnitGroupManagement -
//================================================================================================================================
//
//  Group selection policy for muster / campaign (leave-one / leave-five). Dry collect APIs share
//  the same picks with live leave helpers; UnitMovementMng keeps thin wrappers for call sites.
//
//================================================================================================================================

class UnitGroupManagement {
public:
    UnitGroupManagement () = delete;

    static bool muster_collect_depart (
        GameState& s,
        u16 x,
        u16 y,
        u16 player_idx,
        UnitAddKey* out_keys,
        u16 cap,
        u16* out_n);

    static bool campaign_collect_depart (
        GameState& s,
        const UnitAddKey* in_keys,
        u16 in_n,
        UnitAddKey* out_keys,
        u16 cap,
        u16* out_n);

    static bool muster_leave_one_defense (GameState& s, u16 x, u16 y, u16 player_idx, UnitAddKey* out_head);
    static bool campaign_leave_five_defense (GameState& s, u16 x, u16 y, u16 player_idx, UnitAddKey* out_head);
};

#endif // UNIT_GROUP_MANAGEMENT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
