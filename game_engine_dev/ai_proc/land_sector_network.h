//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LAND_SECTOR_NETWORK_H
#define LAND_SECTOR_NETWORK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - LandSectorLink -
//================================================================================================================================
//
//  Undirected adjacency between two GenLandSectors ids (0-based). m_x/m_y is one shared-border
//  contact tile; m_len counts 4-neighbor border touches (each undirected edge counted once per side).
//
//================================================================================================================================

struct LandSectorLink {
    u16 m_a; // Sector index (0-based)
    u16 m_b; // Sector index (0-based), m_a < m_b
    u16 m_x; // Contact tile column
    u16 m_y; // Contact tile row
    u16 m_len; // Shared-border touch count
};

//================================================================================================================================
//=> - LandSectorNetwork -
//================================================================================================================================
//
//  Owns a heap array of LandSectorLink. GenLandSectorNetwork::build fills it; clr / dtor free it.
//
//================================================================================================================================

class LandSectorNetwork {
public:
    LandSectorNetwork ();
    ~LandSectorNetwork ();

    void clr ();
    bool ok () const;
    u16 link_n () const;
    const LandSectorLink* get (u16 i) const;
    bool take (LandSectorLink* links, u16 n); // Takes ownership of heap links

private:
    LandSectorNetwork (const LandSectorNetwork& o) = delete;
    LandSectorNetwork& operator= (const LandSectorNetwork& o) = delete;
    LandSectorNetwork (LandSectorNetwork&& o) = delete;
    LandSectorNetwork& operator= (LandSectorNetwork&& o) = delete;

    LandSectorLink* m_links; // Heap link table
    u16 m_n; // Valid link count
    bool m_ok; // True after a successful take
};

#endif // LAND_SECTOR_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
