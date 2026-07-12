//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_cont_outlines_view.h"

#include "generator_constants.h"
#include "p1_gen_cont_outlines.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static bool ov_is_gray_fmt (const u8* comp, u32 n) {
    if (comp == nullptr || n == 0) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        if (comp[i] == WL_OVERLAY_LAND_GRAY || comp[i] == WL_OVERLAY_WATER_GRAY) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - P1_Gen_ContOutlinesView -
//================================================================================================================================

bool P1_Gen_ContOutlinesView::save_pri (cstr path, const u8* comp, u16 w, u16 h) {
    if (path == nullptr || comp == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    MapArrayOverlay map;
    if (ov_is_gray_fmt(comp, n)) {
        u8* raw = const_cast<u8*>(comp);
        return map.assign_copy(w, h, raw) && map.save(path);
    }
    Whiteboard_1B wb_tmp("P1_Gen_ContOutlinesView", "gray", 0u);
    P1_WB_CHK(wb_tmp);
    u8* tmp = wb_tmp.get_iter_ptr();
    p1_cont_ov_to_gray(comp, tmp, w, h);
    return map.assign_copy(w, h, tmp) && map.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
