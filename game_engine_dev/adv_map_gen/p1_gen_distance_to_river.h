//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_DISTANCE_TO_RIVER_H
#define P1_GEN_DISTANCE_TO_RIVER_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_DistanceToRiverRslt -
//================================================================================================================================

struct P1_Gen_DistanceToRiverRslt {
    u16 m_w;
    u16 m_h;
    u16 m_max_dist;
    MapArrayOverlay m_ov;
};

//================================================================================================================================
//=> - P1_Gen_DistanceToRiver -
//================================================================================================================================

class P1_Gen_DistanceToRiver {
public:
    explicit P1_Gen_DistanceToRiver (const P1_RunPrm& prm);

    bool generate (const u8* terrain, u16 w, u16 h, const u8* river_ov);
    bool is_valid () const;
    const P1_Gen_DistanceToRiverRslt& result () const;
    void save_output (cstr path) const;

private:
    P1_Gen_DistanceToRiver (const P1_Gen_DistanceToRiver& other) = delete;
    P1_Gen_DistanceToRiver (P1_Gen_DistanceToRiver&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_DistanceToRiverRslt m_rslt;
};

#endif // P1_GEN_DISTANCE_TO_RIVER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
