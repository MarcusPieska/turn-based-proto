//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_tester_early_views.h"

#include "p1_adj_outline_fill_view.h"
#include "p1_gen_cont_outlines_view.h"
#include "p1_gen_land_depth_view.h"
#include "p1_gen_noise_perlin_view.h"
#include "p1_gen_river_pts_view.h"
#include "p1_gen_shaped_outline_view.h"
#include "p1_rprint.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static bool save_path_info (cstr path) {
    if (path == nullptr) {
        return false;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", path);
    P1_RPrint::rprint_info(obuf);
    return true;
}

static bool save_outline (u32 seed, const u8* ov, u16 w, u16 h) {
    char path[320];
    if (ov == nullptr || !p1_make_out_path(seed, "01_outline.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_ContOutlinesView::save_pri(path, ov, w, h)) {
        return false;
    }
    return save_path_info(path);
}

static bool save_outline_fill (u32 seed, const u8* ter, u16 w, u16 h) {
    char path[320];
    if (ter == nullptr || !p1_make_out_path(seed, "02_outline_fill.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Adj_OutlineFillView::save_pri(path, ter, w, h)) {
        return false;
    }
    return save_path_info(path);
}

static bool save_noise (u32 seed, const u8* gray, u16 w, u16 h) {
    char path[320];
    if (gray == nullptr || !p1_make_out_path(seed, "03_noise_perlin.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_NoisePerlinView::save_pri(path, gray, w, h)) {
        return false;
    }
    return save_path_info(path);
}

static bool save_land_depth (u32 seed, const u8* wl, const u16* dist, u16 w, u16 h) {
    char path[320];
    if (wl == nullptr || dist == nullptr || !p1_make_out_path(seed, "04_land_depth.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_LandDepthView::save_pri(path, wl, dist, w, h)) {
        return false;
    }
    return save_path_info(path);
}

static bool save_shaped_ter (u32 seed, cstr fname, const u8* ter, u16 w, u16 h) {
    char path[320];
    if (ter == nullptr || fname == nullptr || !p1_make_out_path(seed, fname, path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_ShapedOutlineView::save_pri(path, ter, w, h)) {
        return false;
    }
    return save_path_info(path);
}

static bool save_river_pts (u32 seed, const u8* ter, const WB_QueXY& que, u16 w, u16 h) {
    char path[320];
    if (ter == nullptr || !que.ok() || que.count() == 0u || !p1_make_out_path(seed, "08_river_pts.ppm", path, sizeof(path))) {
        return false;
    }
    if (!P1_Gen_RiverPtsView::save_pri(path, ter, w, h, que)) {
        return false;
    }
    return save_path_info(path);
}

//================================================================================================================================
//=> - p1_tester_save_early_views -
//================================================================================================================================

bool p1_tester_save_early_views (u32 seed, u16 through_step, const P1_EarlyChainRslt& ec) {
    const u16 w = ec.m_w;
    const u16 h = ec.m_h;
    if (w == 0 || h == 0) {
        return false;
    }
    if (through_step >= 1u) {
        if (!save_outline(seed, ec.m_ov.data(), w, h)) {
            return false;
        }
    }
    if (through_step >= 2u && ec.m_fill_ter != nullptr) {
        if (!save_outline_fill(seed, ec.m_fill_ter, w, h)) {
            return false;
        }
    }
    if (through_step >= 3u && ec.m_perlin_gray.data() != nullptr) {
        if (!save_noise(seed, ec.m_perlin_gray.data(), w, h)) {
            return false;
        }
    }
    if (through_step >= 4u && ec.m_ld_wl.data() != nullptr && ec.m_ld_dist.data() != nullptr) {
        if (!save_land_depth(seed, ec.m_ld_wl.data(), ec.m_ld_dist.data(), w, h)) {
            return false;
        }
    }
    if (through_step >= 7u && ec.m_ter != nullptr) {
        if (!save_shaped_ter(seed, "07_shaped_outline_merge.ppm", ec.m_ter, w, h)) {
            return false;
        }
    }
    if (through_step >= 8u && ec.m_ter != nullptr && ec.m_pts_que != nullptr && ec.m_pts_que->ok() && ec.m_pts_n > 0u) {
        if (!save_river_pts(seed, ec.m_ter, *ec.m_pts_que, w, h)) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
