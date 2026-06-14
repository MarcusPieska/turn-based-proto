//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_mtn_range.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static u32 tile_i (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_bounds (u16 w, u16 h, u16 x, u16 y) {
    return x < w && y < h;
}

static void q_push (u32* q, u32* qn, u32 v) {
    q[*qn] = v;
    (*qn)++;
}

static u32 q_pop (u32* q, u32* qh, u32 qn) {
    if (*qh >= qn) {
        return 0;
    }
    const u32 v = q[*qh];
    (*qh)++;
    return v;
}

static const u16 k_inf = 0xFFFFu;

static bool flood_comp (
    const u16* dist,
    u16 w,
    u16 h,
    u8* comp,
    u32 n,
    u32 seed,
    u32* q) 
{
    if (dist[seed] == k_inf) {
        return false;
    }
    u32 qn = 0;
    u32 qh = 0;
    q_push(q, &qn, seed);
    comp[seed] = 1;
    while (qh < qn) {
        const u32 i = q_pop(q, &qh, qn);
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (px > 0) {
            const u32 j = i - 1u;
            if (comp[j] == 0 && dist[j] != k_inf) {
                comp[j] = 1;
                q_push(q, &qn, j);
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (comp[j] == 0 && dist[j] != k_inf) {
                comp[j] = 1;
                q_push(q, &qn, j);
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (comp[j] == 0 && dist[j] != k_inf) {
                comp[j] = 1;
                q_push(q, &qn, j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (comp[j] == 0 && dist[j] != k_inf) {
                comp[j] = 1;
                q_push(q, &qn, j);
            }
        }
    }
    return true;
}

static bool on_band (
    const u8* comp,
    const u16* dist,
    u32 i,
    u16 td) 
{
    return comp[i] != 0 && dist[i] == td;
}

static u32 rng_u32 (u32* s) {
    *s = *s * 1103515245u + 12345u;
    return (*s >> 16u) & 0x7FFFu;
}

static u32 count_all_band (
    const u8* comp,
    const u16* dist,
    u32 n,
    u16 td) 
{
    u32 cnt = 0;
    for (u32 i = 0; i < n; ++i) {
        if (on_band(comp, dist, i, td)) {
            cnt++;
        }
    }
    return cnt;
}

static u32 pick_rand_band (
    const u8* comp,
    const u16* dist,
    u32 n,
    u16 td,
    u32 pick) 
{
    u32 seen = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!on_band(comp, dist, i, td)) {
            continue;
        }
        if (seen == pick) {
            return i;
        }
        seen++;
    }
    return 0;
}

static void band_push_nbrs8 (
    u16 w,
    u16 h,
    const u8* comp,
    const u16* dist,
    u16 td,
    u32 i,
    u8* vis,
    u32* q,
    u32* qn) 
{
    const u16 py = static_cast<u16>(i / static_cast<u32>(w));
    const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
    static const i8 k_dx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const i8 k_dy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (u32 k = 0; k < 8u; ++k) {
        const i32 nx = static_cast<i32>(px) + static_cast<i32>(k_dx[k]);
        const i32 ny = static_cast<i32>(py) + static_cast<i32>(k_dy[k]);
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
            continue;
        }
        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (vis[j] == 0 && on_band(comp, dist, j, td)) {
            vis[j] = 1;
            q_push(q, qn, j);
        }
    }
}

static u32 flood_band_pick (
    u16 w,
    u16 h,
    u32 n,
    const u8* comp,
    const u16* dist,
    u16 td,
    u32 start,
    u32 want_n,
    MtnRangePt* out,
    u32* q) 
{
    if (want_n == 0) {
        return 0;
    }
    u8* vis = new u8[n];
    if (vis == nullptr) {
        return 0;
    }
    for (u32 i = 0; i < n; ++i) {
        vis[i] = 0;
    }
    u32 qn = 0;
    u32 qh = 0;
    q_push(q, &qn, start);
    vis[start] = 1;
    u32 got = 0;
    while (qh < qn && got < want_n) {
        const u32 i = q_pop(q, &qh, qn);
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        out[got].x = px;
        out[got].y = py;
        got++;
        band_push_nbrs8(w, h, comp, dist, td, i, vis, q, &qn);
    }
    delete[] vis;
    return got;
}

//================================================================================================================================
//=> - Generate_MtnRange -
//================================================================================================================================

MtnRangeResult* Generate_MtnRange::generate (
    const u16* dist,
    u16 w,
    u16 h,
    i32 coverage_perc,
    i32 target_depth,
    u16 sx,
    u16 sy) 
{
    if (dist == nullptr || w == 0 || h == 0 || coverage_perc < 0 || coverage_perc > 100 || target_depth < 0) {
        return nullptr;
    }
    if (!in_bounds(w, h, sx, sy)) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 seed = tile_i(w, sx, sy);
    if (dist[seed] == k_inf) {
        return nullptr;
    }
    u8* comp = new u8[n];
    u32* q = new u32[n];
    if (comp == nullptr || q == nullptr) {
        delete[] comp;
        delete[] q;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        comp[i] = 0;
    }
    if (!flood_comp(dist, w, h, comp, n, seed, q)) {
        delete[] comp;
        delete[] q;
        return nullptr;
    }
    const u16 td = static_cast<u16>(target_depth);
    const u32 band_n = count_all_band(comp, dist, n, td);
    if (band_n == 0) {
        delete[] comp;
        delete[] q;
        MtnRangeResult* empty = new MtnRangeResult();
        if (empty == nullptr) {
            return nullptr;
        }
        empty->pts = nullptr;
        empty->pt_n = 0;
        return empty;
    }
    u32 want_n = (band_n * static_cast<u32>(coverage_perc) + 50u) / 100u;
    if (want_n == 0) {
        want_n = 1;
    }
    if (want_n > band_n) {
        want_n = band_n;
    }
    u32 rng = seed + static_cast<u32>(sx) * 7919u + static_cast<u32>(sy);
    const u32 pick = rng_u32(&rng) % band_n;
    const u32 band_start = pick_rand_band(comp, dist, n, td, pick);
    MtnRangeResult* out = new MtnRangeResult();
    if (out == nullptr) {
        delete[] comp;
        delete[] q;
        return nullptr;
    }
    out->pts = new MtnRangePt[want_n];
    if (out->pts == nullptr) {
        delete out;
        delete[] comp;
        delete[] q;
        return nullptr;
    }
    out->pt_n = flood_band_pick(w, h, n, comp, dist, td, band_start, want_n, out->pts, q);
    delete[] comp;
    delete[] q;
    return out;
}

void Generate_MtnRange::free_result (MtnRangeResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->pts;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
