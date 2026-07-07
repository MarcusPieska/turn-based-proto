//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ADD_STRUCT_H
#define UNIT_ADD_STRUCT_H

#include "game_primitives.h"

//================================================================================================================================
//=> - UnitAddStruct -
//================================================================================================================================
//
//  Each live unit lives in UnitAddVector and is keyed by UnitAddKey (u16 pool index).
//  The map discovers units per tile via GameTileSimple::m_unit_hd, which holds the
//  head key of a stack at (x, y). Vector iteration discovers placement via m_x/m_y.
//
//  Two orthogonal linked-list axes (a unit is on at most one axis at a time):
//    - m_next_unit_on_tile: co-located allies on the same hex; each moves independently
//    - m_next_unit_in_group: move-as-one chain; only the head may initiate movement
//
//  Group tail units use m_x/m_y = U16_KEY_NULL so vector scans can detect followers
//  that must not move alone. Group heads keep real coords and own m_unit_hd on the map.
//
//  Invariants (enforced by UnitMovementMng, not this struct):
//    - A unit in m_next_unit_in_group is never referenced by m_next_unit_on_tile
//    - Tile m_unit_hd matches the stack or group head at that tile, or U16_KEY_NULL
//    - Tile-stack units share the head's m_x/m_y; group tails use U16_KEY_NULL
//    - Group chains via m_next_unit_in_group are acyclic; U16_KEY_NULL terminates
//
//================================================================================================================================

struct UnitAddStruct {
    u16 m_x; // Tile column; U16_KEY_NULL for group tails
    u16 m_y; // Tile row; U16_KEY_NULL for group tails
    u16 m_player_idx; // Owning seat index
    u16 m_unit_typ_idx; // Index into unit static registry
    u16 m_next_unit_on_tile; // Stack link: next UnitAddKey raw value, or U16_KEY_NULL
    u16 m_next_unit_in_group; // Group link: next UnitAddKey raw value, or U16_KEY_NULL
    
    i16 m_mvt_points; // Remaining movement budget (static mvt_pts x 1000 scale)
    u8 m_health; // Reduced via combat and attrition; increased when healing
    u8 m_level; // Nerf or boost to damage dealt and taken; green, regular, elite, etc
};

#endif // UNIT_ADD_STRUCT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
