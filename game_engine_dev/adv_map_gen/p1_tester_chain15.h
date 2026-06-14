//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_CHAIN15_H
#define P1_TESTER_CHAIN15_H

#include "game_primitives.h"
#include "p1_adj_land_altitude.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_TesterChain15Rslt -
//================================================================================================================================

struct P1_TesterChain15Rslt {
    u16 m_w;
    u16 m_h;
    u8* m_terrain;
    u8* m_river;
    u8* m_noise;
    u8* m_dist_riv;
    u8* m_near_mtn;
};

//================================================================================================================================
//=> - P1 tester chain15 -
//================================================================================================================================

bool p1_build_step14_input (
    const P1_RunPrm& prm,
    P1_TesterChain15Rslt* out,
    double* out_sec);
bool p1_build_step15 (
    const P1_RunPrm& prm,
    const P1_Adj_LandAltitudePrm& lap,
    P1_TesterChain15Rslt* out,
    double* out_sec);
bool p1_build_ensure_input (
    const P1_RunPrm& prm,
    const P1_Adj_LandAltitudePrm& lap,
    u16 step_n,
    P1_TesterChain15Rslt* out,
    double* out_sec);
void p1_free_chain15 (P1_TesterChain15Rslt* out);
bool p1_save_terrain_rivers_ppm (
    cstr path,
    const u8* terrain,
    const u8* river,
    u16 w,
    u16 h);

#endif // P1_TESTER_CHAIN15_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
