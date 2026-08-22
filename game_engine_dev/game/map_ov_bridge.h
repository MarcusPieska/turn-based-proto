//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_OV_BRIDGE_H
#define MAP_OV_BRIDGE_H

#include "assert_log.h"
#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_overlay_enum.h"

//================================================================================================================================
//=> - Map overlay bridge -
//================================================================================================================================
//
//  Map-gen / PPM overlay class ids (OV_* in game_map_defs.h) are not the same as
//  game_config.map_overlays catalog indices. Translate at the GameTileSimple boundary.
//  Empty occupancy is U16_KEY_NULL (not MapOverlay::City / not OV_NONE).
//
//================================================================================================================================

static inline u16 map_gen_ov_to_catalog (u8 ov) {
    if (ov == OV_NONE[0]) {
        return U16_KEY_NULL;
    }
    if (ov == OV_FOREST[0]) {
        return static_cast<u16>(MapOverlay::Forest);
    }
    if (ov == OV_SWAMP[0]) {
        return static_cast<u16>(MapOverlay::Swamp);
    }
    if (ov == OV_JUNGLE[0]) {
        return static_cast<u16>(MapOverlay::Jungle);
    }
    if (ov == OV_GLACIER[0]) {
        return static_cast<u16>(MapOverlay::Glacier);
    }
    GAME_EXPECT(false, "map_gen_ov_to_catalog unknown overlay class");
    return U16_KEY_NULL;
}

static inline u8 catalog_ov_to_map_gen (u16 ov) {
    if (ov == U16_KEY_NULL) {
        return OV_NONE[0];
    }
    if (ov == static_cast<u16>(MapOverlay::Forest)) {
        return OV_FOREST[0];
    }
    if (ov == static_cast<u16>(MapOverlay::Swamp)) {
        return OV_SWAMP[0];
    }
    if (ov == static_cast<u16>(MapOverlay::Jungle)) {
        return OV_JUNGLE[0];
    }
    if (ov == static_cast<u16>(MapOverlay::Glacier)) {
        return OV_GLACIER[0];
    }
    return OV_GLACIER[0];
}

#endif // MAP_OV_BRIDGE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
