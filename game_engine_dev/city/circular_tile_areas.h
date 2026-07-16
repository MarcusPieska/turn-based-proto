//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIRCULAR_TILE_AREAS_H
#define CIRCULAR_TILE_AREAS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - CircArea -
//================================================================================================================================
//
//  View into a circular tile-offset table for one radius. m_lim is the entry count; m_brd points at {dx,dy} pairs.
//  Returned by copy from CircularTileAreas::get; m_lim is 0 when the requested radius is invalid.
//
//================================================================================================================================

struct CircArea {
    u16 m_lim; // Entry count for this radius; 0 if radius invalid
    const i8 (*m_brd)[2]; // Offset pairs {dx, dy}; undefined if m_lim is 0
};

//================================================================================================================================
//=> - CircularTileAreas -
//================================================================================================================================
//
//  Static lookup of Euclidean disk offsets (dist < radius) for city borders and unit visibility.
//  Offsets are shared; get returns a by-value CircArea that views the first m_lim entries.
//
//================================================================================================================================

class CircularTileAreas {
public:
    static CircArea get (u16 radius);

private:
    static const u16 m_r_max; // Largest valid radius index into m_cnt
};

#endif // CIRCULAR_TILE_AREAS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
