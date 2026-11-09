//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_RELATIONS_H
#define CIV_RELATIONS_H

#include "civ_static_key.h"
#include "game_primitives.h"

class MapBitArrayOverlay;

//================================================================================================================================
//=> - CivRel -
//================================================================================================================================

enum class CivRel : u8 {
    CIV_REL_UNDISCOVERED = 0,
    CIV_REL_NONE = 1,
    CIV_REL_WAR = 2,
    CIV_REL_PEACE = 3,
    CIV_REL_PEACE_ACCESS = 4,
    CIV_REL_ALLY = 5,
    CIV_REL_SUBJECT = 6,
};

//================================================================================================================================
//=> - CivRelations -
//================================================================================================================================
//
//  Per-seat relation row (MapBitArrayOverlay of length civ_n, 4 bits/cell). Keys are seat indices
//  via CivStaticDataKey::from_raw. set() writes both seats' rows. remove_civ frees a destroyed seat.
//
//================================================================================================================================

class CivRelations {
public:
    CivRelations ();
    ~CivRelations ();

    void reset (u16 civ_n);
    bool add_civ (u16 i);
    void remove_civ (u16 i);

    CivRel get (CivStaticDataKey c1, CivStaticDataKey c2) const;
    void set (CivStaticDataKey c1, CivStaticDataKey c2, CivRel rel);

    u16 civ_n () const;

private:
    CivRelations (const CivRelations& o) = delete;
    CivRelations& operator= (const CivRelations& o) = delete;

    void clr_rows ();

    MapBitArrayOverlay** m_row; // One row per seat; nullptr if not added / removed
    u16 m_n; // Capacity (seat count)
};

#endif // CIV_RELATIONS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
