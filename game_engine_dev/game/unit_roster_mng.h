//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_ROSTER_MNG_H
#define UNIT_ROSTER_MNG_H

#include "game_primitives.h"

class RuntimeStatics;
struct AssessorCtx;

//================================================================================================================================
//=> - UnitRosterMng -
//================================================================================================================================
//
//  Per unit-type catalog row: currently available best unit index (U16_KEY_NULL if none).
//  Best = max attack+defense+mvt_pts+sight among units of that type that pass UnitAssessor.
//  Ties keep the higher catalog index. Body from impl/unit_roster_mng_impl_mkNN.cpp.
//  mk2/mk3 setup packs unit indices by type into m_pack (score-sorted) for single-type select.
//  mk3 rebuild fills the roster via get_best_unit_of_type per type.
//
//================================================================================================================================

class UnitRosterMng {
public:
    UnitRosterMng () = delete;

    struct TypeIx {
        u16 m_cnt;                                                      // unit count of this type
        u16 m_start;                                                    // start offset into m_pack
    };

    static bool setup (const RuntimeStatics& st);
    static void clear ();
    static bool rebuild (const AssessorCtx& ctx);
    static const u16* roster ();
    static u16 get_best_unit_of_type (u16 type_idx, const AssessorCtx& ctx);
    static u16 type_n ();

private:
    static const RuntimeStatics* m_st;
    static u16* m_roster;
    static TypeIx* m_type_ix;
    static u16* m_pack;
    static u16 m_type_n;
    static u16 m_unit_n;
};

#endif // UNIT_ROSTER_MNG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
