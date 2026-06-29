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

struct GameTileSimple {
    u16 m_unit_hd;
    u16 m_add_idx;
    u16 m_res;
    u8 m_terr;
    u8 m_clim;
    u8 m_ov;
    u8 m_riv;
    u8 m_add_typ;
};

//================================================================================================================================
//=> - GameArraySimple -
//================================================================================================================================

class Factory_GameArraySimple;

class GameArraySimple {
public:
    GameArraySimple ();
    ~GameArraySimple ();
    void clear ();
    u16 width () const;
    u16 height () const;
    u32 tile_n () const;

    u8 get_terrain (u16 x, u16 y) const;
    u8 get_climate (u16 x, u16 y) const;
    u8 get_overlay (u16 x, u16 y) const;
    u8 get_river (u16 x, u16 y) const;
    u16 get_unit_hd (u16 x, u16 y) const;
    u16 get_add_idx (u16 x, u16 y) const;
    u8 get_add_typ (u16 x, u16 y) const;
    u16 get_res (u16 x, u16 y) const;

private:
    friend class Factory_GameArraySimple;
    GameArraySimple (const GameArraySimple& other) = delete;
    GameArraySimple& operator= (const GameArraySimple& other) = delete;
    GameArraySimple (GameArraySimple&& other) = delete;
    GameArraySimple& operator= (GameArraySimple&& other) = delete;
    u32 tidx (u16 x, u16 y) const;
    u16 m_w;
    u16 m_h;
    GameTileSimple* m_tiles;
};

#endif // GAME_ARRAY_SIMPLE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
