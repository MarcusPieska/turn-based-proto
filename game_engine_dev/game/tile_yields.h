//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_YIELDS_H
#define TILE_YIELDS_H

#include "game_primitives.h"

class GameArraySimple;

//================================================================================================================================
//=> - TileYield -
//================================================================================================================================
//
//  Per-tile production yields returned by TileYields::get. Placeholder fields until game data drives yields.
//
//================================================================================================================================

struct TileYield {
    u8 m_food; // Food yield on this tile
    u8 m_production; // Production yield on this tile
    u8 m_commerce; // Commerce yield on this tile
};

//================================================================================================================================
//=> - TileYields -
//================================================================================================================================
//
//  Static read-only lookup of tile yields from the bound GameArraySimple map. bind_map wires the active match tile grid.
//  Internals are temporary hard-coded rules; later replaced by game data tables.
//
//================================================================================================================================

class TileYields {
public:
    static void bind_map (const GameArraySimple* map);
    static TileYield get (u16 x, u16 y);
    static bool in_bounds (u16 x, u16 y);

private:
    static const GameArraySimple* m_map; // Active match tile grid; null until bind_map
};

#endif // TILE_YIELDS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
