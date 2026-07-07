//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_RIVER_MK1_H
#define WALK_RIVER_MK1_H

#include "game_array_simple.h"
#include "game_map_grid_defs.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"
#include "path_mk1.h"
#include "river_pathing.h"

//================================================================================================================================
//=> - WalkRiverMk1 -
//================================================================================================================================
//
// - River walker mk1: greedy walk on river tiles, then path jump to next river front
// - Greedy phase picks the neighbor that reveals the most unexplored tiles in sight
// - Path phase uses RiverPathing to reach the nearest unexplored river-front tile
// - can_ai_start_from: 0=no, 1=greedy (on river + pick), 2=repos (land path to river)
// - Designed to not need: STL, heap allocations, or full map overlays
//
//================================================================================================================================
//=> - Header -
//================================================================================================================================

class WalkRiverMk1 {
public:
    static u16 can_ai_start_from (
        const GameArraySimple& map,
        MapBitOverlay& exp,
        u16 x,
        u16 y,
        u16 sight);

    WalkRiverMk1 (const GameArraySimple& map, MapBitOverlay& explored, u16 sx, u16 sy, u16 sight, u8 player);
    void move (u16 moves);
    u16 x () const;
    u16 y () const;
    u8 phase () const;
    u16 path_n () const;
    u16 path_i () const;
    u16 wp_x (u16 i) const;
    u16 wp_y (u16 i) const;
    u16 gre_end_n () const;
    u16 gre_end_x (u16 i) const;
    u16 gre_end_y (u16 i) const;
    u16 gre_start_n () const;
    u16 gre_start_x (u16 i) const;
    u16 gre_start_y (u16 i) const;

private:
    static const u8 k_ph_greedy = 1u; // Greedy river walk
    static const u8 k_ph_path = 2u; // Follow path to river front
    static const u8 k_ph_done = 3u; // No further moves
    const GameArraySimple& m_map; // Terrain and river input
    
    MapBitOverlay& m_exp; // Shared explore overlay per player
    MapPt m_pos; // Unit tile
    MapPt m_sp; // Spawn tile
    u16 m_sight; // Chebyshev reveal radius
    u8 m_player; // Trace player id
    u8 m_ph; // Current phase
    u16 m_path_i; // Index along m_path
    bool m_greedy_ph; // In an active greedy segment
    bool m_has_blk; // Blocked tile pending at junction
    MapPt m_blk; // Blocked junction tile
    bool m_land_approach; // Entry path from land before first greedy segment
    PathMk1 m_path; // Front-jump path from RiverPathing
    PathMk1 m_mk_gre_end; // Greedy segment end markers
    PathMk1 m_mk_gre_start; // Greedy segment start markers

    void reveal_around ();
    void reveal_delta (MapPt o, MapPt n);
    void end_greedy_local ();
    void end_all ();
    void start_greedy_ph ();
    bool step_greedy ();
    bool step_path ();
};

#endif // WALK_RIVER_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
