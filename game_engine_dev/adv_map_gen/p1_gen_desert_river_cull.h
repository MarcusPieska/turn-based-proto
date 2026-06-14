//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_DESERT_RIVER_CULL_H
#define P1_GEN_DESERT_RIVER_CULL_H

#include "game_primitives.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_DesertRiverCull -
//================================================================================================================================

class P1_Gen_DesertRiverCull {
public:
    explicit P1_Gen_DesertRiverCull (const P1_RunPrm& prm);

    bool generate (
        u8* riv,
        u16 w,
        u16 h,
        const u8* terrain,
        const u8* climate,
        bool cull_from_upstream);
    bool is_valid () const;
    u32 culled_n () const;

private:
    P1_Gen_DesertRiverCull (const P1_Gen_DesertRiverCull& other) = delete;
    P1_Gen_DesertRiverCull (P1_Gen_DesertRiverCull&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    u32 m_culled_n;
};

#endif // P1_GEN_DESERT_RIVER_CULL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
