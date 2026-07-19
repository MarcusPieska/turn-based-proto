//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_ARRAY_SIMPLE_H
#define GAME_ARRAY_SIMPLE_H

#include "game_primitives.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - GameTileSimple -
//================================================================================================================================
//
//  One cell in the world grid. Static geography is filled at map load; handles link into GameState pools.
//  Overlays for terrain, climate, river, and improvement are filled at map load.
//  Adds for units, mines, plantations, shipyards, outposts, trade posts, and monasteries are filled at map load.
//
//================================================================================================================================

/* Keep this old struct until we are happy with the new bit field
struct GameTileSimple {
    u16 m_unit_hd; // Unit pool key on this tile; U16_KEY_NULL if empty
    u16 m_add_idx; // Improvement pool key; U16_KEY_NULL if none
    u16 m_res; // Map resource index on this tile; UINT16_MAX if none
    u8 m_terr; // Terrain class id (TERR_* in game_map_defs.h)
    u8 m_clim; // Climate class id (CLIMATE_* in game_map_defs.h)
    u8 m_ov; // Base-map overlay id (OVERLAY_* in game_map_defs.h)
    u8 m_riv; // River flag (0 none, nonzero has river)
    u8 m_add_typ; // Which add vector m_add_idx refers to
};
*/

struct GameTileSimple {

    // Highly volatile fields: First 8 bytes: 16×3 + 8 + 4*2 = 64b <= 64b

    u64 m_unit_hd : 16; // Unit pool key on this tile; U16_KEY_NULL if empty
    u64 m_add_idx : 16; // Improvement pool key; U16_KEY_NULL if none
    u64 m_city_worker : 16; // Currently working on this tile, city pool key; U16_KEY_NULL if none
    u64 m_civ_owner : 8; // Civilization owner pool key; U8_KEY_NULL if none
    u64 m_add_typ : 4; // Which add vector m_add_idx refers to
    u64 m_road_typ : 3; // Road type (ROAD_* in game_map_defs.h)
    u64 m_settler_blocked : 1; // Settler blocked flag (0 none, nonzero is blocked by existing settlements)

    // Almost static fields: Second 8 bytes: 16 + 4×3 + 1 = 29b  <= 64b

    u64 m_res : 16; // Map resource index on this tile; UINT16_MAX if none
    u64 m_terr : 4; // Terrain class id (TERR_* in game_map_defs.h)
    u64 m_clim : 4; // Climate class id (CLIMATE_* in game_map_defs.h)
    u64 m_ov : 4; // Base-map overlay id (OVERLAY_* in game_map_defs.h)
    u64 m_riv : 1; // River flag (0 none, nonzero has river)
};

//================================================================================================================================
//=> - GameArraySimple -
//================================================================================================================================
//
//  Dense w x h tile grid. Embedded in GameState as the canonical map.
//  Populated by Factory_GameArraySimple at new-game load; restored by GameSetup when loading a save.
//  Read each turn by GameLoop, AI pathing, and systems that need per-tile terrain or handles.
//
//================================================================================================================================

class Factory_GameArraySimple;
class GameLoopCache;

class GameArraySimple {
public:
    GameArraySimple ();
    ~GameArraySimple ();
    void clear (); // Free tile storage; map becomes 0 x 0
    u16 width () const; // Grid width in tiles
    u16 height () const; // Grid height in tiles
    u32 tile_n () const; // Tile count: width * height

    u8 get_terrain (u16 x, u16 y) const; // Terrain class at tile
    u8 get_climate (u16 x, u16 y) const; // Climate class at tile
    u8 get_overlay (u16 x, u16 y) const; // Base overlay at tile
    u8 get_river (u16 x, u16 y) const; // River flag at tile
    u16 get_unit_hd (u16 x, u16 y) const;// Unit handle at tile
    u16 get_add_idx (u16 x, u16 y) const;// Improvement handle at tile
    u8 get_add_typ (u16 x, u16 y) const; // Improvement type tag at tile
    u16 get_res (u16 x, u16 y) const;  // Resource index at tile
    u16 get_city_worker (u16 x, u16 y) const; // City pool key working this tile; U16_KEY_NULL if none
    u8 get_civ_owner (u16 x, u16 y) const; // Civ/seat owner at tile; U8_KEY_NULL if none
    u8 get_settler_blocked (u16 x, u16 y) const; // 0 free, nonzero blocked for settling
    
    bool set_unit_hd (u16 x, u16 y, u16 unit_hd); // Unit handle at tile; U16_KEY_NULL clears
    bool set_tile_add (u16 x, u16 y, u16 add_idx, u8 add_typ); // Improvement handle at tile
    bool set_city_worker (u16 x, u16 y, u16 city_idx); // City worker key at tile; U16_KEY_NULL clears
    bool set_civ_owner (u16 x, u16 y, u8 owner); // Civ/seat owner at tile; U8_KEY_NULL clears
    bool set_settler_blocked (u16 x, u16 y, u8 blocked); // Settler block flag; 0 clears

private:
    friend class Factory_GameArraySimple;
    friend class GameLoopCache;
    GameArraySimple (const GameArraySimple& other) = delete;
    GameArraySimple& operator= (const GameArraySimple& other) = delete;
    GameArraySimple (GameArraySimple&& other) = delete;
    GameArraySimple& operator= (GameArraySimple&& other) = delete;
    u32 tidx (u16 x, u16 y) const;

    u16 m_w; // Grid width
    u16 m_h; // Grid height
    GameTileSimple* m_tiles; // Row-major tile array; length m_w * m_h
};

#endif // GAME_ARRAY_SIMPLE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
