//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_latitude_overlay.h"

#include <cmath>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static f32 dist_to_equator (f32 px, f32 py, u16 w, u16 h) {
    if (w == 0 || h == 0) {
        return 0.f;
    }
    const f32 ax = 0.f;
    const f32 ay = static_cast<f32>(h) - 1.f;
    const f32 bx = static_cast<f32>(w) - 1.f;
    const f32 by = 0.f;
    const f32 abx = bx - ax;
    const f32 aby = by - ay;
    const f32 apx = px - ax;
    const f32 apy = py - ay;
    const f32 cross = abx * apy - aby * apx;
    const f32 ab_len = std::sqrt(abx * abx + aby * aby);
    if (ab_len <= 0.f) {
        return 0.f;
    }
    return std::fabs(cross) / ab_len;
}

static u8 scale_dist (f32 dist, f32 max_dist) {
    if (max_dist <= 0.f) {
        return 0;
    }
    f32 t = dist / max_dist;
    if (t < 0.f) {
        t = 0.f;
    }
    if (t > 1.f) {
        t = 1.f;
    }
    const u32 v = static_cast<u32>(t * 255.f + 0.5f);
    return static_cast<u8>(v > 255u ? 255u : v);
}

//================================================================================================================================
//=> - Generate_LatitudeOverlay -
//================================================================================================================================

u8* Generate_LatitudeOverlay::generate (u16 w, u16 h) {
    if (w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* out = new u8[n];
    if (out == nullptr) {
        return nullptr;
    }
    const f32 d_tl = dist_to_equator(0.5f, 0.5f, w, h);
    const f32 d_br = dist_to_equator(static_cast<f32>(w) - 0.5f, static_cast<f32>(h) - 0.5f, w, h);
    const f32 max_dist = (d_tl > d_br) ? d_tl : d_br;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const f32 cx = static_cast<f32>(px) + 0.5f;
            const f32 cy = static_cast<f32>(py) + 0.5f;
            const f32 d = dist_to_equator(cx, cy, w, h);
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            out[i] = static_cast<u8>(255u - static_cast<u32>(scale_dist(d, max_dist)));
        }
    }
    return out;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
