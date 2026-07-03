//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef NEAR_EXPLORE_MK1_H
#define NEAR_EXPLORE_MK1_H

#include "game_array_simple.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - NearExploreBias -
//================================================================================================================================

enum NearExploreBias : u8 {
    NE_BIAS_NONE = 0,
    NE_BIAS_NORTH = 1,
    NE_BIAS_SOUTH = 2,
    NE_BIAS_EAST = 3,
    NE_BIAS_WEST = 4
};

//================================================================================================================================
//=> - NearExploreMk1 -
//================================================================================================================================

class NearExploreMk1 {
public:
    NearExploreMk1 (
        const GameArraySimple& map,
        u8* exp,
        u16 sx,
        u16 sy,
        u16 sight,
        u8 player,
        NearExploreBias bias);
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
    static const u8 k_ph_explore = 0u;
    static const u8 k_ph_repos = 1u;
    static const u16 k_l = 4u;
    static const u16 k_box = 21u;
    static const u16 k_cen = 10u;
    static const u16 k_n = 441u;
    static const u8 k_blk = 255u;
    static const u8 k_non = 254u;
    const GameArraySimple& m_map;
    u8* m_exp;
    u16 m_x;
    u16 m_y;
    u16 m_hx;
    u16 m_hy;
    u16 m_ax;
    u16 m_ay;
    u16 m_sight;
    u8 m_player;
    u8 m_bias;
    u8 m_ph;
    u8 m_dst[k_n];
    bool m_done;
    bool m_loc_exh;
    bool m_riv_spot;
    u16 m_riv_x;
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

#endif // NEAR_EXPLORE_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
