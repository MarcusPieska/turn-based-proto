//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RIVER_EXPLORE_MK1_H
#define RIVER_EXPLORE_MK1_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"
#include "path_mk1.h"
#include "river_pathing.h"

//================================================================================================================================
//=> - RiverExploreMk1 -
//================================================================================================================================

class RiverExploreMk1 {
public:
    RiverExploreMk1 (
        const GameArraySimple& map,
        MapBitOverlay& explored,
        u16 sx,
        u16 sy,
        u16 sight,
        u8 player);
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
    static const u8 k_ph_greedy = 1u;
    static const u8 k_ph_path = 2u;
    static const u8 k_ph_done = 3u;
    const GameArraySimple& m_map;
    MapBitOverlay& m_exp;
    u16 m_x;
    u16 m_y;
    u16 m_sx;
    u16 m_sy;
    u16 m_sight;
    u8 m_player;
    u8 m_ph;
    u16 m_path_i;
    bool m_greedy_ph;
    bool m_has_blk;
    u16 m_blk_x;
    u16 m_blk_y;
    PathMk1 m_path;
    PathMk1 m_mk_gre_end;
    PathMk1 m_mk_gre_start;
    void reveal_around ();
    void reveal_delta (u16 ox, u16 oy, u16 nx, u16 ny);
    void end_greedy_local ();
    void end_all ();
    void start_greedy_ph ();
    bool step_greedy ();
    bool step_path ();
};

#endif // RIVER_EXPLORE_MK1_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
