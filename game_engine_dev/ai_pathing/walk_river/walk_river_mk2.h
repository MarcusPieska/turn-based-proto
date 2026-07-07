//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_RIVER_MK2_H
#define WALK_RIVER_MK2_H

#include "game_array_simple.h"
#include "game_map_grid_defs.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"

//================================================================================================================================
//=> - WalkRiverMk2 -
//================================================================================================================================
//
// River explorer mk2: walk on river tiles with junction branch peek and backtrack.
//
// Phases (phase()):
//   1 greedy  Walk on river; pick first valid river-front neighbor (4-neigh in greedy)
//   2 back    Pop history ring after greedy stalls; retry branch peek or resume greedy
//   3 done    No further moves
//   4 repos   Land entry: 15x15 grad toward river-front fog, then greedy
//
// can_ai_start_from: 0=no, 1=on river with front pick, 2=land repos (grad to river-front fog)
//
// Memory: m_hist union — u8 dst[225] during repos; MapPt pt[k_ring] during greedy/back (128 slots).
// No heap, no full-map overlay. ~600B instance (platform-dependent).
//
//================================================================================================================================
//=> - Header -
//================================================================================================================================

class WalkRiverMk2 {
public:
    static u16 can_ai_start_from (const GameArraySimple& map, MapBitOverlay& exp, u16 x, u16 y, u16 sight);

    WalkRiverMk2 (const GameArraySimple& map, MapBitOverlay& explored, u16 sx, u16 sy, u16 sight, u8 player);
    void move (u16 moves);
    void reveal ();
    bool pump ();
    void finish_move_credit (u16 loop, bool acted);
    bool greedy_ph () const;
    u16 x () const;
    u16 y () const;
    u8 phase () const;

private:
    static const u8 k_ph_greedy = 1u; // Greedy river walk
    static const u8 k_ph_back = 2u; // Backtrack along history
    static const u8 k_ph_done = 3u; // No further moves
    static const u8 k_ph_repos = 4u; // Entry-only grad toward river front
    static const u16 k_box = 15u; // Local window side length
    static const u16 k_cen = 7u; // Window center index
    static const u16 k_grad = 225u; // k_box * k_box; repos dist field
    static const u16 k_ring = 128u; // Greedy/back history ring capacity
    static const u16 k_pump = 128u; // Max micro-steps per move credit
    static const u16 k_q = 8u; // Backtrack revisit queue depth
    static const u16 k_pkq = 12u; // Branch peek BFS tile cap
    static const u8 k_blk = 255u; // Blocked in dist field
    static const u8 k_non = 254u; // Explored in dist field
    static const u8 k_wat = 253u; // Interior in dist field
    const GameArraySimple& m_map; // Terrain and river input

    MapBitOverlay& m_exp; // Shared explore overlay per player
    MapPt m_pos; // Unit tile
    MapPt m_win; // Repos window anchor tile
    u16 m_sight; // Chebyshev reveal radius
    u8 m_player; // Trace player id
    u8 m_ph; // Current phase
    bool m_greedy_ph; // In an active greedy segment
    bool m_has_blk; // Blocked tile pending at junction
    MapPt m_blk; // Blocked junction tile
    union {
        MapPt pt[k_ring]; // History ring when greedy/back
        u8 dst[k_grad]; // Distance field during entry repos
    } m_hist;
    u16 m_hn; // History count
    u16 m_hp; // History write head
    MapPt m_bck[k_q]; // Backtrack revisit queue
    u16 m_bck_n; // Backtrack queue count
    u16 m_bck_t; // Backtrack queue write head

    void hist_push (MapPt p);
    bool hist_pop (MapPt& p);
    void bck_clr ();
    void bck_push (MapPt p);
    bool bck_has (MapPt p) const;
    bool hist8_has (MapPt p) const;
    bool blk_peek (MapPt p) const;
    void reveal_around ();
    void reveal_delta (MapPt o, MapPt n);
    u16 lidx (u16 lx, u16 ly) const;
    bool g2l (u16 gx, u16 gy, u16& lx, u16& ly) const;
    bool l2g (u16 lx, u16 ly, u16& gx, u16& gy) const;
    bool is_riv_front (u16 x, u16 y) const;
    bool pick_near_front (u16 sx, u16 sy, u16& ox, u16& oy) const;
    bool peek_branch (u16 jx, u16 jy, u16 sx, u16 sy) const;
    bool try_peek_branch (bool cont);
    void end_greedy_local ();
    void end_all ();
    void start_greedy_ph ();
    bool try_resume_greedy ();
    bool step_repos ();
    bool step_greedy ();
    bool step_back ();
};

#endif // WALK_RIVER_MK2_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
