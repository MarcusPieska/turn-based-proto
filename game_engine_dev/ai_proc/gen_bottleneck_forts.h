//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_BOTTLENECK_FORTS_H
#define GEN_BOTTLENECK_FORTS_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

#include "gen_bottleneck_markers.h"
#include "gen_walkable_sectors.h"
#include "sector_network.h"

class GameArraySimple;

//================================================================================================================================
//=> - GenBottleneckForts -
//================================================================================================================================
//
//  Builds sector network, walkable sectors, and bottleneck marks, then places forts on marked
//  sectors. Fort count is ceil(sector_tiles / ZoC_area). ZoC is 3x3 or 5x5 via GBF_ZOC_5X5 in the
//  .cpp; ZoCs do not overlap. Fort centers never land on resource tiles.
//  Needs WhiteboardMng::init (1x Whiteboard_2B + 2x Whiteboard_1B).
//
//================================================================================================================================

class GenBottleneckForts {
public:
    GenBottleneckForts ();
    ~GenBottleneckForts ();

    bool begin (const GameArraySimple& map);
    bool build ();
    void clr ();

    bool ok () const;
    u32 mark_n () const;
    u32 fort_n () const;
    static i32 zoc_r (); // ZoC half-extent (1 => 3x3, 2 => 5x5)
    const Whiteboard_1B& marks () const;
    const Whiteboard_1B& forts () const;
    const Whiteboard_2B& walk_secs () const;

private:
    GenBottleneckForts (const GenBottleneckForts& other) = delete;
    GenBottleneckForts& operator= (const GenBottleneckForts& other) = delete;
    GenBottleneckForts (GenBottleneckForts&& other) = delete;
    GenBottleneckForts& operator= (GenBottleneckForts&& other) = delete;

    void place_all ();
    u32 place_sec (const u32* tiles, u32 tn, u8* in_sec, u8* zoc);

    const GameArraySimple* m_map; // Non-owning map
    SectorNetwork m_net; // Lattice + land links
    GenWalkableSectors m_walk; // Walkable flood ownership
    GenBottleneckMarkers m_mark; // Bottleneck sector tile marks
    Whiteboard_1B m_fort; // 1 on fort center tiles
    u32 m_fort_n; // Forts placed
    bool m_ok; // True after begin
};

#endif // GEN_BOTTLENECK_FORTS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
