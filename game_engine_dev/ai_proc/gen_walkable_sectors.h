//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_WALKABLE_SECTORS_H
#define GEN_WALKABLE_SECTORS_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameArraySimple;
class SectorNetwork;

//================================================================================================================================
//=> - GenWalkableSectors -
//================================================================================================================================
//
//  Same seed centers as SectorNetwork. Multi-source flood over land-walkable tiles only (blocked by
//  water and mountains). Whiteboard stores sector id + 1; 0 is unassigned. Needs 1x Whiteboard_2B.
//
//================================================================================================================================

#define GWS_IDX_NONE 0u

class GenWalkableSectors {
public:
    GenWalkableSectors ();
    ~GenWalkableSectors ();

    bool begin (const SectorNetwork& net, const GameArraySimple& map);
    bool build ();
    void clr ();

    bool ok () const;
    u16 sector_n () const;
    u32 paint_n () const;
    const Whiteboard_2B& sectors () const;

private:
    GenWalkableSectors (const GenWalkableSectors& other) = delete;
    GenWalkableSectors& operator= (const GenWalkableSectors& other) = delete;
    GenWalkableSectors (GenWalkableSectors&& other) = delete;
    GenWalkableSectors& operator= (GenWalkableSectors&& other) = delete;

    const SectorNetwork* m_net; // Non-owning lattice for seed centers
    const GameArraySimple* m_map; // Non-owning terrain
    Whiteboard_2B m_sec; // Sector id + 1 per walkable tile; 0 if none
    u16 m_sec_n; // Sector count from lattice
    u32 m_paint_n; // Walkable tiles assigned
    bool m_ok; // True after begin with live whiteboard
};

#endif // GEN_WALKABLE_SECTORS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
