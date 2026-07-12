//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_shaped_outline_view.h"

#include "map_terrain_data.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static bool save_terrain_ppm (cstr path, const u8* terrain, u16 w, u16 h) {
    if (path == nullptr || terrain == nullptr || w == 0 || h == 0) {
        return false;
    }
    MapTerrainData map;
    u8* raw = const_cast<u8*>(terrain);
    if (!map.assign_raw(w, h, raw)) {
        return false;
    }
    return map.save_terrain_ppm(path);
}

//================================================================================================================================
//=> - P1_Gen_ShapedOutlineView -
//================================================================================================================================

bool P1_Gen_ShapedOutlineView::save_pri (cstr path, const u8* terrain, u16 w, u16 h) {
    return save_terrain_ppm(path, terrain, w, h);
}

bool P1_Gen_ShapedOutlineView::save_near (cstr path, const u8* terrain, u16 w, u16 h) {
    return save_terrain_ppm(path, terrain, w, h);
}

bool P1_Gen_ShapedOutlineView::save_far (cstr path, const u8* terrain, u16 w, u16 h) {
    return save_terrain_ppm(path, terrain, w, h);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
