//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_MAKE_MAP_H
#define P1_MAKE_MAP_H

#include "game_primitives.h"
#include "p1_adj_land_altitude.h"
#include "p1_gen_shaped_outline.h"
#include "p1_map_size.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - P1_MakeMapPrm -
//================================================================================================================================

struct P1_MakeMapPrm {
    P1_Gen_ShapedOutlinePrm m_shaped;
    P1_Adj_LandAltitudePrm m_lap;
};

static inline P1_MakeMapPrm p1_make_map_prm_def () {
    P1_MakeMapPrm p;
    p.m_shaped = p1_gen_shaped_outline_prm_def();
    p.m_lap = p1_tester_land_altitude_prm();
    return p;
}

//================================================================================================================================
//=> - P1_MakeMapRslt -
//================================================================================================================================

struct P1_MakeMapRslt {
    u16 m_w;
    u16 m_h;
    u8* m_terrain;
    u8* m_climate;
    u8* m_rivers;
};

//================================================================================================================================
//=> - P1_MakeMap -
//================================================================================================================================

class P1_MakeMap {
public:
    explicit P1_MakeMap (const P1_RunPrm& prm, const P1_MakeMapPrm& mp = p1_make_map_prm_def ());

    bool generate ();
    bool is_valid () const;
    const P1_MakeMapRslt& result () const;
    bool save_terrain_ppm (cstr path) const;
    bool save_climate_ppm (cstr path) const;
    bool save_rivers_ppm (cstr path) const;
    static void free_rslt (P1_MakeMapRslt* rslt);

private:
    P1_MakeMap (const P1_MakeMap& other) = delete;
    P1_MakeMap (P1_MakeMap&& other) = delete;

    P1_RunPrm m_prm;
    P1_MakeMapPrm m_mp;
    bool m_valid_generation;
    P1_MakeMapRslt m_rslt;
};

#endif // P1_MAKE_MAP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
