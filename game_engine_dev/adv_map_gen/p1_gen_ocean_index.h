//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_OCEAN_INDEX_H
#define P1_GEN_OCEAN_INDEX_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1 ocean index limits -
//================================================================================================================================

#define P1_OCEAN_IDX_NONE 0u
#define P1_OCEAN_IDX_MAX 255u

//================================================================================================================================
//=> - P1_Gen_OceanIndexRslt -
//================================================================================================================================

struct P1_Gen_OceanIndexRslt {
    u16 m_w;
    u16 m_h;
    u16 m_ocean_n;
    u16 m_largest_idx;
    u32 m_wat_n;
    MapArrayDistance m_ov;
};

//================================================================================================================================
//=> - P1_OceanIndexRef -
//================================================================================================================================
//
//  Non-owning view of an ocean index overlay for pipeline handoff.
//
//================================================================================================================================

struct P1_OceanIndexRef {
    u16 m_w;
    u16 m_h;
    u16 m_ocean_n;
    u16 m_largest_idx;
    u32 m_wat_n;
    const u16* m_ov;
};

inline bool p1_ocean_ref_ok (const P1_OceanIndexRef& r) {
    return r.m_ov != nullptr && p1_map_size_ok(r.m_w, r.m_h);
}

inline P1_OceanIndexRef p1_ocean_ref_from_rslt (const P1_Gen_OceanIndexRslt& r) {
    P1_OceanIndexRef out;
    out.m_w = r.m_w;
    out.m_h = r.m_h;
    out.m_ocean_n = r.m_ocean_n;
    out.m_largest_idx = r.m_largest_idx;
    out.m_wat_n = r.m_wat_n;
    out.m_ov = r.m_ov.data();
    return out;
}

//================================================================================================================================
//=> - P1_Gen_OceanIndex -
//================================================================================================================================
//
//  Sweep ocean/sea/coastal tiles; flood each unindexed component with 1..255; skip inland lake/sea.
//
//================================================================================================================================

class P1_Gen_OceanIndex {
public:
    explicit P1_Gen_OceanIndex (const P1_RunPrm& prm);

    bool generate (const u8* terrain, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_OceanIndexRslt& result () const;

private:
    P1_Gen_OceanIndex (const P1_Gen_OceanIndex& other) = delete;
    P1_Gen_OceanIndex (P1_Gen_OceanIndex&& other) = delete;

    void clear_rslt ();

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_OceanIndexRslt m_rslt;
};

#endif // P1_GEN_OCEAN_INDEX_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
