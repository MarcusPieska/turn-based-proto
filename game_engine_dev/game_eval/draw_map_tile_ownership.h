//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DRAW_MAP_TILE_OWNERSHIP_H
#define DRAW_MAP_TILE_OWNERSHIP_H

#include "game_primitives.h"

//================================================================================================================================
//=> - DrawMapTileOwnership -
//================================================================================================================================
//
//  Seat -> palette pair + diagonal direction. Stripes alternate light(a) / dark(b); direction is
//  either x+y or x-y. Solid borders (k_border) use light(a) near none/other. No-owner is white.
//
//================================================================================================================================

class DrawMapTileOwnership {
public:
    static const u8 k_stripe = 3u; // Diagonal band width in px
    static const u8 k_border = 1u; // Ownership edge width in px
    static const u8 k_none_r = 255u; // No-owner overlay red
    static const u8 k_none_g = 255u; // No-owner overlay green
    static const u8 k_none_b = 255u; // No-owner overlay blue

    DrawMapTileOwnership () = delete;

    static void rgb (u16 seat, u16 x, u16 y, u8* out);
    static void at (const u8* seats, u16 w, u16 h, u16 x, u16 y, u8* out);
    static bool brd (const u8* seats, u16 w, u16 h, u16 x, u16 y);
    static void blend (u8* dst, u16 w, u16 h, u16 x, u16 y, const u8* seats);
    static bool fill (u8* ov, u16 w, u16 h, const u8* seats);

private:
    static const u8 k_pal[][3];
    static const u16 k_pal_n;

    static void sch (u16 seat, u16* a, u16* b, u8* dir);
    static void lit (const u8* c, u8* out);
    static void dim (const u8* c, u8* out);
    static bool edge (const u8* seats, u16 w, u16 h, u16 x, u16 y, u8 seat);
};

#endif // DRAW_MAP_TILE_OWNERSHIP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
