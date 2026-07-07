//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstring>

#include "p1_gen_loess_boost.h"
#include "game_map_defs.h"
#include "wb_sheet.h"
#include "whiteboard.h"

//================================================================================================================================
//=> - Private constants -
//================================================================================================================================

static const f32 k_tau = 6.28318530f; 

static f32 s_wdx[256];
static f32 s_wdy[256];
static bool s_wlut = false;

static void init_wlut () {
    if (s_wlut) {
        return;
    }
    for (u32 i = 0; i < 256u; ++i) {
        const f32 ang = (static_cast<f32>(i) / 255.f) * k_tau;
        s_wdx[i] = std::cos(ang);
        s_wdy[i] = std::sin(ang);
    }
    s_wlut = true;
}

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool is_desert (u8 cl) {
    return cl == CLIMATE_DESERT;
}

static bool is_desert_coarse (u16 cl) {
    return cl == CLIMATE_DESERT;
}

static u32 gidx (u16 gx, u16 gy, u16 gw) {
    return static_cast<u32>(gy) * static_cast<u32>(gw) + static_cast<u32>(gx);
}

static void grid_dims (u16 w, u16 h, u16 chunk_sz, u16* gw_out, u16* gh_out) {
    const u16 gw = static_cast<u16>((static_cast<u32>(w) + static_cast<u32>(chunk_sz) - 1u) / static_cast<u32>(chunk_sz));
    const u16 gh = static_cast<u16>((static_cast<u32>(h) + static_cast<u32>(chunk_sz) - 1u) / static_cast<u32>(chunk_sz));
    if (gw_out != nullptr) {
        *gw_out = gw;
    }
    if (gh_out != nullptr) {
        *gh_out = gh;
    }
}

static void build_coarse (
    u16 w,
    u16 h,
    u16 chunk_sz,
    u16 gw,
    u16 gh,
    const u8* climate,
    const u8* wind_dir,
    const u8* wind_str,
    u16* c_clim,
    u16* c_wdir,
    u16* c_wstr) 
{
    init_wlut();
    for (u16 cy = 0; cy < gh; ++cy) {
        for (u16 cx = 0; cx < gw; ++cx) {
            const u16 x0 = static_cast<u16>(cx * chunk_sz);
            const u16 y0 = static_cast<u16>(cy * chunk_sz);
            const u16 x1 = static_cast<u16>(x0 + chunk_sz > w ? w : x0 + chunk_sz);
            const u16 y1 = static_cast<u16>(y0 + chunk_sz > h ? h : y0 + chunk_sz);
            f32 sx = 0.f;
            f32 sy = 0.f;
            f32 sw = 0.f;
            bool desert = false;
            for (u16 py = y0; py < y1; ++py) {
                for (u16 px = x0; px < x1; ++px) {
                    const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
                    if (is_desert(climate[i])) {
                        desert = true;
                    }
                    const f32 s = static_cast<f32>(wind_str[i]) / 255.f;
                    sx += s_wdx[wind_dir[i]] * s;
                    sy += s_wdy[wind_dir[i]] * s;
                    sw += s;
                }
            }
            const u32 j = gidx(cx, cy, gw);
            c_clim[j] = desert ? CLIMATE_DESERT : climate[static_cast<u32>(y0) * static_cast<u32>(w) + static_cast<u32>(x0)];
            if (sw > 1e-6f) {
                sx /= sw;
                sy /= sw;
            }
            const f32 len = std::sqrt(sx * sx + sy * sy);
            if (len > 1e-6f) {
                f32 ang = std::atan2(sy, sx);
                if (ang < 0.f) {
                    ang += k_tau;
                }
                c_wdir[j] = static_cast<u16>(std::lrint((ang / k_tau) * 255.0));
            } else {
                c_wdir[j] = 0;
            }
            f32 sm = len;
            if (sm < 0.f) {
                sm = 0.f;
            }
            if (sm > 1.f) {
                sm = 1.f;
            }
            c_wstr[j] = static_cast<u16>(std::lrint(static_cast<f64>(sm) * 255.0));
        }
    }
}

static f32 sample_coarse (u16 gw, u16 gh, const f32* gc, u16 gx, u16 gy) {
    if (gc == nullptr || gw == 0 || gh == 0) {
        return 0.f;
    }
    if (gx >= gw) {
        gx = static_cast<u16>(gw - 1);
    }
    if (gy >= gh) {
        gy = static_cast<u16>(gh - 1);
    }
    return gc[gidx(gx, gy, gw)];
}

static void blur_coarse (u16 gw, u16 gh, f32* gc, f32* tmp) {
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    if (tmp == nullptr) {
        return;
    }
    for (u16 gy = 0; gy < gh; ++gy) {
        for (u16 gx = 0; gx < gw; ++gx) {
            f32 sum = 0.f;
            u32 cnt = 0;
            for (i32 oy = -1; oy <= 1; ++oy) {
                for (i32 ox = -1; ox <= 1; ++ox) {
                    i32 nx = static_cast<i32>(gx) + ox;
                    i32 ny = static_cast<i32>(gy) + oy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<i32>(gw) || ny >= static_cast<i32>(gh)) {
                        continue;
                    }
                    sum += sample_coarse(gw, gh, gc, static_cast<u16>(nx), static_cast<u16>(ny));
                    ++cnt;
                }
            }
            tmp[gidx(gx, gy, gw)] = cnt > 0 ? sum / static_cast<f32>(cnt) : 0.f;
        }
    }
    std::memcpy(gc, tmp, static_cast<size_t>(gn) * sizeof(f32));
}

static void upsample_conc (u16 w, u16 h, u16 chunk_sz, u16 gw, u16 gh, const f32* gc, f32* out) {
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
        const f32 c00 = gc[i00];
        const f32 c10 = gc[i10];
        const f32 c01 = gc[i01];
        const f32 c11 = gc[i11];
        out[i] = c00 * (1.f - fx) * (1.f - fy) + c10 * fx * (1.f - fy) + c01 * (1.f - fx) * fy + c11 * fx * fy;
    }
}

static u8 coarse_hop (u8 hop_max, u16 chunk_sz) {
    u32 hop = (static_cast<u32>(hop_max) + static_cast<u32>(chunk_sz) - 1u) / static_cast<u32>(chunk_sz);
    if (hop < 1u) {
        hop = 1u;
    }
    if (hop > 255u) {
        hop = 255u;
    }
    return static_cast<u8>(hop);
}

static u32 wind_dest (u16 w, u16 h, u16 px, u16 py, u8 dir, u8 str, u8 hop_max) {
    init_wlut();
    const f32 ws = static_cast<f32>(str) / 255.f;
    u8 hop = hop_max;
    if (hop < 1) {
        hop = 1;
    }
    const f32 dist = 1.f + (static_cast<f32>(hop) - 1.f) * ws;
    i32 tx = static_cast<i32>(px) + static_cast<i32>(std::lrint(s_wdx[dir] * dist));
    i32 ty = static_cast<i32>(py) + static_cast<i32>(std::lrint(s_wdy[dir] * dist));
    if (tx < 0) {
        tx = 0;
    }
    if (ty < 0) {
        ty = 0;
    }
    if (tx >= static_cast<i32>(w)) {
        tx = static_cast<i32>(w) - 1;
    }
    if (ty >= static_cast<i32>(h)) {
        ty = static_cast<i32>(h) - 1;
    }
    return static_cast<u32>(ty) * static_cast<u32>(w) + static_cast<u32>(tx);
}

static f32 move_frac (u8 str, f32 move_base) {
    const f32 s = static_cast<f32>(str) / 255.f;
    f32 mf = move_base * (0.35f + 0.65f * s);
    if (mf < 0.08f) {
        mf = 0.08f;
    }
    if (mf > 0.92f) {
        mf = 0.92f;
    }
    return mf;
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

static void blur_fine (u16 w, u16 h, f32* fine, f32* tmp) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (fine == nullptr || tmp == nullptr || w == 0 || h == 0) {
        return;
    }
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            f32 sum = 0.f;
            u32 cnt = 0;
            for (i32 oy = -1; oy <= 1; ++oy) {
                for (i32 ox = -1; ox <= 1; ++ox) {
                    const i32 nx = static_cast<i32>(px) + ox;
                    const i32 ny = static_cast<i32>(py) + oy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                        continue;
                    }
                    const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    sum += fine[j];
                    ++cnt;
                }
            }
            tmp[static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)] = cnt > 0 ? sum / static_cast<f32>(cnt) : 0.f;
        }
    }
    std::memcpy(fine, tmp, static_cast<size_t>(n) * sizeof(f32));
}

static void norm_pack (u32 n, const f32* conc, u8* out, f32 gamma, f32 peak) {
    f32 vmax = 0.f;
    for (u32 i = 0; i < n; ++i) {
        if (conc[i] > vmax) {
            vmax = conc[i];
        }
    }
    if (vmax < 1e-6f) {
        vmax = 1.f;
    }
    f32 denom = vmax * peak;
    if (denom < 1e-6f) {
        denom = vmax;
    }
    u8 lut[256];
    build_gamma_lut(lut, gamma);
    for (u32 i = 0; i < n; ++i) {
        f32 t = conc[i] / denom;
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

//================================================================================================================================
//=> - P1_Gen_LoessBoost -
//================================================================================================================================

P1_Gen_LoessBoost::P1_Gen_LoessBoost (const P1_RunPrm& prm, const P1_Gen_LoessBoostPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() 
{
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_LoessBoost::generate (const u8* climate, const u8* wind_dir, const u8* wind_str, u16 w, u16 h) {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    if (climate == nullptr || wind_dir == nullptr || wind_str == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    u16 chunk_sz = m_sp.m_chunk_sz;
    if (chunk_sz == 0) {
        chunk_sz = 1;
    }
    u16 gw = 0;
    u16 gh = 0;
    grid_dims(w, h, chunk_sz, &gw, &gh);
    const u32 gn = static_cast<u32>(gw) * static_cast<u32>(gh);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 wb_n = static_cast<i32>(n * 2u);
    WbSheet sh_conc(wb_n);
    WbSheet sh_next(wb_n);
    if (!sh_conc.ok() || !sh_next.ok()) {
        return false;
    }
    f32* conc = reinterpret_cast<f32*>(sh_conc.get());
    f32* next = reinterpret_cast<f32*>(sh_next.get());
    {
        WbSheet sh_clim(wb_n);
        WbSheet sh_wdir(wb_n);
        WbSheet sh_wstr(wb_n);
        if (!sh_clim.ok() || !sh_wdir.ok() || !sh_wstr.ok()) {
            return false;
        }
        build_coarse(w, h, chunk_sz, gw, gh, climate, wind_dir, wind_str, sh_clim.get(), sh_wdir.get(), sh_wstr.get());
        for (u32 i = 0; i < gn; ++i) {
            conc[i] = 0.f;
        }
        const u8 hop_c = coarse_hop(m_sp.m_hop_max, chunk_sz);
        u16 step_n = m_sp.m_step_n;
        if (step_n == 0) {
            step_n = 1;
        }
        u16* c_clim = sh_clim.get();
        u16* c_wdir = sh_wdir.get();
        u16* c_wstr = sh_wstr.get();
        f32* rd = conc;
        f32* wr = next;
        for (u16 step = 0; step < step_n; ++step) {
            std::memset(wr, 0, static_cast<size_t>(gn) * sizeof(f32));
            for (u32 i = 0; i < gn; ++i) {
                const u16 py = static_cast<u16>(i / static_cast<u32>(gw));
                const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(gw));
                const f32 mf = move_frac(static_cast<u8>(c_wstr[i]), m_sp.m_move);
                f32 m = rd[i];
                const f32 mov = m * mf;
                const f32 stay = m - mov;
                f32 add = 0.f;
                if (is_desert_coarse(c_clim[i])) {
                    add = m_sp.m_emit;
                }
                wr[i] += stay + add;
                const u32 j = wind_dest(gw, gh, px, py, static_cast<u8>(c_wdir[i]), static_cast<u8>(c_wstr[i]), hop_c);
                wr[j] += mov;
            }
            f32* t = rd;
            rd = wr;
            wr = t;
        }
        if (rd != conc) {
            std::memcpy(conc, rd, static_cast<size_t>(gn) * sizeof(f32));
        }
    }
    blur_coarse(gw, gh, conc, next);
    if (!m_rslt.m_ov.resize(w, h)) {
        return false;
    }
    f32* fine = reinterpret_cast<f32*>(sh_next.get());
    f32* sm_tmp = reinterpret_cast<f32*>(sh_conc.get());
    upsample_conc(w, h, chunk_sz, gw, gh, conc, fine);
    for (u8 pass = 0; pass < m_sp.m_smooth_n; ++pass) {
        blur_fine(w, h, fine, sm_tmp);
    }
    norm_pack(n, fine, m_rslt.m_ov.data_w(), m_sp.m_gamma, m_sp.m_peak);
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_LoessBoost::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_LoessBoostRslt& P1_Gen_LoessBoost::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
