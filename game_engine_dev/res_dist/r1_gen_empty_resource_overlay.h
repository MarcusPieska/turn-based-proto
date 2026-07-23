//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef R1_GEN_EMPTY_RESOURCE_OVERLAY_H
#define R1_GEN_EMPTY_RESOURCE_OVERLAY_H

#include "game_primitives.h"
#include "resource_placement.h"

//================================================================================================================================
//= - R1_Gen_EmptyResourceOverlay -
//================================================================================================================================
//=
//= Allocates the canonical resource overlay and fills it with U16_KEY_NULL.
//=
//================================================================================================================================

class R1_Gen_EmptyResourceOverlay {
public:
    R1_Gen_EmptyResourceOverlay ();
    ~R1_Gen_EmptyResourceOverlay ();

    R1_Gen_EmptyResourceOverlay (const R1_Gen_EmptyResourceOverlay& o) = delete;
    R1_Gen_EmptyResourceOverlay (R1_Gen_EmptyResourceOverlay&& o) = delete;

    bool generate (u16 w, u16 h);
    bool is_valid () const;
    u16 width () const;
    u16 height () const;
    const u16* overlay () const;
    u16* overlay_mut ();
    u16* take_overlay ();

    static bool save_ppm (
        cstr path,
        const ResPlcMapCtx& ctx,
        const u16* res_ov,
        u16 w,
        u16 h,
        u16 res_max);

private:
    void clr ();

    bool m_ok; // True after successful generate
    u16 m_w; // Overlay width
    u16 m_h; // Overlay height
    u16* m_ov; // Row-major res idx or U16_KEY_NULL
};

#endif // R1_GEN_EMPTY_RESOURCE_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
