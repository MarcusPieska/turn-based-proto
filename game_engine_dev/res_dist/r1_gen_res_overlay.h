//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_GEN_RES_OVERLAY_H
#define R1_GEN_RES_OVERLAY_H

#include "game_primitives.h"
#include "resource_placement.h"
#include "runtime_static_loader.h"

//================================================================================================================================
//=> - R1_Gen_ResOverlay -
//================================================================================================================================

class R1_Gen_ResOverlay {
public:
    R1_Gen_ResOverlay ();
    ~R1_Gen_ResOverlay ();

    R1_Gen_ResOverlay (const R1_Gen_ResOverlay& o) = delete;
    R1_Gen_ResOverlay (R1_Gen_ResOverlay&& o) = delete;

    bool generate (
        const ResPlcMapCtx& ctx,
        const RuntimeStatics& s,
        u32 base_n,
        u32 seed);
    bool is_valid () const;
    u16 width () const;
    u16 height () const;
    const u16* overlay () const;
    u16* take_overlay ();

    static bool save_ppm (
        cstr path,
        const ResPlcMapCtx& ctx,
        const u16* res_ov,
        u16 w,
        u16 h,
        u16 res_max);

private:
    bool m_valid;
    u16 m_w;
    u16 m_h;
    u16* m_ov;
};

#endif // R1_GEN_RES_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
