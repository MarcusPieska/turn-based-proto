//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ADD_STRUCT_H
#define UNIT_ADD_STRUCT_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

#define UNIT_HEALTH 100 // Default unit health at spawn and training

#define VERY_GREEN 0
#define GREEN 1 // Default unit skill level
#define REGULAR 2
#define DISCIPLINED 3
#define HARDENED 4
#define VETERAN 5
#define COMMANDO 6
#define ELITE 7

#define UNIT_DELTA_DEST_NONE (-128) // Per-axis sentinel: no work destination
#define UNIT_DELTA_DEST_ARRIVED 0 // Both axes zero: standing on work tile

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
    u64 m_x : 16; // Tile column; U16_KEY_NULL for group tails
    u64 m_y : 16; // Tile row; U16_KEY_NULL for group tails
    u64 m_unit_typ_idx : 16; // Index into unit static registry
    u64 m_next_unit_on_tile : 16; // Stack link: next UnitAddKey raw value, or U16_KEY_NULL
    //u64 m_in_campaign : 1; // True if unit is in an AI-managed campaign
    //u64 m_unused : 7; // Extra padding to align to 16 bytes, keep these

    u16 m_next_unit_in_group; // Group link: next UnitAddKey raw value, or U16_KEY_NULL
    i16 m_mvt_points; // Remaining movement budget (static mvt_pts x 1000 scale)
    u8 m_player_idx; // Owning seat index
    u8 m_health; // Reduced via combat and attrition; increased when healing; UNIT_HEALTH at full
    u8 m_level; // Nerf or boost to damage dealt and taken; green, regular, elite, etc'
    u8 m_misc; // Only used by worker helper

    i8 m_delta_x_dest; // Work tile dx from m_x; UNIT_DELTA_DEST_NONE or UNIT_DELTA_DEST_ARRIVED
    i8 m_delta_y_dest; // Work tile dy from m_y; UNIT_DELTA_DEST_NONE or UNIT_DELTA_DEST_ARRIVED
    u8 m_in_campaign; // Will be moved up to bit array once we refactor to use a U12_KEY_NULL sentinel
    u8 un_used2;
    u8 un_used3;
    u8 un_used4;
    u8 un_used5;
    u8 un_used6; // Extra padding to align to 16 bytes, keep these
};

inline void unit_add_clr_work_dest (UnitAddStruct* u) {
    if (u == nullptr) {
        return;
    }
    u->m_delta_x_dest = static_cast<i8>(UNIT_DELTA_DEST_NONE);
    u->m_delta_y_dest = static_cast<i8>(UNIT_DELTA_DEST_NONE);
}

#endif // UNIT_ADD_STRUCT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
