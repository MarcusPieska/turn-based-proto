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
//  Group selection policy: muster takes all non-tail units on the tile; campaign leave-five.
//  campaign_form_land_army is the single-pass leave-five + land-army form used by war form_army.
//  partition_land_armies_by_arty round-robins each unit_type into arty-sized armies; role NONE is one trailing group.
//
//================================================================================================================================

class UnitGroupManagement { 
public:
    static const u16 k_arty_lim = 20u; // Target min LAND_ARTILLERY units per partitioned army

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
    static bool campaign_form_land_army (GameState& s, u16 x, u16 y, u16 player_idx, UnitAddKey* out_head);
    static bool partition_land_armies_by_arty (GameState& s, UnitAddKey head, UnitAddKey* out_heads, u16 cap, u16* out_n);
};

#endif // UNIT_GROUP_MANAGEMENT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
