//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LAND_MASS_INDEX_H
#define LAND_MASS_INDEX_H

#include "game_primitives.h"

//================================================================================================================================
//=> - LandMassIndex -
//================================================================================================================================
//
//  Flood-fills connected non-water tiles into a dense u16 overlay (0 = water/none, 1..n = landmass).
//  Copied from adv_map_gen P1_Gen_OceanIndex and inverted for land; used to reject cross-mass paths early.
//
//================================================================================================================================

#define LAND_MASS_IDX_NONE 0u

struct LandMassIndexRslt {
    u16 m_w; // Overlay width
    u16 m_h; // Overlay height
    u16 m_mass_n; // Landmass count (indices 1..m_mass_n)
    u16 m_largest_idx; // Index of largest landmass; NONE if empty
    u32 m_land_n; // Non-water tile count
    u16* m_ov; // Row-major indices; length m_w * m_h; owned by LandMassIndex
};

class LandMassIndex {
public:
    LandMassIndex ();
    ~LandMassIndex ();

    bool generate (const u8* terr, u16 w, u16 h);
    bool is_valid () const;
    const LandMassIndexRslt& result () const;

private:
    LandMassIndex (const LandMassIndex& other) = delete;
    LandMassIndex (LandMassIndex&& other) = delete;

    void clear_rslt ();

    bool m_ok; // True after successful generate
    LandMassIndexRslt m_rslt; // Owned overlay result
};

#endif // LAND_MASS_INDEX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
