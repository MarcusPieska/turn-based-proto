//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_BOTTLENECK_MARKERS_H
#define GEN_BOTTLENECK_MARKERS_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameArraySimple;
class GenWalkableSectors;
class SectorNetwork;

//================================================================================================================================
//=> - GenBottleneckMarkers -
//================================================================================================================================
//
//  Mark land sectors that look like bottlenecks on the sector lattice: land-gaps form at least
//  two separate clusters on the hex neighbor ring. Paints walkable tiles owned by those sectors
//  (from GenWalkableSectors). Needs 1x Whiteboard_1B.
//
//================================================================================================================================

class GenBottleneckMarkers {
public:
    GenBottleneckMarkers ();
    ~GenBottleneckMarkers ();

    bool begin (const SectorNetwork& net, const GenWalkableSectors& walk, const GameArraySimple& map);
    bool mark ();
    void clr ();

    bool ok () const;
    u32 mark_n () const;
    const Whiteboard_1B& marks () const;

private:
    GenBottleneckMarkers (const GenBottleneckMarkers& other) = delete;
    GenBottleneckMarkers& operator= (const GenBottleneckMarkers& other) = delete;
    GenBottleneckMarkers (GenBottleneckMarkers&& other) = delete;
    GenBottleneckMarkers& operator= (GenBottleneckMarkers&& other) = delete;

    const SectorNetwork* m_net; // Non-owning lattice for gap checks
    const GenWalkableSectors* m_walk; // Non-owning walkable sector ownership
    const GameArraySimple* m_map; // Non-owning map for land-center checks
    Whiteboard_1B m_mark; // 1 on walkable tiles in bottleneck sectors
    u32 m_mark_n; // Marked sector count
    bool m_ok; // True after begin with live whiteboard
};

#endif // GEN_BOTTLENECK_MARKERS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
