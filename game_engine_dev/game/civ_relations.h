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
    CIV_REL_ALLY = 4,
    CIV_REL_SUBJECT = 5,
};

//================================================================================================================================
//=> - CivRelations -
//================================================================================================================================
//
//  civ_n x civ_n directed relation matrix (full static civ roster).
//  Backed by MapBitArrayOverlay with 3 bits per cell; CivStaticDataKey indexes
//  rows/cols directly. Valid keys are always in range for a sized matrix.
//
//================================================================================================================================

class CivRelations {
public:
    CivRelations ();
    ~CivRelations ();

    void reset (u16 civ_n);

    CivRel get (CivStaticDataKey c1, CivStaticDataKey c2) const;
    void set (CivStaticDataKey c1, CivStaticDataKey c2, CivRel rel);

    u16 civ_n () const;

private:
    CivRelations (const CivRelations& o) = delete;
    CivRelations& operator= (const CivRelations& o) = delete;

    MapBitArrayOverlay* m_grid;
};

#endif // CIV_RELATIONS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
