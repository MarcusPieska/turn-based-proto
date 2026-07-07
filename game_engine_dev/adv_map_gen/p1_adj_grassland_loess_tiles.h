//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_GRASSLAND_LOESS_TILES_H
#define P1_ADJ_GRASSLAND_LOESS_TILES_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Adj_GrasslandLoessTilesPrm -
//================================================================================================================================

struct P1_Adj_GrasslandLoessTilesPrm {
    u16 m_w_rain;
    u8 m_land_pct;
};

static inline P1_Adj_GrasslandLoessTilesPrm p1_adj_grassland_loess_tiles_prm_def () {
    P1_Adj_GrasslandLoessTilesPrm p;
    p.m_w_rain = 50;
    p.m_land_pct = 5;
    return p;
}

//================================================================================================================================
//=> - P1_Adj_GrasslandLoessTiles -
//================================================================================================================================
//
//  Rain-weighted loess minus rain; inner then outer grass queue; top land pct get black soil climate.
//
//================================================================================================================================

class P1_Adj_GrasslandLoessTiles {
public:
    explicit P1_Adj_GrasslandLoessTiles (
        const P1_RunPrm& prm,
        const P1_Adj_GrasslandLoessTilesPrm& sp = p1_adj_grassland_loess_tiles_prm_def ());

    bool adjust (
        u8* terrain,
        u8* climate,
        const u8* loess,
        const u8* rain,
        u16 w,
        u16 h);
    bool is_valid () const;
    u32 picked_n () const;

private:
    P1_Adj_GrasslandLoessTiles (const P1_Adj_GrasslandLoessTiles& other) = delete;
    P1_Adj_GrasslandLoessTiles (P1_Adj_GrasslandLoessTiles&& other) = delete;

    P1_RunPrm m_prm;
    P1_Adj_GrasslandLoessTilesPrm m_sp;
    bool m_valid_adjust;
    u32 m_pick_n;
};

#endif // P1_ADJ_GRASSLAND_LOESS_TILES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
