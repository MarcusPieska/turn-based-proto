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
    bool set_unit_hd (u16 x, u16 y, u16 unit_hd); // Unit handle at tile; U16_KEY_NULL clears

private:
    friend class Factory_GameArraySimple;
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
