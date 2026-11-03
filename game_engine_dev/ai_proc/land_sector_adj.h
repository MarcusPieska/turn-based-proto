//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LAND_SECTOR_ADJ_H
#define LAND_SECTOR_ADJ_H

#include "game_primitives.h"
#include "land_sector_network.h"

//================================================================================================================================
//=> - LandSectorAdj -
//================================================================================================================================
//
//  Shared directed CSR over LandSectorNetwork links. Built once; selectors read it without owning
//  graph state. m_len mirrors undirected border length on both directed arcs.
//
//================================================================================================================================

class LandSectorAdj {
public:
    LandSectorAdj ();
    ~LandSectorAdj ();

    void clr ();
    bool build (const LandSectorNetwork& net, u16 sec_n);
    bool ok () const;
    u16 sec_n () const;

    u16 hd (u16 s) const;
    u16 to (u16 e) const;
    u16 nx (u16 e) const;
    u16 ln (u16 e) const;

private:
    LandSectorAdj (const LandSectorAdj& o) = delete;
    LandSectorAdj& operator= (const LandSectorAdj& o) = delete;

    u16* m_head; // Adjacency list heads
    u16* m_to; // Edge destinations
    u16* m_nxt; // Edge next indices
    u16* m_len; // Edge border lengths
    u16 m_sec_n; // Sector count
    bool m_ok;
};

#endif // LAND_SECTOR_ADJ_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
