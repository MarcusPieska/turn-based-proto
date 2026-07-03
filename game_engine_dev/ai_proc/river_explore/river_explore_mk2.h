//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RIVER_EXPLORE_MK2_H
#define RIVER_EXPLORE_MK2_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"

//================================================================================================================================
//=> - RiverExploreMk2 -
//================================================================================================================================

class RiverExploreMk2 {
public:
    RiverExploreMk2 (const GameArraySimple& map, MapBitOverlay& explored, u16 sx, u16 sy, u16 sight, u8 player);
    void move (u16 moves);
    u16 x () const;
    u16 y () const;
    u8 phase () const;
    u16 gre_end_n () const;
    u16 gre_end_x (u16 i) const;
    u16 gre_end_y (u16 i) const;
    u16 gre_start_n () const;
    u16 gre_start_x (u16 i) const;
    u16 gre_start_y (u16 i) const;

private:
    static const u8 k_ph_greedy = 1u;
    static const u8 k_ph_back = 2u;
    static const u8 k_ph_done = 3u;
    static const u16 k_hist = 200u;
    static const u16 k_q = 8u;
    static const u16 k_pkq = 12u;
    static const u16 k_mk = 64u;
    const GameArraySimple& m_map;
    MapBitOverlay& m_exp;
    u16 m_x;
    u16 m_y;
    u16 m_sx;
    u16 m_sy;
    u16 m_sight;
    u8 m_player;
    u8 m_ph;
    bool m_greedy_ph;
    bool m_has_blk;
    u16 m_blk_x;
    u16 m_blk_y;
    u16 m_hx[k_hist];
    u16 m_hy[k_hist];
    u16 m_hn;
    u16 m_hp;
    u16 m_bck_x[k_q];
    u16 m_bck_y[k_q];
    u16 m_bck_n;
    u16 m_bck_t;
    u16 m_gsx[k_mk];
    u16 m_gsy[k_mk];
    u16 m_gs_n;
    u16 m_gex[k_mk];
    u16 m_gey[k_mk];
    u16 m_ge_n;
    void gs_push (u16 x, u16 y);
    void ge_push (u16 x, u16 y);
    void hist_push (u16 x, u16 y);
    bool hist_pop (u16& x, u16& y);
    void bck_clr ();
    void bck_push (u16 x, u16 y);
    bool bck_has (u16 x, u16 y) const;
    bool hist8_has (u16 x, u16 y) const;
    bool blk_peek (u16 x, u16 y) const;
    void reveal_around ();
    void reveal_delta (u16 ox, u16 oy, u16 nx, u16 ny);
    bool is_riv (u16 x, u16 y) const;
    bool has_unexp_chb (u16 x, u16 y) const;
    bool is_riv_front (u16 x, u16 y) const;
    bool has_pick (u16 x, u16 y) const;
    bool pick_near_front (u16 sx, u16 sy, u16& ox, u16& oy) const;
    bool peek_branch (u16 jx, u16 jy, u16 sx, u16 sy) const;
    bool try_peek_branch (bool cont);
    void end_greedy_local ();
    void end_all ();
    void start_greedy_ph ();
    bool try_resume_greedy ();
    bool step_greedy ();
    bool step_back ();
};

#endif // RIVER_EXPLORE_MK2_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
