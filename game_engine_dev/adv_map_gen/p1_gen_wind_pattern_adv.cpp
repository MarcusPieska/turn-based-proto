//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_wind_pattern_adv.h"

#include "game_map_defs.h"
#include "wb_sheet.h"
#include "whiteboard.h"

#include <cmath>
#include <cstring>

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const f32 k_tau = 6.28318530f;
static const u8 k_blk_water = 8u;
static const u8 k_blk_plain = 18u;
static const u8 k_blk_hill = 140u;
static const u8 k_blk_mtn = 255u;
static const u8 k_upwind_n = 36u;
static const f32 k_upwind_step = 1.2f;

static u8 tile_block (u8 cls);

static u32 gidx (u16 gx, u16 gy, u16 gw);

static f32 blk_drag (f32 b);

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

static void build_tile_block (const u8* terrain, u32 n, u16* blk) {
    if (terrain == nullptr || blk == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        blk[i] = tile_block(terrain[i]);
    }
}

static f32 sample_block_c (u16 gw, u16 gh, const u8* block, f32 fx, f32 fy) {
    if (block == nullptr || gw == 0 || gh == 0) {
        return 0.f;
    }
    if (fx < 0.f || fy < 0.f || fx > static_cast<f32>(gw) - 1.001f || fy > static_cast<f32>(gh) - 1.001f) {
        return 0.f;
    }
    const i32 x0 = static_cast<i32>(fx);
    const i32 y0 = static_cast<i32>(fy);
    const i32 x1 = x0 < static_cast<i32>(gw) - 1 ? x0 + 1 : x0;
    const i32 y1 = y0 < static_cast<i32>(gh) - 1 ? y0 + 1 : y0;
    const f32 tx = fx - static_cast<f32>(x0);
    const f32 ty = fy - static_cast<f32>(y0);
    const u32 i00 = gidx(static_cast<u16>(x0), static_cast<u16>(y0), gw);
    const u32 i10 = gidx(static_cast<u16>(x1), static_cast<u16>(y0), gw);
    const u32 i01 = gidx(static_cast<u16>(x0), static_cast<u16>(y1), gw);
    const u32 i11 = gidx(static_cast<u16>(x1), static_cast<u16>(y1), gw);
    const f32 b00 = static_cast<f32>(block[i00]);
    const f32 b10 = static_cast<f32>(block[i10]);
    const f32 b01 = static_cast<f32>(block[i01]);
    const f32 b11 = static_cast<f32>(block[i11]);
    return b00 * (1.f - tx) * (1.f - ty) + b10 * tx * (1.f - ty) + b01 * (1.f - tx) * ty + b11 * tx * ty;
}

static void downsample_wind (
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    const f32* vx,
    const f32* vy,
    f32* cvx,
    f32* cvy) 
{
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    for (u32 gi = 0; gi < gn; ++gi) {
        cvx[gi] = 0.f;
        cvy[gi] = 0.f;
    }
    for (u16 cy = 0; cy < gh; ++cy) {
        for (u16 cx = 0; cx < gw; ++cx) {
            const u16 x0 = static_cast<u16>(cx * chunk_sz);
            const u16 y0 = static_cast<u16>(cy * chunk_sz);
            const u16 x1 = static_cast<u16>(x0 + chunk_sz > w ? w : x0 + chunk_sz);
            const u16 y1 = static_cast<u16>(y0 + chunk_sz > h ? h : y0 + chunk_sz);
            f32 sx = 0.f;
            f32 sy = 0.f;
            u32 cnt = 0u;
            for (u16 py = y0; py < y1; ++py) {
                for (u16 px = x0; px < x1; ++px) {
                    const u32 ti = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
                    sx += vx[ti];
                    sy += vy[ti];
                    ++cnt;
                }
            }
            const u32 gi = gidx(cx, cy, gw);
            if (cnt > 0u) {
                cvx[gi] = sx / static_cast<f32>(cnt);
                cvy[gi] = sy / static_cast<f32>(cnt);
            }
        }
    }
}

static void apply_upwind_lee_coarse (
    u16 gw,
    u16 gh,
    u16 chunk_sz,
    const u8* block,
    const f32* cvx,
    const f32* cvy,
    f32* shl) 
{
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    const f32 step = k_upwind_step / static_cast<f32>(chunk_sz > 0 ? chunk_sz : 1);
    for (u32 gi = 0; gi < gn; ++gi) {
        const u16 gy = static_cast<u16>(gi / static_cast<u32>(gw));
        const u16 gx = static_cast<u16>(gi - static_cast<u32>(gy) * static_cast<u32>(gw));
        f32 uvx = cvx[gi];
        f32 uvy = cvy[gi];
        norm_vec(&uvx, &uvy);
        if (std::fabs(uvx) + std::fabs(uvy) < 1e-6f) {
            shl[gi] = 0.2f;
            continue;
        }
        f32 fx = static_cast<f32>(gx) + 0.5f;
        f32 fy = static_cast<f32>(gy) + 0.5f;
        f32 cost = 0.f;
        for (u8 s = 0; s < k_upwind_n; ++s) {
            fx -= uvx * step;
            fy -= uvy * step;
            cost += blk_drag(sample_block_c(gw, gh, block, fx, fy));
        }
        f32 shelter = 1.f - cost;
        if (shelter < 0.05f) {
            shelter = 0.05f;
        }
        if (shelter > 1.f) {
            shelter = 1.f;
        }
        shl[gi] = shelter;
    }
}

static void apply_coarse_shelter (
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    const f32* shl,
    f32* mag) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 cx = static_cast<u16>(px / chunk_sz);
        const u16 cy = static_cast<u16>(py / chunk_sz);
        const u16 cx1 = cx < gw - 1 ? static_cast<u16>(cx + 1) : cx;
        const u16 cy1 = cy < gh - 1 ? static_cast<u16>(cy + 1) : cy;
        const f32 fx = static_cast<f32>(px % chunk_sz) / static_cast<f32>(chunk_sz);
        const f32 fy = static_cast<f32>(py % chunk_sz) / static_cast<f32>(chunk_sz);
        const u32 i00 = gidx(cx, cy, gw);
        const u32 i10 = gidx(cx1, cy, gw);
        const u32 i01 = gidx(cx, cy1, gw);
        const u32 i11 = gidx(cx1, cy1, gw);
        const f32 s00 = shl[i00];
        const f32 s10 = shl[i10];
        const f32 s01 = shl[i01];
        const f32 s11 = shl[i11];
        const f32 s = s00 * (1.f - fx) * (1.f - fy) + s10 * fx * (1.f - fy) + s01 * (1.f - fx) * fy + s11 * fx * fy;
        mag[i] *= s;
    }
}

static f32 blk_drag (f32 b) {
    if (b >= static_cast<f32>(k_blk_mtn) - 12.f) {
        return 0.16f;
    }
    if (b >= static_cast<f32>(k_blk_hill) - 12.f) {
        return 0.07f;
    }
    if (b >= static_cast<f32>(k_blk_plain) + 20.f) {
        return 0.02f;
    }
    return 0.f;
}

static void block_grad_f (u16 w, u16 h, const u16* blk, u16 px, u16 py, f32* gx, f32* gy) {
    f32 dx = 0.f;
    f32 dy = 0.f;
    const u32 i0 = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
    const f32 c = static_cast<f32>(blk[i0]);
    if (px > 0) {
        const f32 d = static_cast<f32>(blk[i0 - 1u]);
        dx -= c - d;
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
        const f32 d = static_cast<f32>(blk[i0 + 1u]);
        dx += d - c;
    }
    if (py > 0) {
        const f32 d = static_cast<f32>(blk[i0 - static_cast<u32>(w)]);
        dy -= c - d;
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
        const f32 d = static_cast<f32>(blk[i0 + static_cast<u32>(w)]);
        dy += d - c;
    }
    if (gx != nullptr) {
        *gx = dx;
    }
    if (gy != nullptr) {
        *gy = dy;
    }
}

static void apply_ter_deflect (u16 w, u16 h, const u16* blk, f32* vx, f32* vy) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u8 b = static_cast<u8>(blk[i]);
        if (b >= k_blk_mtn - 8u) {
            vx[i] = 0.f;
            vy[i] = 0.f;
            continue;
        }
        f32 gx = 0.f;
        f32 gy = 0.f;
        block_grad_f(w, h, blk, px, py, &gx, &gy);
        const f32 gl = std::sqrt(gx * gx + gy * gy);
        if (gl < 1e-4f) {
            continue;
        }
        const f32 nx = gx / gl;
        const f32 ny = gy / gl;
        const f32 vdn = vx[i] * nx + vy[i] * ny;
        if (vdn > 0.f) {
            const f32 k = 0.88f * (static_cast<f32>(b) / 255.f);
            vx[i] -= vdn * nx * k;
            vy[i] -= vdn * ny * k;
        }
        if (b >= k_blk_hill - 10u) {
            const f32 tx = -ny;
            const f32 ty = nx;
            const f32 vdt = vx[i] * tx + vy[i] * ty;
            vx[i] = tx * vdt;
            vy[i] = ty * vdt;
        }
        norm_vec(&vx[i], &vy[i]);
    }
}

static void sync_mag (u32 n, const f32* vx, const f32* vy, f32* mag) {
    for (u32 i = 0; i < n; ++i) {
        mag[i] = std::sqrt(vx[i] * vx[i] + vy[i] * vy[i]);
    }
}

//================================================================================================================================
//=> - Private terrain helpers -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static u8 tile_block (u8 cls) {
    if (cls == TERR_MOUNTAINS[0]) {
        return k_blk_mtn;
    }
    if (cls == TERR_HILLS[0]) {
        return k_blk_hill;
    }
    if (is_water(cls)) {
        return k_blk_water;
    }
    if (cls == TERR_PLAINS[0]) {
        return k_blk_plain;
    }
    return k_blk_plain;
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

static f32 smooth01 (f32 t) {
    if (t <= 0.f) {
        return 0.f;
    }
    if (t >= 1.f) {
        return 1.f;
    }
    return t * t * (3.f - 2.f * t);
}

static f32 smooth_band (f32 lo, f32 hi, f32 x) {
    if (hi <= lo) {
        return x >= hi ? 1.f : 0.f;
    }
    return smooth01((x - lo) / (hi - lo));
}

static void lat_wind (f32 px, f32 py, u16 w, u16 h, f32* out_vx, f32* out_vy) {
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
    const f32 blend = 0.14f;
    const f32 e0 = 0.33f;
    const f32 e1 = 0.66f;
    f32 tvx = -1.f;
    f32 tvy = 0.35f * side;
    f32 wvx = 1.f;
    f32 wvy = 0.12f * side;
    f32 pvx = -0.85f;
    f32 pvy = -0.25f * side;
    f32 wt = 1.f - smooth_band(e0 - blend, e0 + blend, lat);
    f32 wp = smooth_band(e1 - blend, e1 + blend, lat);
    f32 ww = 1.f - wt - wp;
    if (ww < 0.f) {
        ww = 0.f;
    }
    const f32 wsum = wt + ww + wp;
    if (wsum > 1e-6f) {
        wt /= wsum;
        ww /= wsum;
        wp /= wsum;
    }
    f32 vx = tvx * wt + wvx * ww + pvx * wp;
    f32 vy = tvy * wt + wvy * ww + pvy * wp;
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

static void grid_dims (u16 w, u16 h, u16 chunk_sz, u16* gw_out, u16* gh_out) {
    const u16 gw = static_cast<u16>((static_cast<u32>(w) + static_cast<u32>(chunk_sz) - 1u)
        / static_cast<u32>(chunk_sz));
    const u16 gh = static_cast<u16>((static_cast<u32>(h) + static_cast<u32>(chunk_sz) - 1u)
        / static_cast<u32>(chunk_sz));
    if (gw_out != nullptr) {
        *gw_out = gw;
    }
    if (gh_out != nullptr) {
        *gh_out = gh;
    }
}

static u32 gidx (u16 gx, u16 gy, u16 gw) {
    return static_cast<u32>(gy) * static_cast<u32>(gw) + static_cast<u32>(gx);
}

static void chunk_center_px (
    u16 cx,
    u16 cy,
    u16 chunk_sz,
    u16 w,
    u16 h,
    f32* px_out,
    f32* py_out) 
{
    u32 px_u = static_cast<u32>(cx) * static_cast<u32>(chunk_sz) + static_cast<u32>(chunk_sz) / 2u;
    u32 py_u = static_cast<u32>(cy) * static_cast<u32>(chunk_sz) + static_cast<u32>(chunk_sz) / 2u;
    if (px_u >= static_cast<u32>(w)) {
        px_u = static_cast<u32>(w) - 1u;
    }
    if (py_u >= static_cast<u32>(h)) {
        py_u = static_cast<u32>(h) - 1u;
    }
    if (px_out != nullptr) {
        *px_out = static_cast<f32>(px_u);
    }
    if (py_out != nullptr) {
        *py_out = static_cast<f32>(py_u);
    }
}

static bool build_chunk_block (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    u8* block) 
{
    if (terrain == nullptr || block == nullptr || chunk_sz == 0) {
        return false;
    }
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    for (u32 gi = 0; gi < gn; ++gi) {
        block[gi] = 0;
    }
    for (u16 cy = 0; cy < gh; ++cy) {
        for (u16 cx = 0; cx < gw; ++cx) {
            u32 sum = 0;
            u32 cnt = 0;
            const u16 x0 = static_cast<u16>(cx * chunk_sz);
            const u16 y0 = static_cast<u16>(cy * chunk_sz);
            const u16 x1 = static_cast<u16>(x0 + chunk_sz > w ? w : x0 + chunk_sz);
            const u16 y1 = static_cast<u16>(y0 + chunk_sz > h ? h : y0 + chunk_sz);
            for (u16 py = y0; py < y1; ++py) {
                for (u16 px = x0; px < x1; ++px) {
                    const u32 ti = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
                    sum += static_cast<u32>(tile_block(terrain[ti]));
                    cnt++;
                }
            }
            if (cnt == 0) {
                return false;
            }
            const u32 gi = gidx(cx, cy, gw);
            block[gi] = static_cast<u8>(sum / cnt);
        }
    }
    return true;
}

static void build_open_block (u16 gw, u16 gh, u8* block) {
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    for (u32 i = 0; i < gn; ++i) {
        block[i] = k_blk_water;
    }
}

static void build_lat_grid (
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    f32* tvx,
    f32* tvy) 
{
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    for (u32 gi = 0; gi < gn; ++gi) {
        const u16 gy = static_cast<u16>(gi / static_cast<u32>(gw));
        const u16 gx = static_cast<u16>(gi - static_cast<u32>(gy) * static_cast<u32>(gw));
        f32 px = 0.f;
        f32 py = 0.f;
        chunk_center_px(gx, gy, chunk_sz, w, h, &px, &py);
        lat_wind(px, py, w, h, &tvx[gi], &tvy[gi]);
    }
}

static f32 block_open (u8 blk) {
    return 1.f - static_cast<f32>(blk) / 255.f;
}

static void apply_block_vel (u16 gw, u16 gh, const u8* block, f32* vx, f32* vy) {
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    for (u32 i = 0; i < gn; ++i) {
        const f32 open = block_open(block[i]);
        vx[i] *= open;
        vy[i] *= open;
    }
}

static void diffuse_grid (
    u16 gw,
    u16 gh,
    const u8* block,
    f32 k,
    f32* vx,
    f32* vy,
    f32* nx,
    f32* ny) 
{
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    if (nx == nullptr || ny == nullptr) {
        return;
    }
    for (u16 gy = 0; gy < gh; ++gy) {
        for (u16 gx = 0; gx < gw; ++gx) {
            const u32 i = gidx(gx, gy, gw);
            f32 sx = vx[i];
            f32 sy = vy[i];
            f32 cnt = 1.f;
            if (gx > 0) {
                const u32 j = gidx(static_cast<u16>(gx - 1), gy, gw);
                sx += vx[j];
                sy += vy[j];
                cnt += 1.f;
            }
            if (static_cast<u32>(gx) + 1u < static_cast<u32>(gw)) {
                const u32 j = gidx(static_cast<u16>(gx + 1), gy, gw);
                sx += vx[j];
                sy += vy[j];
                cnt += 1.f;
            }
            if (gy > 0) {
                const u32 j = gidx(gx, static_cast<u16>(gy - 1), gw);
                sx += vx[j];
                sy += vy[j];
                cnt += 1.f;
            }
            if (static_cast<u32>(gy) + 1u < static_cast<u32>(gh)) {
                const u32 j = gidx(gx, static_cast<u16>(gy + 1), gw);
                sx += vx[j];
                sy += vy[j];
                cnt += 1.f;
            }
            const f32 open = block_open(block[i]);
            nx[i] = vx[i] * (1.f - k) + (sx / cnt) * k * open;
            ny[i] = vy[i] * (1.f - k) + (sy / cnt) * k * open;
        }
    }
    std::memcpy(vx, nx, static_cast<size_t>(gn) * sizeof(f32));
    std::memcpy(vy, ny, static_cast<size_t>(gn) * sizeof(f32));
}

static void project_grid (
    u16 gw,
    u16 gh,
    const u8* block,
    u8 jac_n,
    f32* vx,
    f32* vy,
    f32* div,
    f32* pres,
    f32* pres0) 
{
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    if (div == nullptr || pres == nullptr || pres0 == nullptr) {
        return;
    }
    for (u32 i = 0; i < gn; ++i) {
        div[i] = 0.f;
        pres[i] = 0.f;
    }
    for (u16 gy = 0; gy < gh; ++gy) {
        for (u16 gx = 0; gx < gw; ++gx) {
            const u32 i = gidx(gx, gy, gw);
            const f32 open = block_open(block[i]);
            if (open < 0.05f) {
                div[i] = 0.f;
                continue;
            }
            f32 lvx = vx[i];
            f32 rvx = vx[i];
            f32 tvx = vy[i];
            f32 bvx = vy[i];
            if (gx > 0) {
                lvx = vx[gidx(static_cast<u16>(gx - 1), gy, gw)];
            }
            if (static_cast<u32>(gx) + 1u < static_cast<u32>(gw)) {
                rvx = vx[gidx(static_cast<u16>(gx + 1), gy, gw)];
            }
            if (gy > 0) {
                tvx = vy[gidx(gx, static_cast<u16>(gy - 1), gw)];
            }
            if (static_cast<u32>(gy) + 1u < static_cast<u32>(gh)) {
                bvx = vy[gidx(gx, static_cast<u16>(gy + 1), gw)];
            }
            div[i] = 0.5f * ((rvx - lvx) + (bvx - tvx)) * open;
        }
    }
    for (u8 pass = 0; pass < jac_n; ++pass) {
        std::memcpy(pres0, pres, static_cast<size_t>(gn) * sizeof(f32));
        for (u16 gy = 0; gy < gh; ++gy) {
            for (u16 gx = 0; gx < gw; ++gx) {
                const u32 i = gidx(gx, gy, gw);
                f32 lp = pres0[i];
                f32 rp = pres0[i];
                f32 tp = pres0[i];
                f32 bp = pres0[i];
                if (gx > 0) {
                    lp = pres0[gidx(static_cast<u16>(gx - 1), gy, gw)];
                }
                if (static_cast<u32>(gx) + 1u < static_cast<u32>(gw)) {
                    rp = pres0[gidx(static_cast<u16>(gx + 1), gy, gw)];
                }
                if (gy > 0) {
                    tp = pres0[gidx(gx, static_cast<u16>(gy - 1), gw)];
                }
                if (static_cast<u32>(gy) + 1u < static_cast<u32>(gh)) {
                    bp = pres0[gidx(gx, static_cast<u16>(gy + 1), gw)];
                }
                pres[i] = (lp + rp + tp + bp - div[i]) * 0.25f;
            }
        }
    }
    for (u16 gy = 0; gy < gh; ++gy) {
        for (u16 gx = 0; gx < gw; ++gx) {
            const u32 i = gidx(gx, gy, gw);
            const f32 open = block_open(block[i]);
            if (open < 0.05f) {
                vx[i] = 0.f;
                vy[i] = 0.f;
                continue;
            }
            f32 lp = pres[i];
            f32 rp = pres[i];
            f32 tp = pres[i];
            f32 bp = pres[i];
            if (gx > 0) {
                lp = pres[gidx(static_cast<u16>(gx - 1), gy, gw)];
            }
            if (static_cast<u32>(gx) + 1u < static_cast<u32>(gw)) {
                rp = pres[gidx(static_cast<u16>(gx + 1), gy, gw)];
            }
            if (gy > 0) {
                tp = pres[gidx(gx, static_cast<u16>(gy - 1), gw)];
            }
            if (static_cast<u32>(gy) + 1u < static_cast<u32>(gh)) {
                bp = pres[gidx(gx, static_cast<u16>(gy + 1), gw)];
            }
            vx[i] -= 0.5f * (rp - lp) * open;
            vy[i] -= 0.5f * (bp - tp) * open;
        }
    }
}

static bool flu_layout (f32** fb, u32 fb_n, u32 cap, u32 gn, f32* out[7]) {
    if (fb == nullptr || out == nullptr || fb_n == 0u || cap == 0u || gn == 0u) {
        return false;
    }
    u32 used[7] = {};
    u32 bi = 0;
    for (u32 s = 0; s < 7u; ++s) {
        while (bi < fb_n && used[bi] + gn > cap) {
            ++bi;
        }
        if (bi >= fb_n) {
            return false;
        }
        out[s] = fb[bi] + used[bi];
        used[bi] += gn;
    }
    return true;
}

static void fluid_solve (
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    const u8* block,
    u8 iter_n,
    u8 jac_n,
    f32* vx,
    f32* vy,
    f32** fb,
    u32 fb_n,
    u32 flu_cap) 
{
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    f32* flu[7] = {};
    if (!flu_layout(fb, fb_n, flu_cap, gn, flu)) {
        return;
    }
    f32* tvx = flu[0];
    f32* tvy = flu[1];
    f32* dnx = flu[2];
    f32* dny = flu[3];
    f32* div = flu[4];
    f32* pres = flu[5];
    f32* pres0 = flu[6];
    build_lat_grid(w, h, chunk_sz, gw, gh, vx, vy);
    apply_block_vel(gw, gh, block, vx, vy);
    for (u8 it = 0; it < iter_n; ++it) {
        build_lat_grid(w, h, chunk_sz, gw, gh, tvx, tvy);
        for (u32 i = 0; i < gn; ++i) {
            const f32 open = block_open(block[i]);
            const f32 pull = 0.06f * open;
            vx[i] += (tvx[i] - vx[i]) * pull;
            vy[i] += (tvy[i] - vy[i]) * pull;
        }
        diffuse_grid(gw, gh, block, 0.18f, vx, vy, dnx, dny);
        project_grid(gw, gh, block, jac_n, vx, vy, div, pres, pres0);
        apply_block_vel(gw, gh, block, vx, vy);
    }
}

static void upsample_grid (
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    const f32* gvx,
    const f32* gvy,
    f32* vx,
    f32* vy,
    f32* mag) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 cx = static_cast<u16>(px / chunk_sz);
        const u16 cy = static_cast<u16>(py / chunk_sz);
        const u16 cx1 = cx < gw - 1 ? static_cast<u16>(cx + 1) : cx;
        const u16 cy1 = cy < gh - 1 ? static_cast<u16>(cy + 1) : cy;
        const f32 fx = static_cast<f32>(px % chunk_sz) / static_cast<f32>(chunk_sz);
        const f32 fy = static_cast<f32>(py % chunk_sz) / static_cast<f32>(chunk_sz);
        const u32 i00 = gidx(cx, cy, gw);
        const u32 i10 = gidx(cx1, cy, gw);
        const u32 i01 = gidx(cx, cy1, gw);
        const u32 i11 = gidx(cx1, cy1, gw);
        const f32 vxx = gvx[i00] * (1.f - fx) * (1.f - fy) + gvx[i10] * fx * (1.f - fy)
            + gvx[i01] * (1.f - fx) * fy + gvx[i11] * fx * fy;
        const f32 vyy = gvy[i00] * (1.f - fx) * (1.f - fy) + gvy[i10] * fx * (1.f - fy)
            + gvy[i01] * (1.f - fx) * fy + gvy[i11] * fx * fy;
        vx[i] = vxx;
        vy[i] = vyy;
        mag[i] = std::sqrt(vxx * vxx + vyy * vyy);
    }
}

static void norm_mag_field (u32 n, f32* mag) {
    f32 vmax = 0.f;
    for (u32 i = 0; i < n; ++i) {
        if (mag[i] > vmax) {
            vmax = mag[i];
        }
    }
    if (vmax < 1e-6f) {
        return;
    }
    const f32 inv = 1.f / vmax;
    for (u32 i = 0; i < n; ++i) {
        mag[i] *= inv;
    }
}

static void pack_overlays (u32 n, const f32* vx, const f32* vy, const f32* mag, u8* dir, u8* str) {
    for (u32 i = 0; i < n; ++i) {
        f32 len = std::sqrt(vx[i] * vx[i] + vy[i] * vy[i]);
        if (len < 1e-6f) {
            dir[i] = 0;
            str[i] = enc_str(mag[i]);
            continue;
        }
        dir[i] = enc_dir(vx[i] / len, vy[i] / len);
        str[i] = enc_str(mag[i]);
    }
}

struct AdvCoreCtx {
    u16 m_w;
    u16 m_h;
    u16 m_chunk_sz;
    u16 m_gw;
    u16 m_gh;
    u32 m_n;
    const u8* m_terrain;
    bool m_use_ter;
    u8 m_iter_n;
    u8 m_jac_n;
    f32* m_gvx;
    f32* m_gvy;
    u8* m_block;
    P1_Gen_WindPatternAdvRslt* m_rslt;
    bool* m_valid;
};

static bool adv_flu_body (void* vp, f32* fb[7], u32 fb_n) {
    AdvCoreCtx* c = static_cast<AdvCoreCtx*>(vp);
    const i32 wb_n = static_cast<i32>(c->m_n * 2u);
    if (c->m_use_ter) {
        WbSheet sh_blk(wb_n);
        if (!sh_blk.ok()) {
            return false;
        }
        build_tile_block(c->m_terrain, c->m_n, sh_blk.get());
        if (!build_chunk_block(c->m_terrain, c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, c->m_block)) {
            return false;
        }
        fluid_solve(c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, c->m_block, c->m_iter_n, c->m_jac_n,
            c->m_gvx, c->m_gvy, fb, fb_n, c->m_n);
        WbSheet sh_vx(wb_n);
        WbSheet sh_vy(wb_n);
        WbSheet sh_mag(wb_n);
        if (!sh_vx.ok() || !sh_vy.ok() || !sh_mag.ok()) {
            return false;
        }
        f32* vx = reinterpret_cast<f32*>(sh_vx.get());
        f32* vy = reinterpret_cast<f32*>(sh_vy.get());
        f32* mag = reinterpret_cast<f32*>(sh_mag.get());
        upsample_grid(c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, c->m_gvx, c->m_gvy, vx, vy, mag);
        apply_ter_deflect(c->m_w, c->m_h, sh_blk.get(), vx, vy);
        sync_mag(c->m_n, vx, vy, mag);
        downsample_wind(c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, vx, vy, c->m_gvx, c->m_gvy);
        f32* cshl = fb[0];
        apply_upwind_lee_coarse(c->m_gw, c->m_gh, c->m_chunk_sz, c->m_block, c->m_gvx, c->m_gvy, cshl);
        apply_coarse_shelter(c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, cshl, mag);
        norm_mag_field(c->m_n, mag);
        if (!c->m_rslt->m_dir.resize(c->m_w, c->m_h) || !c->m_rslt->m_str.resize(c->m_w, c->m_h)) {
            return false;
        }
        pack_overlays(c->m_n, vx, vy, mag, c->m_rslt->m_dir.data_w(), c->m_rslt->m_str.data_w());
    } else {
        build_open_block(c->m_gw, c->m_gh, c->m_block);
        fluid_solve(c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, c->m_block, c->m_iter_n, c->m_jac_n,
            c->m_gvx, c->m_gvy, fb, fb_n, c->m_n);
        WbSheet sh_vx(wb_n);
        WbSheet sh_vy(wb_n);
        WbSheet sh_mag(wb_n);
        if (!sh_vx.ok() || !sh_vy.ok() || !sh_mag.ok()) {
            return false;
        }
        f32* vx = reinterpret_cast<f32*>(sh_vx.get());
        f32* vy = reinterpret_cast<f32*>(sh_vy.get());
        f32* mag = reinterpret_cast<f32*>(sh_mag.get());
        upsample_grid(c->m_w, c->m_h, c->m_chunk_sz, c->m_gw, c->m_gh, c->m_gvx, c->m_gvy, vx, vy, mag);
        norm_mag_field(c->m_n, mag);
        if (!c->m_rslt->m_dir.resize(c->m_w, c->m_h) || !c->m_rslt->m_str.resize(c->m_w, c->m_h)) {
            return false;
        }
        pack_overlays(c->m_n, vx, vy, mag, c->m_rslt->m_dir.data_w(), c->m_rslt->m_str.data_w());
    }
    c->m_rslt->m_w = c->m_w;
    c->m_rslt->m_h = c->m_h;
    *c->m_valid = true;
    return true;
}

static bool flu_wrap (i32 wb_n, u32 flu_need, u32 idx, f32* fb[7], void* ctx, bool (*body) (void*, f32* fb[7], u32 fb_n)) {
    WbSheet sh(wb_n);
    if (!sh.ok()) {
        return false;
    }
    fb[idx] = reinterpret_cast<f32*>(sh.get());
    if (idx + 1u < flu_need) {
        return flu_wrap(wb_n, flu_need, idx + 1u, fb, ctx, body);
    }
    return body(ctx, fb, flu_need);
}

//================================================================================================================================
//=> - P1_Gen_WindPatternAdv -
//================================================================================================================================

P1_Gen_WindPatternAdv::P1_Gen_WindPatternAdv (const P1_RunPrm& prm, const P1_Gen_WindPatternAdvPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_WindPatternAdv::generate () {
    return gen_core(nullptr, m_prm.m_w, m_prm.m_h, false);
}

bool P1_Gen_WindPatternAdv::generate (const u8* terrain, u16 w, u16 h) { 
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    return gen_core(terrain, w, h, true);
}

bool P1_Gen_WindPatternAdv::gen_core (const u8* terrain, u16 w, u16 h, bool use_ter) {
    m_valid_generation = false;
    m_rslt.m_dir.clear();
    m_rslt.m_str.clear();
    if (!p1_map_size_ok(w, h)) {
        return false;
    }
    u16 chunk_sz = m_sp.m_chunk_sz;
    if (chunk_sz == 0) {
        chunk_sz = 1;
    }
    u16 gw = 0;
    u16 gh = 0;
    grid_dims(w, h, chunk_sz, &gw, &gh);
    if (gw == 0 || gh == 0) {
        return false;
    }
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 wb_n = static_cast<i32>(n * 2u);
    WbSheet sh_gvx(wb_n);
    WbSheet sh_gvy(wb_n);
    WbSheet sh_cblk(wb_n);
    if (!sh_gvx.ok() || !sh_gvy.ok() || !sh_cblk.ok()) {
        return false;
    }
    f32* gvx = reinterpret_cast<f32*>(sh_gvx.get());
    f32* gvy = reinterpret_cast<f32*>(sh_gvy.get());
    u8* block = reinterpret_cast<u8*>(sh_cblk.get());
    const u32 flu_need = (gn * 7u + n - 1u) / n;
    if (flu_need == 0u || flu_need > 7u) {
        return false;
    }
    AdvCoreCtx ctx;
    ctx.m_w = w;
    ctx.m_h = h;
    ctx.m_chunk_sz = chunk_sz;
    ctx.m_gw = gw;
    ctx.m_gh = gh;
    ctx.m_n = n;
    ctx.m_terrain = terrain;
    ctx.m_use_ter = use_ter;
    ctx.m_iter_n = m_sp.m_iter_n;
    ctx.m_jac_n = m_sp.m_jacobi_n;
    ctx.m_gvx = gvx;
    ctx.m_gvy = gvy;
    ctx.m_block = block;
    ctx.m_rslt = &m_rslt;
    ctx.m_valid = &m_valid_generation;
    f32* fb[7] = {};
    if (!flu_wrap(wb_n, flu_need, 0u, fb, &ctx, adv_flu_body)) {
        return false;
    }
    return true;
}

bool P1_Gen_WindPatternAdv::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_WindPatternAdvRslt& P1_Gen_WindPatternAdv::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
