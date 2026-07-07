//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RIVER_PATHING_H
#define RIVER_PATHING_H

#include "game_array_simple.h"
#include "game_primitives.h"
#include "map_bit_overlay.h"
#include "path_mk1.h"

//================================================================================================================================
//=> - RiverPathing -
//================================================================================================================================

class RiverPathing {
public:
    static bool is_riv (const GameArraySimple& map, u16 x, u16 y);
    static bool has_unexp_chb (
        const GameArraySimple& map,
        const MapBitOverlay& exp,
        u16 x,
        u16 y,
        u16 sight);
    static bool is_riv_front (
        const GameArraySimple& map,
        const MapBitOverlay& exp,
        u16 x,
        u16 y,
        u16 sight);
    static bool pick_near_front (
        const GameArraySimple& map,
        const MapBitOverlay& exp,
        u16 sx,
        u16 sy,
        u16 sight,
        u16& ox,
        u16& oy,
        u16 skx,
        u16 sky,
        bool has_sk);
    static bool find_path_to_front (
        const GameArraySimple& map,
        const MapBitOverlay& exp,
        u16 sx,
        u16 sy,
        u16 sight,
        PathMk1& path);
    static bool find_land_path_to_river (
        const GameArraySimple& map,
        const MapBitOverlay& exp,
        u16 sx,
        u16 sy,
        u16 sight,
        PathMk1& path);
};

#endif // RIVER_PATHING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
