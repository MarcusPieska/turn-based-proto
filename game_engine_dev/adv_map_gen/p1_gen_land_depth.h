//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef P1_GEN_LAND_DEPTH_H
#define P1_GEN_LAND_DEPTH_H

#include "game_primitives.h"
#include "generator_constants.h"
#include "p1_map_size.h"

//================================================================================================================================
//=> - P1_Gen_LandDepthRslt -
//================================================================================================================================

struct P1_Gen_LandDepthRslt {
    u16 m_w;
    u16 m_h;
    MapArrayOverlay m_wl;
    MapArrayDistance m_dist;
};

//================================================================================================================================
//=> - P1_Gen_LandDepth -
//================================================================================================================================

class P1_Gen_LandDepth {
public:
    explicit P1_Gen_LandDepth (const P1_RunPrm& prm);

    bool generate ();
    bool generate (const u8* ol_gray, u16 w, u16 h);
    bool is_valid () const;
    const P1_Gen_LandDepthRslt& result () const;
    void save_output (cstr path) const;

private:
    P1_Gen_LandDepth (const P1_Gen_LandDepth& other) = delete;
    P1_Gen_LandDepth (P1_Gen_LandDepth&& other) = delete;

    P1_RunPrm m_prm;
    bool m_valid_generation;
    P1_Gen_LandDepthRslt m_rslt;
};

#endif // P1_GEN_LAND_DEPTH_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
