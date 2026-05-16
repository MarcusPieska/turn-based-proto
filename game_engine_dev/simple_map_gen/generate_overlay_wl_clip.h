//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_OVERLAY_WL_CLIP_H
#define GENERATE_OVERLAY_WL_CLIP_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - OverlayWlClip -
//================================================================================================================================

class Generate_OverlayWlClip {
public:
    explicit Generate_OverlayWlClip ();
    ~Generate_OverlayWlClip ();

    bool generate (const MapArrayOverlay& first, const MapArrayOverlay& second, i32 offset_x = 0, i32 offset_y = 0);
    bool is_valid () const;
    void save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const u8* water_land_gray () const;
    bool save_water_land_gray (cstr path) const;

    MapArrayOverlay* take_overlay ();

private:
    Generate_OverlayWlClip (const Generate_OverlayWlClip& other) = delete;
    Generate_OverlayWlClip (Generate_OverlayWlClip&& other) = delete;

    bool m_valid_generation;
    MapArrayOverlay* m_overlay;
};

#endif // GENERATE_OVERLAY_WL_CLIP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
