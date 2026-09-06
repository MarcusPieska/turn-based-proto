//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_WATERSHED_H
#define GEN_WATERSHED_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenWatershed -
//================================================================================================================================
//
//  From a seed river tile, marks the connected river system (4-adjacent cardinal only) on an overlay.
//  Ocean/sea/coastal always block. Inland seas/lakes are sized; bodies larger than k_lake_blk (20)
//  block, smaller ones are crossable and painted. Step 2 (later): full watershed. Needs WhiteboardMng
//  sized to the map. Overlay 0 = unset; nonzero = in this river system.
//
//================================================================================================================================

class GenWatershed {
public:
    GenWatershed ();
    ~GenWatershed ();

    bool begin (const GameArraySimple& map);
    void clr ();

    u32 fill_rivers (u16 sx, u16 sy);

    bool ok () const;
    u32 river_n () const;
    const Whiteboard_1B& overlay () const;

private:
    GenWatershed (const GenWatershed& other) = delete;
    GenWatershed& operator= (const GenWatershed& other) = delete;
    GenWatershed (GenWatershed&& other) = delete;
    GenWatershed& operator= (GenWatershed&& other) = delete;

    const GameArraySimple* m_map; // Non-owning map
    Whiteboard_1B m_ov; // River-system mark per tile
    u32 m_riv_n; // Tiles marked in last fill_rivers
    bool m_ok; // True after successful begin
};

#endif // GEN_WATERSHED_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
