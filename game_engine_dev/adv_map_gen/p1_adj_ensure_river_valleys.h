//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_ENSURE_RIVER_VALLEYS_H
#define P1_ADJ_ENSURE_RIVER_VALLEYS_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Adj_EnsureRiverValleys -
//================================================================================================================================

class P1_Adj_EnsureRiverValleys {
public:
    explicit P1_Adj_EnsureRiverValleys (const P1_RunPrm& prm);

    bool adjust (u8* terrain, u16 w, u16 h, const u8* riv, const u16* dist_dn);
    bool is_valid () const;

private:
    P1_Adj_EnsureRiverValleys (const P1_Adj_EnsureRiverValleys& other) = delete;
    P1_Adj_EnsureRiverValleys (P1_Adj_EnsureRiverValleys&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_adjust;
};

#endif // P1_ADJ_ENSURE_RIVER_VALLEYS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
