//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GEN_LAND_SECTORS_H
#define GEN_LAND_SECTORS_H

#include "game_primitives.h"
#include "whiteboard_mng.h"

class GameArraySimple;

//================================================================================================================================
//=> - LandSectorSeedPt / LandSectorSeeds -
//================================================================================================================================
//
//  Heap sector array for GenLandSectors. gen_seeds allocates m_pts, runs the sector flood, then
//  writes tile / yield / resource totals (plus land index / same-land sector count). Call free_seeds
//  when done. Yields need TileAttrTables set up; m_res counts map resource tiles.
//
//================================================================================================================================

struct LandSectorSeedPt {
    u16 m_x; // Seed column
    u16 m_y; // Seed row
    u16 m_land; // Continent / land-mass index (1-based)
    u16 m_land_sec_n; // Sectors that share m_land
    u32 m_tiles; // Exact walkable tiles after gen_seeds flood
    u32 m_yields; // Sum of food+prod+commerce over sector tiles
    u32 m_res; // Count of tiles with a map resource
};

struct LandSectorSeeds {
    LandSectorSeedPt* m_pts; // Heap sectors; null if empty / freed
    u16 m_n; // Valid sector count
};

//================================================================================================================================
//=> - GenLandSectors -
//================================================================================================================================
//
//  Land sectorization (impl via GEN_LAND_SECTORS_IMPL; default mk01). After begin, gen_seeds places
//  staggered lattice / small-mass seeds, transparently runs build (multi-source flood into m_sec),
//  then completes the sector array with m_tiles / m_yields / m_res. sectors() returns the flood whiteboard.
//  Needs WhiteboardMng sized to the map.
//
//================================================================================================================================

#define GLS_IDX_NONE 0u

class GenLandSectors {
public:
    GenLandSectors ();
    ~GenLandSectors ();

    static void free_seeds (LandSectorSeeds* seeds);

    bool begin (const GameArraySimple& map);
    bool gen_seeds (u32 rng_seed, LandSectorSeeds* out);
    bool build (const LandSectorSeeds& seeds);
    void clr ();

    bool ok () const;
    u16 sector_n () const;
    u32 paint_n () const;
    const Whiteboard_2B& sectors () const;

private:
    GenLandSectors (const GenLandSectors& other) = delete;
    GenLandSectors& operator= (const GenLandSectors& other) = delete;
    GenLandSectors (GenLandSectors&& other) = delete;
    GenLandSectors& operator= (GenLandSectors&& other) = delete;

    const GameArraySimple* m_map; // Non-owning terrain
    Whiteboard_2B m_sec; // Sector id + 1 per walkable tile; 0 if none
    u16 m_sec_n; // Seeds that started a sector
    u32 m_paint_n; // Walkable tiles assigned
    bool m_ok; // True after begin with live whiteboard
};

#endif // GEN_LAND_SECTORS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
