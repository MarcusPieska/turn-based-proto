//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_NEAR_MK1_H
#define WALK_NEAR_MK1_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"

//================================================================================================================================
//=> - WalkNearBias -
//================================================================================================================================

enum WalkNearBias : u8 {
    WN_BIAS_NONE = 0,
    WN_BIAS_NORTH = 1,
    WN_BIAS_SOUTH = 2,
    WN_BIAS_EAST = 3,
    WN_BIAS_WEST = 4
};

//================================================================================================================================
//=> - WalkNearMk1 -
//================================================================================================================================
//
// - Near explorer mk1: greedy fog reveal on walkable land, then local repos via gradient
// - Explore phase picks the 8-neighbor that reveals the most unexplored tiles in sight
// - Repos phase flood-fills a 21x21 window toward fog, then descends
// - Optional directional bias from home (north/south/east/west)
// - River spot tracking for handoff to path/river walkers
// - can_ai_start_from: 0=no, 1=explore (8-neigh fog pick), 2=repos (local grad to fog)
// - Designed to not need: STL, heap allocations, or full map overlays
//
//================================================================================================================================
//=> - Header -
//================================================================================================================================

class WalkNearMk1 {
public:
    static u16 can_ai_start_from (const GameArraySimple& map, MapBitOverlay& exp, u16 x, u16 y, u16 sight);

    WalkNearMk1 (const GameArraySimple& map, MapBitOverlay& exp, u16 sx, u16 sy, u16 sight, u8 player, WalkNearBias bias);

    void move (u16 moves);
    u16 x () const;
    u16 y () const;
    u16 hx () const;
    u16 hy () const;
    u8 phase () const;
    bool done () const;
    bool loc_exh () const;
    bool riv_spot () const;
    u16 riv_x () const;
    u16 riv_y () const;
    void clr_riv_spot ();

private:
    static const u8 k_ph_explore = 0u; // Greedy fog reveal
    static const u8 k_ph_repos = 1u; // Gradient descent in local window
    static const u16 k_l = 4u; // Score cap for fog count
    static const u16 k_box = 21u; // Local window side length
    static const u16 k_cen = 10u; // Window center index
    static const u16 k_n = 441u; // Total tiles in local window
    static const u8 k_blk = 255u; // Blocked in m_dst
    static const u8 k_non = 254u; // Explored in m_dst
    const GameArraySimple& m_map; // Terrain and river input

    MapBitOverlay& m_exp; // Shared explore overlay
    u16 m_x; // Unit tile
    u16 m_y;
    u16 m_hx; // Home tile
    u16 m_hy;
    u16 m_ax; // Repos window anchor
    u16 m_ay;
    u16 m_sight; // Chebyshev reveal radius
    u8 m_player; // Trace player id
    u8 m_bias; // Directional bias from home
    u8 m_ph; // Current phase
    u8 m_dst[k_n]; // Local distance field for repos
    bool m_done; // No further moves
    bool m_loc_exh; // Repos could not reach fog
    bool m_riv_spot; // River tile spotted this step
    u16 m_riv_x; // Nearest spotted river tile
    u16 m_riv_y;

    u32 tidx (u16 x, u16 y) const;
    u16 lidx (u16 lx, u16 ly) const;
    bool g2l (u16 gx, u16 gy, u16& lx, u16& ly) const;
    bool l2g (u16 lx, u16 ly, u16& gx, u16& gy) const;
    bool is_walk (u16 x, u16 y) const;
    bool is_riv (u16 x, u16 y) const;
    void note_riv (u16 ax, u16 ay);
    u32 cheb (u16 x0, u16 y0, u16 x1, u16 y1) const;
    u32 adj_dist (u16 x, u16 y) const;
    u32 cnt_new (u16 x, u16 y) const;
    bool has_pick () const;
    i32 score (u16 x, u16 y) const;
    void reveal_around (u16 x, u16 y);
    bool build_grad ();
    bool step_explore ();
    bool step_repos ();
    bool step ();
};

#endif // WALK_NEAR_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
