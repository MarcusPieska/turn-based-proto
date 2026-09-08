//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCATION_GROUPER_H
#define LOCATION_GROUPER_H

#include "continent_size_indexer.h"
#include "game_primitives.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - LocGroup -
//================================================================================================================================
//
//  Starts on one continent, sorted highest land-potential score first. Arrays are sized to the full
//  start list capacity; m_n is how many were actually placed in this group.
//
//================================================================================================================================

struct LocGroup {
    SpgCoordPair* m_pts; // Owned start coords; length m_cap
    i32* m_sc; // Owned parallel scores; length m_cap
    u16 m_n; // Filled entries
    u16 m_cap; // Allocated length (== total start count)
    u16 m_rank; // Land index 1..k matching ContinentSizeIndexer out_idx
};

//================================================================================================================================
//=> - LocGroups -
//================================================================================================================================

class LocGroups {
public:
    LocGroups ();
    ~LocGroups ();

    void clear ();

    LocGroup m_g[ContSizeList::k_cap]; // One slot per indexed continent
    u16 m_n; // Continent slots used (matches ContSizeList.m_n)

private:
    LocGroups (const LocGroups& o) = delete;
    LocGroups& operator= (const LocGroups& o) = delete;
};

//================================================================================================================================
//=> - LocationGrouper -
//================================================================================================================================
//
//  Buckets starts by remapped land index, then sorts each bucket by score descending.
//
//================================================================================================================================

class LocationGrouper {
public:
    static bool group (
        const SpgCoordPair* starts,
        const i32* scores,
        u16 n,
        const u16* land_idx,
        u16 w,
        u16 h,
        u16 cont_n,
        LocGroups* out);

private:
    LocationGrouper () = delete;
};

#endif // LOCATION_GROUPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
