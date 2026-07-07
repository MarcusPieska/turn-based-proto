//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstring>

#include "p1_gen_rain_orographic.h"
#include "game_map_defs.h"
#include "wb_que_xy.h"
#include "wb_sheet.h"
#include "whiteboard.h"

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const u16 k_dist_none = 0xFFFFu;
static const u16 k_ht_none = 0xFFFFu;
static const u16 k_ht_max = 10000u;
static const u32 k_wind_q = 16384u;
static const i16 s_inv_r2[8] = {8192, 16384, 8192, 16384, 16384, 8192, 16384, 8192};

static i16 s_wux[256];
static i16 s_wuy[256];
static bool s_wlut = false;

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static void init_wlut () {
    if (s_wlut) {
        return;
    }
    for (u32 i = 0; i < 256u; ++i) {
        const f32 ang = (static_cast<f32>(i) / 255.f) * 6.28318530f;
        s_wux[i] = static_cast<i16>(std::lrint(std::cos(ang) * static_cast<f64>(k_wind_q)));
        s_wuy[i] = static_cast<i16>(std::lrint(std::sin(ang) * static_cast<f64>(k_wind_q)));
    }
    s_wlut = true;
}

static u32 isqrt64 (u64 x) {
    if (x == 0u) {
        return 0u;
    }
    u64 r = x;
    u64 q = x + 1u;
    while (q < r) {
        r = q;
        q = (x / q + q) / 2u;
    }
    return static_cast<u32>(r);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_inland_water (u8 cls) {
    return cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool is_height_flood_cell (u8 cls) {
    return is_land(cls) || is_inland_water(cls);
}

static bool touches_glob_ocean (u16 w, u16 h, const u16* glob, u32 i) {
    const u16 py = static_cast<u16>(i / static_cast<u32>(w));
    const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
    if (px > 0) {
        const u32 j = i - 1u;
        if (glob[j] != 0) {
            return true;
        }
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
        const u32 j = i + 1u;
        if (glob[j] != 0) {
            return true;
        }
    }
    if (py > 0) {
        const u32 j = i - static_cast<u32>(w);
        if (glob[j] != 0) {
            return true;
        }
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
        const u32 j = i + static_cast<u32>(w);
        if (glob[j] != 0) {
            return true;
        }
    }
    return false;
}

static bool mark_global_ocean (u16 w, u16 h, const u8* terrain, u16* glob, WB_QueXY& q) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (glob == nullptr || terrain == nullptr || !q.ok()) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        glob[i] = 0;
    }
    q.clear();
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            if (px > 0 && px + 1u < w && py > 0 && py + 1u < h) {
                continue;
            }
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_water(terrain[i]) || glob[i] != 0) {
                continue;
            }
            glob[i] = 1;
            if (!q.push(px, py)) {
                return false;
            }
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_water(terrain[j]) && glob[j] == 0) {
                glob[j] = 1;
                if (!q.push(static_cast<u16>(px - 1), py)) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_water(terrain[j]) && glob[j] == 0) {
                glob[j] = 1;
                if (!q.push(static_cast<u16>(px + 1), py)) {
                    return false;
                }
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_water(terrain[j]) && glob[j] == 0) {
                glob[j] = 1;
                if (!q.push(px, static_cast<u16>(py - 1))) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_water(terrain[j]) && glob[j] == 0) {
                glob[j] = 1;
                if (!q.push(px, static_cast<u16>(py + 1))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool dist_flood_from_coast (u16 w, u16 h, const u8* terrain, u16* dist, u16* glob, WB_QueXY& q) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (glob == nullptr || dist == nullptr || terrain == nullptr || !q.ok()) {
        return false;
    }
    if (!mark_global_ocean(w, h, terrain, glob, q)) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_dist_none;
    }
    q.clear();
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land(terrain[i]) || !touches_glob_ocean(w, h, glob, i)) {
                continue;
            }
            dist[i] = 1;
            if (!q.push(px, py)) {
                return false;
            }
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u16 cur = dist[i];
        if (cur >= 30000u) {
            continue;
        }
        const bool on_mtn = is_mtn(terrain[i]);
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_height_flood_cell(terrain[j]) && dist[j] == k_dist_none && (!on_mtn || is_mtn(terrain[j]))) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px - 1), py)) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_height_flood_cell(terrain[j]) && dist[j] == k_dist_none && (!on_mtn || is_mtn(terrain[j]))) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px + 1), py)) {
                    return false;
                }
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_height_flood_cell(terrain[j]) && dist[j] == k_dist_none && (!on_mtn || is_mtn(terrain[j]))) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py - 1))) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_height_flood_cell(terrain[j]) && dist[j] == k_dist_none && (!on_mtn || is_mtn(terrain[j]))) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py + 1))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static bool dist_flood_from_mtn (u16 w, u16 h, const u8* terrain, u16* dist, WB_QueXY& q) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (dist == nullptr || terrain == nullptr || !q.ok()) {
        return false;
    }
    q.clear();
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_dist_none;
        if (is_mtn(terrain[i])) {
            dist[i] = 0;
            const u16 py = static_cast<u16>(i / static_cast<u32>(w));
            const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
            if (!q.push(px, py)) {
                return false;
            }
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u16 cur = dist[i];
        if (cur >= 30000u) {
            continue;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_height_flood_cell(terrain[j]) && !is_mtn(terrain[j]) && dist[j] == k_dist_none) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px - 1), py)) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_height_flood_cell(terrain[j]) && !is_mtn(terrain[j]) && dist[j] == k_dist_none) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px + 1), py)) {
                    return false;
                }
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_height_flood_cell(terrain[j]) && !is_mtn(terrain[j]) && dist[j] == k_dist_none) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py - 1))) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_height_flood_cell(terrain[j]) && !is_mtn(terrain[j]) && dist[j] == k_dist_none) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py + 1))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void norm_dist (u32 n, const u16* dist, u16* out) {
    u16 dmax = 0;
    for (u32 i = 0; i < n; ++i) {
        if (dist[i] != k_dist_none && dist[i] > dmax) {
            dmax = dist[i];
        }
    }
    if (dmax <= 0) {
        for (u32 i = 0; i < n; ++i) {
            out[i] = dist[i] != k_dist_none ? 0u : k_ht_none;
        }
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        if (dist[i] == k_dist_none) {
            out[i] = k_ht_none;
        } else {
            const u32 v = (static_cast<u32>(dist[i]) * static_cast<u32>(k_ht_max)) / static_cast<u32>(dmax);
            out[i] = static_cast<u16>(v > static_cast<u32>(k_ht_max) ? k_ht_max : v);
        }
    }
}

static void norm_dist_land (u32 n, const u16* dist, const u8* terrain, u16* out) {
    u16 dmax = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_land(terrain[i])) {
            continue;
        }
        if (dist[i] != k_dist_none && dist[i] > dmax) {
            dmax = dist[i];
        }
    }
    if (dmax <= 0) {
        for (u32 i = 0; i < n; ++i) {
            out[i] = dist[i] != k_dist_none ? 0u : k_ht_none;
        }
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        if (dist[i] == k_dist_none) {
            out[i] = k_ht_none;
        } else {
            const u32 v = (static_cast<u32>(dist[i]) * static_cast<u32>(k_ht_max)) / static_cast<u32>(dmax);
            out[i] = static_cast<u16>(v > static_cast<u32>(k_ht_max) ? k_ht_max : v);
        }
    }
}

static void blind_height (u32 n, const u16* h1, const u16* h2, u16* out) {
    for (u32 i = 0; i < n; ++i) {
        const u16 dv1 = h1[i];
        const u16 dv2 = h2[i];
        if (dv1 == k_ht_none && dv2 == k_ht_none) {
            out[i] = k_ht_none;
            continue;
        }
        if (dv1 == k_ht_none && dv2 != k_ht_none) {
            out[i] = k_ht_max;
            continue;
        }
        if (dv2 == k_ht_none && dv1 != k_ht_none) {
            out[i] = 0;
            continue;
        }
        u32 v = 0u;
        if (dv1 <= 1u && dv2 + 50u >= k_ht_max) {
            v = static_cast<u32>(k_ht_max) / 2u;
        } else {
            const u32 den = static_cast<u32>(dv1) + (static_cast<u32>(k_ht_max) - static_cast<u32>(dv2));
            if (den > 0u) {
                v = (static_cast<u32>(dv2) * static_cast<u32>(dv1)) / den;
            }
        }
        if (v > static_cast<u32>(k_ht_max)) {
            v = k_ht_max;
        }
        out[i] = static_cast<u16>(v);
    }
}

static u16 sample_ht (u16 w, u16 h, const u16* ht, u16 px, u16 py) {
    if (ht == nullptr || px >= w || py >= h) {
        return k_ht_none;
    }
    return ht[static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)];
}

static void height_slope (u16 w, u16 h, const u16* ht, u16* smag, i16* gx, i16* gy) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    static const i8 k_ox[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const i8 k_oy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (u32 i = 0; i < n; ++i) {
        smag[i] = 0;
        gx[i] = 0;
        gy[i] = 0;
    }
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            const u16 c = ht[i];
            if (c == k_ht_none) {
                continue;
            }
            i32 gx_sum = 0;
            i32 gy_sum = 0;
            u32 w_sum = 0;
            for (u8 k = 0; k < 8u; ++k) {
                const i32 nx = static_cast<i32>(px) + static_cast<i32>(k_ox[k]);
                const i32 ny = static_cast<i32>(py) + static_cast<i32>(k_oy[k]);
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 hv = sample_ht(w, h, ht, static_cast<u16>(nx), static_cast<u16>(ny));
                if (hv == k_ht_none) {
                    continue;
                }
                const i32 dz = static_cast<i32>(hv) - static_cast<i32>(c);
                gx_sum += (dz * static_cast<i32>(k_ox[k]) * static_cast<i32>(s_inv_r2[k])) >> 14;
                gy_sum += (dz * static_cast<i32>(k_oy[k]) * static_cast<i32>(s_inv_r2[k])) >> 14;
                ++w_sum;
            }
            if (w_sum == 0u) {
                continue;
            }
            gx[i] = static_cast<i16>(gx_sum / static_cast<i32>(w_sum));
            gy[i] = static_cast<i16>(gy_sum / static_cast<i32>(w_sum));
            const u64 mag2 = static_cast<u64>(gx[i]) * static_cast<u64>(gx[i])
                + static_cast<u64>(gy[i]) * static_cast<u64>(gy[i]);
            const u32 mag = isqrt64(mag2);
            smag[i] = mag > 65535u ? 65535u : static_cast<u16>(mag);
        }
    }
}

static void smooth_u16 (u16 w, u16 h, u16 pass_n, const u16* ht, u16* field, u16* tmp, u16* s_max_out) {
    if (pass_n == 0 || ht == nullptr || field == nullptr || tmp == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u16 pass = 0; pass < pass_n; ++pass) {
        for (u16 py = 0; py < h; ++py) {
            for (u16 px = 0; px < w; ++px) {
                const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
                if (ht[i] == k_ht_none) {
                    tmp[i] = 0;
                    continue;
                }
                u32 sum = 0;
                u32 cnt = 0;
                for (i32 oy = -1; oy <= 1; ++oy) {
                    for (i32 ox = -1; ox <= 1; ++ox) {
                        const i32 nx = static_cast<i32>(px) + ox;
                        const i32 ny = static_cast<i32>(py) + oy;
                        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                            continue;
                        }
                        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                        if (ht[j] == k_ht_none) {
                            continue;
                        }
                        sum += static_cast<u32>(field[j]);
                        ++cnt;
                    }
                }
                tmp[i] = cnt > 0u ? static_cast<u16>(sum / cnt) : 0;
            }
        }
        std::memcpy(field, tmp, static_cast<size_t>(n) * sizeof(u16));
    }
    if (s_max_out != nullptr) {
        u16 mx = 0;
        for (u32 i = 0; i < n; ++i) {
            if (ht[i] != k_ht_none && field[i] > mx) {
                mx = field[i];
            }
        }
        *s_max_out = mx;
    }
}

static void smooth_i16 (u16 w, u16 h, u16 pass_n, const u16* ht, i16* field, i16* tmp) {
    if (pass_n == 0 || ht == nullptr || field == nullptr || tmp == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u16 pass = 0; pass < pass_n; ++pass) {
        for (u16 py = 0; py < h; ++py) {
            for (u16 px = 0; px < w; ++px) {
                const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
                if (ht[i] == k_ht_none) {
                    tmp[i] = 0;
                    continue;
                }
                i32 sum = 0;
                u32 cnt = 0;
                for (i32 oy = -1; oy <= 1; ++oy) {
                    for (i32 ox = -1; ox <= 1; ++ox) {
                        const i32 nx = static_cast<i32>(px) + ox;
                        const i32 ny = static_cast<i32>(py) + oy;
                        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                            continue;
                        }
                        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                        if (ht[j] == k_ht_none) {
                            continue;
                        }
                        sum += static_cast<i32>(field[j]);
                        ++cnt;
                    }
                }
                tmp[i] = cnt > 0u ? static_cast<i16>(sum / static_cast<i32>(cnt)) : 0;
            }
        }
        std::memcpy(field, tmp, static_cast<size_t>(n) * sizeof(i16));
    }
}

static void smooth_u32 (u16 w, u16 h, u16 pass_n, const u16* ht, u32* field, u32* tmp) {
    if (pass_n == 0 || ht == nullptr || field == nullptr || tmp == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u16 pass = 0; pass < pass_n; ++pass) {
        for (u16 py = 0; py < h; ++py) {
            for (u16 px = 0; px < w; ++px) {
                const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
                if (ht[i] == k_ht_none) {
                    tmp[i] = 0;
                    continue;
                }
                u64 sum = 0;
                u32 cnt = 0;
                for (i32 oy = -1; oy <= 1; ++oy) {
                    for (i32 ox = -1; ox <= 1; ++ox) {
                        const i32 nx = static_cast<i32>(px) + ox;
                        const i32 ny = static_cast<i32>(py) + oy;
                        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                            continue;
                        }
                        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                        if (ht[j] == k_ht_none) {
                            continue;
                        }
                        sum += static_cast<u64>(field[j]);
                        ++cnt;
                    }
                }
                tmp[i] = cnt > 0u ? static_cast<u32>(sum / cnt) : 0u;
            }
        }
        std::memcpy(field, tmp, static_cast<size_t>(n) * sizeof(u32));
    }
}

static void pack_height_u8 (u32 n, const u16* ht, u8* out) {
    for (u32 i = 0; i < n; ++i) {
        if (ht[i] == k_ht_none) {
            out[i] = 0;
            continue;
        }
        const u32 v = (static_cast<u32>(ht[i]) * 255u + static_cast<u32>(k_ht_max) / 2u) / static_cast<u32>(k_ht_max);
        out[i] = static_cast<u8>(v > 255u ? 255u : v);
    }
}

static void build_gamma_lut (u8* lut, f32 gamma) {
    if (gamma < 0.1f) {
        gamma = 0.1f;
    }
    for (u32 i = 0; i < 256u; ++i) {
        const f32 t = static_cast<f32>(i) / 255.f;
        lut[i] = static_cast<u8>(std::lrint(std::pow(static_cast<f64>(t), static_cast<f64>(gamma)) * 255.0));
    }
}

static void pack_u16_u8 (u32 n, const u16* src, u8* out, f32 gamma, f32 peak) {
    u16 vmax = 0;
    for (u32 i = 0; i < n; ++i) {
        if (src[i] > vmax) {
            vmax = src[i];
        }
    }
    if (vmax == 0) {
        vmax = 1;
    }
    f32 denom = static_cast<f32>(vmax) * peak;
    if (denom < 1e-6f) {
        denom = static_cast<f32>(vmax);
    }
    u8 lut[256];
    build_gamma_lut(lut, gamma);
    for (u32 i = 0; i < n; ++i) {
        f32 t = static_cast<f32>(src[i]) / denom;
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        const u32 idx = static_cast<u32>(std::lrint(static_cast<f64>(t) * 255.0));
        out[i] = lut[idx > 255u ? 255u : idx];
    }
}

static void pack_u32_u8 (u32 n, const u32* src, u8* out, f32 gamma, f32 peak) {
    u32 vmax = 0;
    for (u32 i = 0; i < n; ++i) {
        if (src[i] > vmax) {
            vmax = src[i];
        }
    }
    if (vmax == 0u) {
        vmax = 1u;
    }
    f32 denom = static_cast<f32>(vmax) * peak;
    if (denom < 1e-6f) {
        denom = static_cast<f32>(vmax);
    }
    u8 lut[256];
    build_gamma_lut(lut, gamma);
    for (u32 i = 0; i < n; ++i) {
        f32 t = static_cast<f32>(src[i]) / denom;
        if (t < 0.f) {
            t = 0.f;
        }
        if (t > 1.f) {
            t = 1.f;
        }
        const u32 idx = static_cast<u32>(std::lrint(static_cast<f64>(t) * 255.0));
        out[i] = lut[idx > 255u ? 255u : idx];
    }
}

static u32 tile_rain (u8 wdir, u8 wstr, i16 gx, i16 gy, u16 smag, u16 s_max, u32 pf, u32 pa) {
    if (wstr == 0) {
        return 0u;
    }
    init_wlut();
    const i32 ux = s_wux[wdir];
    const i32 uy = s_wuy[wdir];
    const i64 dot = static_cast<i64>(ux) * static_cast<i64>(gx) + static_cast<i64>(uy) * static_cast<i64>(gy);
    const i32 up = static_cast<i32>(dot >> 14);
    if (up <= 0) {
        return 0u;
    }
    u32 r = 0u;
    const u32 ws = static_cast<u32>(wstr);
    if (pf > 0u && s_max > 0u) {
        const u64 t = static_cast<u64>(up) * static_cast<u64>(s_max) * ws * pf;
        r += static_cast<u32>(t / (255ull * 65535ull));
    }
    if (pa > 0u && smag > 0u) {
        const u64 t = static_cast<u64>(up) * static_cast<u64>(smag) * ws * pa;
        r += static_cast<u32>(t / (255ull * 65535ull));
    }
    return r;
}

//================================================================================================================================
//=> - P1_Gen_RainOrographic -
//================================================================================================================================

P1_Gen_RainOrographic::P1_Gen_RainOrographic (
    const P1_RunPrm& prm,
    u16 rain_finish,
    u16 slope_finish,
    const P1_Gen_RainOrographicPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_rain_finish(rain_finish),
    m_slope_finish(slope_finish),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RainOrographic::generate (
    const u8* terrain,
    const u8* wind_dir,
    const u8* wind_str,
    u16 w,
    u16 h) 
{
    m_valid_generation = false;
    m_rslt.m_rain.clear();
    m_rslt.m_flood_coast.clear();
    m_rslt.m_flood_mtn.clear();
    m_rslt.m_height.clear();
    m_rslt.m_slope.clear();
    if (terrain == nullptr || wind_dir == nullptr || wind_str == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 wb_n = static_cast<i32>(n * 2u);
    WbSheet sh_d1(wb_n);
    WbSheet sh_d2(wb_n);
    WbSheet sh_glob(wb_n);
    WbSheet sh_ht(wb_n);
    WbSheet sh_smag(wb_n);
    WbSheet sh_gx(wb_n);
    WbSheet sh_gy(wb_n);
    WbSheet sh_tmp(wb_n);
    WB_QueXY que(wb_n);
    if (!sh_d1.ok() || !sh_d2.ok() || !sh_glob.ok() || !sh_ht.ok() || !sh_smag.ok() || !sh_gx.ok() || !sh_gy.ok()
        || !sh_tmp.ok() || !que.ok()) {
        return false;
    }
    u16* h1 = sh_d1.get();
    u16* h2 = sh_d2.get();
    u16* ht = sh_ht.get();
    u16* smag = sh_smag.get();
    i16* gx = reinterpret_cast<i16*>(sh_gx.get());
    i16* gy = reinterpret_cast<i16*>(sh_gy.get());
    i16* gx_tmp = reinterpret_cast<i16*>(sh_tmp.get());
    if (!dist_flood_from_coast(w, h, terrain, h1, sh_glob.get(), que)
        || !dist_flood_from_mtn(w, h, terrain, h2, que)) {
        return false;
    }
    norm_dist(n, h1, h1);
    norm_dist_land(n, h2, terrain, h2);
    for (u32 i = 0; i < n; ++i) {
        if (h2[i] != k_ht_none) {
            h2[i] = static_cast<u16>(k_ht_max - h2[i]);
        }
    }
    if (!m_rslt.m_rain.resize(w, h)) {
        return false;
    }
    if (m_sp.m_pack_dbg != 0u) {
        if (!m_rslt.m_flood_coast.resize(w, h) || !m_rslt.m_flood_mtn.resize(w, h)
            || !m_rslt.m_height.resize(w, h) || !m_rslt.m_slope.resize(w, h)) {
            return false;
        }
    }
    blind_height(n, h1, h2, ht);
    if (m_sp.m_pack_dbg != 0u) {
        pack_height_u8(n, h1, m_rslt.m_flood_coast.data_w());
        pack_height_u8(n, h2, m_rslt.m_flood_mtn.data_w());
    }
    u32* rain = reinterpret_cast<u32*>(sh_d1.get());
    u32* rain_tmp = reinterpret_cast<u32*>(sh_d2.get());
    height_slope(w, h, ht, smag, gx, gy);
    smooth_i16(w, h, m_sp.m_smooth_n, ht, gx, gx_tmp);
    smooth_i16(w, h, m_sp.m_smooth_n, ht, gy, gx_tmp);
    u16 s_max = 0;
    smooth_u16(w, h, m_sp.m_smooth_n, ht, smag, sh_tmp.get(), &s_max);
    u32 pf = 0u;
    if (m_sp.m_slp_full > 0.0) {
        if (m_sp.m_slp_full > 1.0) {
            pf = 65535u;
        } else {
            pf = static_cast<u32>(m_sp.m_slp_full * 65535.0);
        }
    }
    const u32 pa = 65535u - pf;
    for (u32 i = 0; i < n; ++i) {
        rain[i] = 0u;
        if (!is_land(terrain[i]) || ht[i] == k_ht_none) {
            continue;
        }
        rain[i] = tile_rain(wind_dir[i], wind_str[i], gx[i], gy[i], smag[i], s_max, pf, pa);
    }
    smooth_u32(w, h, m_rain_finish, ht, rain, rain_tmp);
    if (m_sp.m_pack_dbg != 0u) {
        std::memcpy(sh_gy.get(), ht, static_cast<size_t>(n) * sizeof(u16));
        smooth_u16(w, h, m_sp.m_smooth_n, ht, sh_gy.get(), sh_tmp.get(), nullptr);
        pack_height_u8(n, sh_gy.get(), m_rslt.m_height.data_w());
        std::memcpy(sh_tmp.get(), smag, static_cast<size_t>(n) * sizeof(u16));
        smooth_u16(w, h, m_slope_finish, ht, sh_tmp.get(), sh_glob.get(), nullptr);
        pack_u16_u8(n, sh_tmp.get(), m_rslt.m_slope.data_w(), 0.55f, 1.f);
    }
    pack_u32_u8(n, rain, m_rslt.m_rain.data_w(), m_sp.m_gamma, m_sp.m_peak);
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RainOrographic::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RainOrographicRslt& P1_Gen_RainOrographic::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
