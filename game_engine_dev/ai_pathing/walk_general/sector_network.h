//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SECTOR_NETWORK_H
#define SECTOR_NETWORK_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Sector -
//================================================================================================================================
//
//  One dense lattice cell (id = col * rows + row). Centers start at (5,5) on a 10px grid;
//  even columns -2py, odd +3py. m_mask is land connectivity; m_wmask is reserved for water.
//  Bit order: 0 top, 1 bottom, 2 top-left, 3 top-right, 4 bottom-left, 5 bottom-right.
//
//================================================================================================================================

struct Sector {
    u16 m_x; // Center column
    u16 m_y; // Center row
    u8 m_mask; // Land connectivity to 6 hex neighbors
    u8 m_wmask; // Water connectivity to 6 hex neighbors
};

//================================================================================================================================
//=> - SectorNetwork -
//================================================================================================================================
//
//  Dense hex-like sector lattice covering the map. id is the index in m_sec. Land and water
//  links are separate center-to-center floods (m_mask / m_wmask); land is blocked by water and
//  mountains, water walks water tiles only.
//
//================================================================================================================================

//#define SN_STEP 10u
//#define SN_ORIGIN 5u
//#define SN_OFF_EVEN 2
//#define SN_OFF_ODD 3
//#define SN_FLOOD_R 16u

#define SN_STEP 4u
#define SN_ORIGIN SN_STEP / 2u
#define SN_OFF_EVEN SN_STEP / 4u
#define SN_OFF_ODD SN_STEP / 4u
#define SN_FLOOD_R SN_STEP * 2u


#define SN_WIN (SN_FLOOD_R * 2u + 1u)
#define SN_WIN_N (SN_WIN * SN_WIN)

class SectorNetwork {
public:
    SectorNetwork ();
    ~SectorNetwork ();

    bool begin (u16 w, u16 h, const u8* terr);
    bool is_valid () const;
    u16 sector_n () const;
    u16 id_at (u16 x, u16 y) const;
    u16 nbr (u16 id, u8 bit) const;
    u16 wnbr (u16 id, u8 bit) const;
    const Sector* get (u16 id) const;

private:
    SectorNetwork (const SectorNetwork& other) = delete;
    SectorNetwork (SectorNetwork&& other) = delete;

    void clear ();
    static i32 cen_x (i32 col);
    static i32 cen_y (i32 col, i32 row);
    u16 id_of (i32 col, i32 row) const;
    bool tile_pass (const u8* terr, u16 x, u16 y, bool water) const;
    void wire (const u8* terr);
    void flood_sec (u16 id, const u8* terr, bool water, u16* wd, u8* qx, u8* qy);

    bool m_ok; // True after successful begin
    u16 m_w; // Map width used to build lattice
    u16 m_h; // Map height used to build lattice
    u16 m_n; // Sector count (= m_cols * m_rows)
    u16 m_cols; // Lattice column count
    u16 m_rows; // Lattice row count
    Sector* m_sec; // Dense sector table; id = col * m_rows + row
};

#endif // SECTOR_NETWORK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
