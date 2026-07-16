//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_WORKING_H
#define TILE_WORKING_H

#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - TileWorking -
//================================================================================================================================
//
//  Static mutation of per-tile city worker keys on the bound GameArraySimple map. bind_map wires the active match grid.
//  clear_worked drops one tile when it is assigned to the given city.
//
//================================================================================================================================

class TileWorking {
public:
    static void bind_map (GameArraySimple* map);
    static bool in_bounds (u16 x, u16 y);
    static u16 get_worker (u16 x, u16 y);
    static bool mark_worked (u16 x, u16 y, u16 city_idx);
    static void clear_worked (u16 x, u16 y, u16 city_idx);

private:
    static GameArraySimple* m_map; // Active match tile grid; null until bind_map
};

#endif // TILE_WORKING_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
