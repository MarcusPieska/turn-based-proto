//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_ADJ_ENSURE_MTN_FOOTHILLS_H
#define P1_ADJ_ENSURE_MTN_FOOTHILLS_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Adj_EnsureMtnFoothills -
//================================================================================================================================

class P1_Adj_EnsureMtnFoothills {
public:
    explicit P1_Adj_EnsureMtnFoothills (const P1_RunPrm& prm);

    bool adjust (u8* terrain, u16 w, u16 h);
    bool is_valid () const;

private:
    P1_Adj_EnsureMtnFoothills (const P1_Adj_EnsureMtnFoothills& other) = delete;
    P1_Adj_EnsureMtnFoothills (P1_Adj_EnsureMtnFoothills&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_adjust;
};

#endif // P1_ADJ_ENSURE_MTN_FOOTHILLS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
