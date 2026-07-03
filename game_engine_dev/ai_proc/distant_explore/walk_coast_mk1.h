//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WALK_COAST_MK1_H
#define WALK_COAST_MK1_H

#include "game_array_simple.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - WalkCoastMk1 -
//================================================================================================================================

class WalkCoastMk1 {
public:
    WalkCoastMk1 (
        const GameArraySimple& map,
        u8* exp,
        u16 sx,
        u16 sy,
        u16 sight,
        u8 player);
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
    static const u8 k_ph_walk = 0u;
    static const u8 k_ph_repos = 1u;
    static const u16 k_box = 31u;
    static const u16 k_cen = 15u;
    static const u16 k_n = 961u;
    static const u8 k_blk = 255u;
    static const u8 k_non = 254u;
    static const u8 k_wat = 253u;
    const GameArraySimple& m_map;
    u8* m_exp;
    u16 m_x;
    u16 m_y;
    u16 m_ax;
    u16 m_ay;
    u16 m_sight;
    u8 m_player;
    u8 m_ph;
    u8 m_dst[k_n];
    bool m_done;
    bool m_loc_exh;
    u32 tidx (u16 x, u16 y) const;
    u16 lidx (u16 lx, u16 ly) const;
    bool g2l (u16 gx, u16 gy, u16& lx, u16& ly) const;
    bool l2g (u16 lx, u16 ly, u16& gx, u16& gy) const;
    bool is_coast (u16 x, u16 y) const;
    bool is_path (u16 x, u16 y) const;
    bool has_coast_nbr (u16 x, u16 y) const;
    bool is_walk (u16 x, u16 y) const;
    u32 cnt_new (u16 x, u16 y) const;
    bool has_pick () const;
    void reveal_around (u16 x, u16 y);
    bool build_grad ();
    bool step_walk ();
    bool step_repos ();
    bool step ();
};

#endif // WALK_COAST_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
