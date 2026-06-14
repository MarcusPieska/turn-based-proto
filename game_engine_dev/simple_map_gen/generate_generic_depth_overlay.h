//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_GENERIC_DEPTH_OVERLAY_H
#define GENERATE_GENERIC_DEPTH_OVERLAY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Generate_GenericDepthOverlay -
//================================================================================================================================

class Generate_GenericDepthOverlay {
public:
    static u8* generate (const u8* terrain, u16 w, u16 h, u8 terr_idx);
    static u8* generate_ge (const u8* terrain, u16 w, u16 h, u8 terr_idx);
    static u16* generate_ge_dist (const u8* terrain, u16 w, u16 h, u8 terr_idx);

private:
    Generate_GenericDepthOverlay () = delete;
    Generate_GenericDepthOverlay (const Generate_GenericDepthOverlay& other) = delete;
    Generate_GenericDepthOverlay (Generate_GenericDepthOverlay&& other) = delete;
};

#endif // GENERATE_GENERIC_DEPTH_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
