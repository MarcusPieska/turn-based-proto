//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_TURN_HANDLER_H
#define RESOURCE_TURN_HANDLER_H

#include "game_primitives.h"

class BitArrayCL;
class GameArraySimple;
class GameState;
class GeneralBitBank;
class RuntimeStatics;
struct GameTileSimple;

//================================================================================================================================
//=> - ResourceExtractCtx -
//================================================================================================================================
//
//  Per owned resource tile: assessor gates extract via catalog reqs; effector applies
//  RESOURCE boosters (tech/bld/wonder/imp) on base yield.
//
//================================================================================================================================

struct ResourceExtractCtx {
    u16 m_owner; // Seat that owns the tile
    u16 m_x; // Tile column
    u16 m_y; // Tile row
    u16 m_res_idx; // Catalog resource on this tile
    u16 m_city_idx; // Optional city batch; U16_KEY_NULL when none
    const GameTileSimple* m_tile; // Live map cell
    const BitArrayCL* m_tech; // Owner researched-tech bitset
    const GeneralBitBank* m_flag_bank; // City toggle-flag bank
    const GeneralBitBank* m_bld_bank; // Per-city building flags
    const u16* m_wonder_city; // Global wonder city row
    u16 m_wonder_n; // Wonder catalog count
    const u16* m_small_wonder_city; // Seat small-wonder city row
    u16 m_small_wonder_n; // Small-wonder catalog count
    const RuntimeStatics* m_statics; // Bound catalogs
    const GameArraySimple* m_map; // Live map for tile reqs
};

//================================================================================================================================
//=> - ResourceTurnHandler -
//================================================================================================================================
//
//  setup scans the map once into a dense resource-tile coord table and builds ResourceEffector.
//  handle walks that table; extract_amt runs ResourceAssessor then ResourceEffector.
//
//================================================================================================================================

class ResourceTurnHandler {
public:
    ResourceTurnHandler () = delete;

    static bool setup (GameState& state);
    static void clear ();
    static void handle (GameState& state);
    static u32 coord_n ();

private:
    struct ResCoord {
        u16 m_x; // Map column
        u16 m_y; // Map row
        u16 m_res; // Catalog resource index
    };

    static ResCoord* m_coord;
    static u32 m_coord_n;

    static u16 extract_amt (const ResourceExtractCtx& ctx);
};

#endif // RESOURCE_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
