//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_HARNESS_H
#define P1_TESTER_HARNESS_H

#include "game_primitives.h"
#include "p1_adj_land_altitude.h"
#include "p1_make_map.h"
#include "p1_map_size.h"
#include "p1_tester_early_chain.h"
#include "map_config.h"
#include "p1_tester_cli.h"

//================================================================================================================================
//=> - P1_TesterHarness -
//================================================================================================================================
//
//  Canonical P1 stage tester setup: clear seed output folder, run shared input chain, expose overlays, finish checks.
//
//================================================================================================================================

class P1_TesterHarness {
public:
    P1_TesterHarness ();
    ~P1_TesterHarness ();
    bool begin (i32 argc, char* argv[]);
    bool run_input (const P1_Adj_LandAltitudePrm* lap = nullptr);
    bool finish ();
    const P1_TesterCli& cli () const;
    bool full () const;
    bool keep () const;
    const P1_Adj_LandAltitudePrm& lap () const;
    u8 rain_wt () const;
    bool rain_wt_set () const;
    const P1_RunPrm& prm () const;
    const MapConfig& cfg () const;
    u32 seed () const;
    u32 step () const;
    double input_sec () const;
    bool path_pri (char* out, size_t cap) const;
    bool path_sec (char* out, size_t cap) const;
    bool has_sec () const;
    bool path_extra (cstr suffix, char* out, size_t cap) const;
    bool has_mk () const;
    const P1_MakeMapRslt& mk () const;
    P1_MakeMapRslt& mk_mut ();
    bool has_early () const;
    const P1_EarlyChainRslt& early () const;
    P1_EarlyChainRslt& early_mut ();

private:
    P1_TesterHarness (const P1_TesterHarness& other) = delete;
    P1_TesterHarness (P1_TesterHarness&& other) = delete;
    P1_TesterHarness& operator= (const P1_TesterHarness& other) = delete;
    P1_TesterHarness& operator= (P1_TesterHarness&& other) = delete;

    void free_all ();

    P1_RunPrm m_prm;
    MapConfig m_cfg;
    P1_TesterCli m_cli;
    P1_MakeMapRslt m_mk;
    P1_EarlyChainRslt m_early;
    bool m_begun;
    bool m_has_mk;
    bool m_has_early;
    bool m_input_ok;
    double m_input_sec;
};

#endif // P1_TESTER_HARNESS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
