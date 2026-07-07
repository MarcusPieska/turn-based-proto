//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_MTN_MK1_H
#define WALK_MTN_MK1_H

#include "game_array_simple.h"
#include "game_map_grid_defs.h"
#include "map_bit_overlay.h"

//================================================================================================================================
//=> - WalkMtnMk1 -
//================================================================================================================================
//
// - Mountain explorer mk1: greedy walk on mtn-adjacent land, then local repos via gradient
// - Greedy phase picks the neighbor that reveals the most unexplored tiles in sight
// - Repos phase flood-fills a 31x31 window toward unexplored mtn-front fog, then descends
// - Function can_ai_start_from: returns 0=no, 1=walk (walk-front + pick), 2=repos (path + grad fog)
// - Designed to not need: STL, heap allocations, or full map overlays
//
//================================================================================================================================
//=> - Header -
//================================================================================================================================

class WalkMtnMk1 {
public:
    static u16 can_ai_start_from (const GameArraySimple& map, MapBitOverlay& exp, u16 x, u16 y, u16 sight);
    
    WalkMtnMk1 (const GameArraySimple& map, MapBitOverlay& exp, u16 sx, u16 sy, u16 sight, u8 player);
    void move (u16 moves);
    u16 x () const;
    u16 y () const;
    u8 phase () const;
    bool done () const;
    bool loc_exh () const;
    u8 grad_max () const;
    bool grad_tile (u16 gi, u16& x, u16& y, u8& d) const;
    bool sink_at (u16 si, u16& x, u16& y) const;

private:
    static const u8 k_ph_walk = 0u; // Greedy mtn walk
    static const u8 k_ph_repos = 1u; // Gradient descent in local window
    static const u16 k_box = 31u; // Local window side length
    static const u16 k_cen = 15u; // Window center index
    static const u16 k_n = 961u; // Total tiles in local window: k_box * k_box
    static const u8 k_blk = 255u; // Blocked in m_dst: outside window or not path; repos never enters
    static const u8 k_non = 254u; // Explored in m_dst: already in overlay; not a fog seed, no descent
    static const u8 k_wat = 253u; // Interior in m_dst: unexplored path off mtn front; BFS fills dist
    const GameArraySimple& m_map; // Terrain and river input

    MapBitOverlay& m_exp; // Shared explore overlay
    MapPt m_pos; // Unit tile
    MapPt m_win; // Repos window anchor tile
    u16 m_sight; // Chebyshev reveal radius
    u8 m_player; // Trace player id
    u8 m_ph; // Current phase
    u8 m_dst[k_n]; // Local distance field for repos
    bool m_done; // No further moves
    bool m_loc_exh; // Repos could not reach fog

    u32 tidx (u16 x, u16 y) const;
    u16 lidx (u16 lx, u16 ly) const;
    bool g2l (u16 gx, u16 gy, u16& lx, u16& ly) const;
    bool l2g (u16 lx, u16 ly, u16& gx, u16& gy) const;
    bool is_mtn (u16 x, u16 y) const;
    bool is_path (u16 x, u16 y) const;
    bool has_mtn_nbr (u16 x, u16 y) const;
    bool is_walk (u16 x, u16 y) const;
    u32 cnt_new (u16 x, u16 y) const;
    bool has_pick () const;
    void reveal_around (u16 x, u16 y);
    bool build_grad ();
    bool step_walk ();
    bool step_repos ();
    bool step ();
};

#endif // WALK_MTN_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
