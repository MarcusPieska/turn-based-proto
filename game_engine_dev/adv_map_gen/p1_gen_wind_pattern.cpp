//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_wind_pattern.h"

#include "game_map_defs.h"
#include "perlin_noise.h"
#include "wb_que_xy.h"
#include "wb_sheet.h"
#include "whiteboard.h"

#include <cmath>
#include <cstring>

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const f32 k_tau = 6.28318530f;
static const u16 k_inf = 0xFFFFu;

//================================================================================================================================
//=> - Private terrain helpers -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool is_hill (u8 cls) {
    return cls == TERR_HILLS[0];
}

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

static f32 equator_side (f32 px, f32 py, u16 w, u16 h) {
    if (w == 0 || h == 0) {
        return 1.f;
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
    return cross >= 0.f ? 1.f : -1.f;
}

static void lat_wind (f32 px, f32 py, u16 w, u16 h, f32* out_vx, f32* out_vy, f32* out_mag) {
    const f32 max_d = dist_to_equator(0.f, 0.f, w, h);
    f32 lat = dist_to_equator(px, py, w, h);
    if (max_d > 0.f) {
        lat /= max_d;
    }
    if (lat < 0.f) {
        lat = 0.f;
    }
    if (lat > 1.f) {
        lat = 1.f;
    }
    const f32 side = equator_side(px, py, w, h);
    f32 vx = 0.f;
    f32 vy = 0.f;
    f32 mag = 0.7f;
    if (lat < 0.33f) {
        vx = -1.f;
        vy = 0.35f * side;
        mag = 0.65f + 0.15f * (1.f - lat / 0.33f);
    } else if (lat < 0.66f) {
        vx = 1.f;
        vy = 0.12f * side;
        mag = 0.85f;
    } else {
        vx = -0.85f;
        vy = -0.25f * side;
        mag = 0.55f + 0.15f * ((lat - 0.66f) / 0.34f);
    }
    const f32 len = std::sqrt(vx * vx + vy * vy);
    if (len > 1e-6f) {
        vx /= len;
        vy /= len;
    }
    if (out_vx != nullptr) {
        *out_vx = vx;
    }
    if (out_vy != nullptr) {
        *out_vy = vy;
    }
    if (out_mag != nullptr) {
        *out_mag = mag;
    }
}

static u8 enc_dir (f32 vx, f32 vy) {
    f32 ang = std::atan2(vy, vx);
    if (ang < 0.f) {
        ang += k_tau;
    }
    const f32 t = ang / k_tau;
    i32 v = static_cast<i32>(std::lrint(static_cast<f64>(t) * 255.0));
    if (v < 0) {
        v = 0;
    }
    if (v > 255) {
        v = 255;
    }
    return static_cast<u8>(v);
}

static u8 enc_str (f32 mag) {
    if (mag < 0.f) {
        mag = 0.f;
    }
    if (mag > 1.f) {
        mag = 1.f;
    }
    return static_cast<u8>(std::lrint(static_cast<f64>(mag) * 255.0));
}

static void rot_vec (f32* vx, f32* vy, f32 rad) {
    if (vx == nullptr || vy == nullptr) {
        return;
    }
    const f32 c = std::cos(rad);
    const f32 s = std::sin(rad);
    const f32 x = *vx;
    const f32 y = *vy;
    *vx = x * c - y * s;
    *vy = x * s + y * c;
}

static void norm_vec (f32* vx, f32* vy) {
    if (vx == nullptr || vy == nullptr) {
        return;
    }
    const f32 len = std::sqrt((*vx) * (*vx) + (*vy) * (*vy));
    if (len > 1e-6f) {
        *vx /= len;
        *vy /= len;
    }
}

static bool bfs_dist_from_mtn (u16 w, u16 h, const u8* terrain, u16* dist, WB_QueXY& q) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (dist == nullptr || !q.ok()) {
        return false;
    }
    q.clear();
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
    }
    for (u32 i = 0; i < n; ++i) {
        if (!is_mtn(terrain[i])) {
            continue;
        }
        dist[i] = 0;
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (!q.push(px, py)) {
            return false;
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u16 cur = dist[i];
        if (cur >= 65534u) {
            continue;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        for (i32 oy = -1; oy <= 1; ++oy) {
            for (i32 ox = -1; ox <= 1; ++ox) {
                if (ox == 0 && oy == 0) {
                    continue;
                }
                const i32 nx = static_cast<i32>(px) + ox;
                const i32 ny = static_cast<i32>(py) + oy;
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (dist[j] != k_inf) {
                    continue;
                }
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static void mtn_grad (u16 w, u16 h, const u16* dist, u16 px, u16 py, f32* gx, f32* gy) {
    f32 dx = 0.f;
    f32 dy = 0.f;
    const u32 i0 = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
    const u16 d0 = dist[i0];
    if (d0 == k_inf) {
        if (gx != nullptr) {
            *gx = 0.f;
        }
        if (gy != nullptr) {
            *gy = 0.f;
        }
        return;
    }
    if (px > 0) {
        const u16 d1 = dist[i0 - 1u];
        if (d1 != k_inf && d1 < d0) {
            dx -= 1.f;
        }
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
        const u16 d1 = dist[i0 + 1u];
        if (d1 != k_inf && d1 < d0) {
            dx += 1.f;
        }
    }
    if (py > 0) {
        const u16 d1 = dist[i0 - static_cast<u32>(w)];
        if (d1 != k_inf && d1 < d0) {
            dy -= 1.f;
        }
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
        const u16 d1 = dist[i0 + static_cast<u32>(w)];
        if (d1 != k_inf && d1 < d0) {
            dy += 1.f;
        }
    }
    if (gx != nullptr) {
        *gx = dx;
    }
    if (gy != nullptr) {
        *gy = dy;
    }
}

static void water_bias (u16 w, u16 h, const u8* terrain, u16 px, u16 py, f32* vx, f32* vy) {
    i32 wcnt = 0;
    for (i32 oy = -1; oy <= 1; ++oy) {
        for (i32 ox = -1; ox <= 1; ++ox) {
            if (ox == 0 && oy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + ox;
            const i32 ny = static_cast<i32>(py) + oy;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (is_water(terrain[j])) {
                wcnt++;
            }
        }
    }
    if (wcnt > 0) {
        rot_vec(vx, vy, 0.22f * static_cast<f32>(wcnt));
    }
}

static void build_lat_field (u16 w, u16 h, f32* vx, f32* vy, f32* mag) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        lat_wind(static_cast<f32>(px), static_cast<f32>(py), w, h, &vx[i], &vy[i], &mag[i]);
    }
}

static void apply_terrain (
    u16 w,
    u16 h,
    const u8* terrain,
    const u16* mtn_dist,
    u32 seed,
    f32* vx,
    f32* vy,
    f32* mag) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    PerlinNoise pn(seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = terrain[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (is_mtn(cls)) {
            mag[i] *= 0.18f;
            f32 gx = 0.f;
            f32 gy = 0.f;
            mtn_grad(w, h, mtn_dist, px, py, &gx, &gy);
            if (std::fabs(gx) + std::fabs(gy) > 0.f) {
                vx[i] = -gy;
                vy[i] = gx;
                norm_vec(&vx[i], &vy[i]);
            }
        } else if (is_hill(cls)) {
            mag[i] *= 0.72f;
        }
        if (is_water(cls)) {
            mag[i] *= 1.12f;
            if (mag[i] > 1.f) {
                mag[i] = 1.f;
            }
        } else if (is_land(cls)) {
            water_bias(w, h, terrain, px, py, &vx[i], &vy[i]);
            norm_vec(&vx[i], &vy[i]);
            const u16 d = mtn_dist[i];
            if (d != k_inf && d > 0 && d < 28) {
                const f32 lee = static_cast<f32>(d) / 28.f;
                mag[i] *= 0.25f + 0.75f * lee;
                f32 gx = 0.f;
                f32 gy = 0.f;
                mtn_grad(w, h, mtn_dist, px, py, &gx, &gy);
                if (std::fabs(gx) + std::fabs(gy) > 0.f) {
                    rot_vec(&vx[i], &vy[i], 0.35f * (1.f - lee));
                }
            }
        }
        const f32 nx = static_cast<f32>(px) * 0.01f;
        const f32 ny = static_cast<f32>(py) * 0.01f;
        const f32 perturb = pn.noise2(nx, ny) * 0.14f;
        rot_vec(&vx[i], &vy[i], perturb);
        norm_vec(&vx[i], &vy[i]);
    }
}

static void smooth_field (
    u16 w,
    u16 h,
    u8 pass_n,
    f32* vx,
    f32* vy,
    f32* mag,
    f32* tmp) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (tmp == nullptr) {
        return;
    }
    for (u8 pass = 0; pass < pass_n; ++pass) {
        for (u32 i = 0; i < n; ++i) {
            const u16 py = static_cast<u16>(i / static_cast<u32>(w));
            const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
            f32 sx = 0.f;
            f32 cnt = 0.f;
            for (i32 oy = -1; oy <= 1; ++oy) {
                for (i32 ox = -1; ox <= 1; ++ox) {
                    const i32 nx = static_cast<i32>(px) + ox;
                    const i32 ny = static_cast<i32>(py) + oy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                        continue;
                    }
                    const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    sx += vx[j];
                    cnt += 1.f;
                }
            }
            tmp[i] = cnt > 0.f ? sx / cnt : vx[i];
        }
        std::memcpy(vx, tmp, static_cast<size_t>(n) * sizeof(f32));
        for (u32 i = 0; i < n; ++i) {
            const u16 py = static_cast<u16>(i / static_cast<u32>(w));
            const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
            f32 sy = 0.f;
            f32 cnt = 0.f;
            for (i32 oy = -1; oy <= 1; ++oy) {
                for (i32 ox = -1; ox <= 1; ++ox) {
                    const i32 nx = static_cast<i32>(px) + ox;
                    const i32 ny = static_cast<i32>(py) + oy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                        continue;
                    }
                    const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    sy += vy[j];
                    cnt += 1.f;
                }
            }
            tmp[i] = cnt > 0.f ? sy / cnt : vy[i];
        }
        std::memcpy(vy, tmp, static_cast<size_t>(n) * sizeof(f32));
        for (u32 i = 0; i < n; ++i) {
            const u16 py = static_cast<u16>(i / static_cast<u32>(w));
            const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
            f32 sm = 0.f;
            f32 cnt = 0.f;
            for (i32 oy = -1; oy <= 1; ++oy) {
                for (i32 ox = -1; ox <= 1; ++ox) {
                    const i32 nx = static_cast<i32>(px) + ox;
                    const i32 ny = static_cast<i32>(py) + oy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                        continue;
                    }
                    const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    sm += mag[j];
                    cnt += 1.f;
                }
            }
            mag[i] = cnt > 0.f ? sm / cnt : mag[i];
            norm_vec(&vx[i], &vy[i]);
        }
    }
}

static void pack_overlays (u32 n, const f32* vx, const f32* vy, const f32* mag, u8* dir, u8* str) {
    for (u32 i = 0; i < n; ++i) {
        dir[i] = enc_dir(vx[i], vy[i]);
        str[i] = enc_str(mag[i]);
    }
}

//================================================================================================================================
//=> - P1_Gen_WindPattern -
//================================================================================================================================

P1_Gen_WindPattern::P1_Gen_WindPattern (const P1_RunPrm& prm, const P1_Gen_WindPatternPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_WindPattern::generate () {
    return gen_core(nullptr, m_prm.m_w, m_prm.m_h, false);
}

bool P1_Gen_WindPattern::generate (const u8* terrain, u16 w, u16 h) {
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    return gen_core(terrain, w, h, true);
}

bool P1_Gen_WindPattern::gen_core (const u8* terrain, u16 w, u16 h, bool use_ter) {
    m_valid_generation = false;
    m_rslt.m_dir.clear();
    m_rslt.m_str.clear();
    if (!p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 wb_n = static_cast<i32>(n * 2u);
    WbSheet sh_vx(wb_n);
    WbSheet sh_vy(wb_n);
    WbSheet sh_mag(wb_n);
    WbSheet sh_tmp(wb_n);
    WbSheet sh_mtn(wb_n);
    WB_QueXY que(wb_n);
    if (!sh_vx.ok() || !sh_vy.ok() || !sh_mag.ok() || !sh_tmp.ok()) {
        return false;
    }
    f32* vx = reinterpret_cast<f32*>(sh_vx.get());
    f32* vy = reinterpret_cast<f32*>(sh_vy.get());
    f32* mag = reinterpret_cast<f32*>(sh_mag.get());
    f32* tmp = reinterpret_cast<f32*>(sh_tmp.get());
    build_lat_field(w, h, vx, vy, mag);
    if (use_ter) {
        if (!sh_mtn.ok() || !que.ok() || !bfs_dist_from_mtn(w, h, terrain, sh_mtn.get(), que)) {
            return false;
        }
        apply_terrain(w, h, terrain, sh_mtn.get(), m_prm.m_seed, vx, vy, mag);
    }
    smooth_field(w, h, m_sp.m_smooth_n, vx, vy, mag, tmp);
    if (!m_rslt.m_dir.resize(w, h) || !m_rslt.m_str.resize(w, h)) {
        return false;
    }
    pack_overlays(n, vx, vy, mag, m_rslt.m_dir.data_w(), m_rslt.m_str.data_w());
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_WindPattern::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_WindPatternRslt& P1_Gen_WindPattern::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
