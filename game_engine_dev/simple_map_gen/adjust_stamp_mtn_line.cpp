//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>

#include "adjust_stamp_mtn_line.h"

#include "generate_small_shape.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_shape_land (u8 cls) {
    return !is_water(cls);
}

static i32 seg_angle_deg (i32 x0, i32 y0, i32 x1, i32 y1) {
    const f64 dx = static_cast<f64>(x1 - x0);
    const f64 dy = static_cast<f64>(y1 - y0);
    const f64 ang = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
    return static_cast<i32>(std::lround(ang));
}

static u16 seg_width (f64 dist) {
    u16 sw = static_cast<u16>(std::lround(dist));
    if (sw < 1u) {
        sw = 1u;
    }
    return sw;
}

static u16 seg_height (u16 width) {
    u16 sh = static_cast<u16>(std::lround(static_cast<f64>(width) * 2.0 / 5.0));
    if (sh < 1u) {
        sh = 1u;
    }
    return sh;
}

static u16 oversize_dim (u16 base, f64 fac, u16 add_px) {
    f64 v = static_cast<f64>(base) * fac + static_cast<f64>(add_px);
    i32 r = static_cast<i32>(std::lround(v));
    if (r < 1) {
        r = 1;
    }
    return static_cast<u16>(r);
}

static u32 stamp_shape (
    u8* terrain,
    u16 mw,
    u16 mh,
    const Generate_SmallShape& gen,
    i32 off_x,
    i32 off_y) 
{
    const u16 sw = gen.width();
    const u16 sh = gen.height();
    const u8* st = gen.terrain().data();
    if (st == nullptr || sw == 0 || sh == 0) {
        return 0;
    }
    u32 stamp_n = 0;
    for (u16 sy = 0; sy < sh; ++sy) {
        for (u16 sx = 0; sx < sw; ++sx) {
            const u8 cls = st[static_cast<u32>(sy) * static_cast<u32>(sw) + static_cast<u32>(sx)];
            if (!is_shape_land(cls)) {
                continue;
            }
            const i32 wx = off_x + static_cast<i32>(sx);
            const i32 wy = off_y + static_cast<i32>(sy);
            if (wx < 0 || wy < 0 || wx >= static_cast<i32>(mw) || wy >= static_cast<i32>(mh)) {
                continue;
            }
            const u32 wi = static_cast<u32>(wy) * static_cast<u32>(mw) + static_cast<u32>(wx);
            if (is_water(terrain[wi])) {
                continue;
            }
            terrain[wi] = TERR_MOUNTAINS[0];
            ++stamp_n;
        }
    }
    return stamp_n;
}

//================================================================================================================================
//=> - StampMtnLine -
//================================================================================================================================

Adjust_StampMtnLine::Adjust_StampMtnLine (u32 seed) :
    m_seed(seed),
    m_valid_adjust(false) {
}

u32 Adjust_StampMtnLine::stamp_dp (
    u8* terrain,
    u16 w,
    u16 h,
    const SmallAreaMtnLineDpResult* dp,
    const StampMtnLineParams& params) 
{
    m_valid_adjust = false;
    if (terrain == nullptr || w == 0 || h == 0 || dp == nullptr || dp->chains == nullptr) {
        return 0;
    }
    u32 stamp_px_n = 0;
    for (u16 ci = 0; ci < dp->chain_n; ++ci) {
        const SmallAreaMtnLineDpChain& ch = dp->chains[ci];
        if (ch.pts == nullptr || ch.pt_n < 2u) {
            continue;
        }
        for (u16 pi = 0; pi + 1u < ch.pt_n; ++pi) {
            const i32 x0 = ch.pts[pi].x;
            const i32 y0 = ch.pts[pi].y;
            const i32 x1 = ch.pts[pi + 1u].x;
            const i32 y1 = ch.pts[pi + 1u].y;
            const f64 dx = static_cast<f64>(x1 - x0);
            const f64 dy = static_cast<f64>(y1 - y0);
            const f64 dist = std::sqrt(dx * dx + dy * dy);
            if (dist < 1.0) {
                continue;
            }
            const u16 base_sw = seg_width(dist);
            const u16 base_sh = seg_height(base_sw);
            const u16 sw = oversize_dim(base_sw, params.m_oversize_factor, params.m_oversize_px);
            const u16 sh = oversize_dim(base_sh, params.m_oversize_factor, params.m_oversize_px);
            const i32 mid_x = (x0 + x1) / 2;
            const i32 mid_y = (y0 + y1) / 2;
            const i32 ang = seg_angle_deg(x0, y0, x1, y1);
            SmallShapeParams sp = {};
            sp.m_seed = m_seed ^ (static_cast<u32>(ci) * 73856093u) ^ (static_cast<u32>(pi) * 19349663u);
            sp.m_width = sw;
            sp.m_height = sh;
            sp.m_angle_deg = ang;
            Generate_SmallShape shape(sp);
            if (!shape.generate() || !shape.is_valid()) {
                continue;
            }
            const i32 off_x = mid_x - static_cast<i32>(shape.width()) / 2;
            const i32 off_y = mid_y - static_cast<i32>(shape.height()) / 2;
            stamp_px_n += stamp_shape(terrain, w, h, shape, off_x, off_y);
        }
    }
    if (stamp_px_n > 0) {
        m_valid_adjust = true;
    }
    return stamp_px_n;
}

bool Adjust_StampMtnLine::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
