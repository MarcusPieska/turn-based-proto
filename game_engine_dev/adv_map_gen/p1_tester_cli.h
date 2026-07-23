//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_TESTER_CLI_H
#define P1_TESTER_CLI_H

#include "game_primitives.h"
#include "p1_adj_land_altitude.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_TesterCli -
//================================================================================================================================
//
//  Shared CLI parsing for all P1 test drivers.
//
//================================================================================================================================

class P1_TesterCli {
public:
    P1_TesterCli ();
    bool parse (i32 argc, char* argv[]);
    const P1_RunPrm& prm () const;
    bool full () const;
    bool keep () const;
    bool batch () const;
    bool out_subdir () const;
    const P1_Adj_LandAltitudePrm& lap () const;
    P1_Adj_LandAltitudePrm& lap_mut ();
    u8 rain_wt () const;
    bool rain_wt_set () const;
    u16 sec_pct () const;

private:
    P1_RunPrm m_prm;
    bool m_full;
    bool m_keep;
    bool m_batch;
    bool m_out_subdir;
    P1_Adj_LandAltitudePrm m_lap;
    u8 m_rain_wt;
    bool m_rain_wt_set;
    u16 m_sec_pct;
};

bool p1_tester_out_subdir ();
void p1_tester_set_out_subdir (bool v);
bool p1_tester_batch_export ();
void p1_tester_set_batch_export (bool v);

#endif // P1_TESTER_CLI_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
