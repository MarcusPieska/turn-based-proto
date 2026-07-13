//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_make_map.h"
#include "p1_tester_chain_core.h"

#include <cstdio>

//================================================================================================================================
//=> - Early-tester chain stubs -
//================================================================================================================================

void P1_MakeMap::free_rslt (P1_MakeMapRslt* rslt) {
    if (rslt == nullptr) {
        return;
    }
    delete[] rslt->m_terrain;
    delete[] rslt->m_climate;
    delete[] rslt->m_rivers;
    delete[] rslt->m_wshed;
    delete[] rslt->m_wind_dir;
    delete[] rslt->m_wind_str;
    delete[] rslt->m_loess;
    delete[] rslt->m_rain;
    rslt->m_terrain = nullptr;
    rslt->m_climate = nullptr;
    rslt->m_rivers = nullptr;
    rslt->m_wshed = nullptr;
    rslt->m_wind_dir = nullptr;
    rslt->m_wind_str = nullptr;
    rslt->m_loess = nullptr;
    rslt->m_rain = nullptr;
    rslt->m_w = 0;
    rslt->m_h = 0;
}

bool p1_build_chain_core (const P1_RunPrm& prm, u16 last_step, P1_MakeMapRslt* out, double* out_sec) {
    (void)prm;
    (void)last_step;
    (void)out;
    if (out_sec != nullptr) {
        *out_sec = 0.0;
    }
    std::printf("P1_MakeMap chain unavailable in early-only build\n");
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
