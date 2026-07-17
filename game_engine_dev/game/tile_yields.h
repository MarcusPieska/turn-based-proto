//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TILE_YIELDS_H
#define TILE_YIELDS_H

#include "game_primitives.h"

class GameArraySimple;
class RuntimeStatics;

//================================================================================================================================
//=> - TileYield -
//================================================================================================================================
//
//  Per-tile production yields returned by TileYields::get from TileAttrTables (terr+clim+ov, plus riv if present).
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
//  Static read-only yield lookup on the bound GameArraySimple map. setup unpacks RuntimeStatics into TileAttrTables;
//  get indexes those tables by map cell class ids (no name scan on the hot path).
//
//================================================================================================================================

class TileYields {
public:
    static bool setup (const RuntimeStatics& st);
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
