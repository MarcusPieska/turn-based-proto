//================================================================================================================================
//= - Include guards -
//================================================================================================================================

#ifndef R1_GEN_STD_RES_DIST_H
#define R1_GEN_STD_RES_DIST_H

#include "game_primitives.h"
#include "resource_placement.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//= - R1_Gen_StdResDist -
//================================================================================================================================
//=
//= P1-facing resource distribution facade. Runs the r1 sub-pipeline: empty overlay, res sectors,
//= gemstone adjust, general adjust. Future r1 steps stay behind this API.
//=
//================================================================================================================================

class R1_Gen_StdResDist {
public:
    R1_Gen_StdResDist ();
    ~R1_Gen_StdResDist ();

    R1_Gen_StdResDist (const R1_Gen_StdResDist& o) = delete;
    R1_Gen_StdResDist (R1_Gen_StdResDist&& o) = delete;

    bool generate (const ResPlcMapCtx& ctx, const RuntimeStatics& s, u32 base_n, u32 seed);
    
    bool is_valid () const;
    u16 width () const;
    u16 height () const;
    const u16* overlay () const;
    u16* take_overlay ();

private:
    void clr ();

    bool m_ok; // True after successful generate
    u16 m_w; // Overlay width
    u16 m_h; // Overlay height
    u16* m_ov; // Owned resource overlay
};

#endif // R1_GEN_STD_RES_DIST_H

//================================================================================================================================
//= - End of file -
//================================================================================================================================
