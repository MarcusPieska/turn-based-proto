//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_GEN_RES_SECTORS_H
#define R1_GEN_RES_SECTORS_H

#include "game_primitives.h"
#include "land_mass_index.h"

class Whiteboard_2B;
class WB_QueXY;

//================================================================================================================================
//=> - R1_Gen_ResSectors -
//================================================================================================================================
//
//  Lattice-seeded land sectors (step = pct% map W/H, +/- 10% of step jitter), then parallel flood claim
//  until a round claims nothing. Final pass claims mountains from sector edges. Blocked by water/claims/
//  mountains (mountains only in the final pass).
//
//================================================================================================================================

#define R1_RES_SECTOR_NONE 0xFFFFu
#define R1_RES_SECTOR_PCT_DEF 10u
#define R1_RES_SECTOR_PCT_MET 5u
#define R1_RES_SECTOR_SEED_RAD 3
#define R1_RES_SECTOR_SEED_WIN 21
#define R1_RES_SECTOR_SEED_WIN_R 10

struct R1_Gen_ResSectorsRslt {
    u16 m_w; // Overlay width
    u16 m_h; // Overlay height
    u16 m_sec_n; // Sector count (ids 0..m_sec_n-1)
    u16* m_ov; // Row-major sector ids; NONE if unclaimed; owned via whiteboard
};

class R1_Gen_ResSectors {
public:
    R1_Gen_ResSectors ();
    ~R1_Gen_ResSectors ();

    R1_Gen_ResSectors (const R1_Gen_ResSectors& o) = delete;
    R1_Gen_ResSectors (R1_Gen_ResSectors&& o) = delete;

    bool generate (const u8* terr, u16 w, u16 h, const LandMassIndexRslt& mass, u32 seed, u16 pct);
    bool is_valid () const;
    const R1_Gen_ResSectorsRslt& result () const;
    bool save_ppm (cstr path, const u8* terr) const;

private:
    void clr ();

    bool m_ok; // True after successful generate
    R1_Gen_ResSectorsRslt m_rslt; // Sector overlay result
    Whiteboard_2B* m_wb_ov; // Owns m_rslt.m_ov buffer
    WB_QueXY* m_seeds; // Lattice seed tile positions
};

#endif // R1_GEN_RES_SECTORS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
