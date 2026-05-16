//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_OVERLAY_H
#define GENERATE_OVERLAY_H

#include "game_primitives.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Overlay -
//================================================================================================================================

class Generate_Overlay {
public:
    explicit Generate_Overlay ();
    ~Generate_Overlay ();

    bool generate_spec_terrain_overlay (const MapArrayTerrain& ter, u8 cls, bool invert = false);
    bool generate_terrain_limit_overlay (const MapArrayTerrain& ter, u8 cls, bool invert = false);
    bool generate_spec_terrains_overlay (const MapArrayTerrain& ter, u8 cls0, u8 cls1);

    bool is_valid () const;
    bool save_output (cstr path) const;

    u16 width () const;
    u16 height () const;
    const u8* overlay_gray () const;
    const MapArrayOverlay* overlay_ptr () const;

    MapArrayOverlay* take_overlay ();

private:
    Generate_Overlay (const Generate_Overlay& other) = delete;
    Generate_Overlay (Generate_Overlay&& other) = delete;

    bool begin_overlay (const MapArrayTerrain& ter);

    bool m_valid_generation;
    MapArrayOverlay* m_overlay;
};

#endif // GENERATE_OVERLAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
