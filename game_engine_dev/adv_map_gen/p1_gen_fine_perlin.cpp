//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_fine_perlin.h"

#include "p1_gen_noise_perlin.h"
#include "p1_wb_util.h"
#include "profile_time.h"

#include <cstring>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool compress_avg (
    const f32* src,
    u16 w,
    u16 h,
    u16 tw,
    u16 th,
    f32* a) 
{
    if (src == nullptr || a == nullptr || tw == 0u || th == 0u) {
        return false;
    }
    if (static_cast<u32>(tw) * 4u != static_cast<u32>(w) || static_cast<u32>(th) * 4u != static_cast<u32>(h)) {
        return false;
    }
    for (u16 ty = 0; ty < th; ++ty) {
        for (u16 tx = 0; tx < tw; ++tx) {
            f32 sum = 0.f;
            const u16 x0 = static_cast<u16>(tx * 4u);
            const u16 y0 = static_cast<u16>(ty * 4u);
            for (u16 dy = 0; dy < 4u; ++dy) {
                for (u16 dx = 0; dx < 4u; ++dx) {
                    sum += src[tidx(w, static_cast<u16>(x0 + dx), static_cast<u16>(y0 + dy))];
                }
            }
            a[tidx(tw, tx, ty)] = sum * (1.f / 16.f);
        }
    }
    return true;
}

static void blit_tile (
    const f32* a,
    u16 tw,
    u16 th,
    bool fh,
    bool fv,
    f32* dst,
    u16 w,
    u16 ox,
    u16 oy) 
{
    for (u16 ly = 0; ly < th; ++ly) {
        const u16 sy = fv ? static_cast<u16>(th - 1u - ly) : ly;
        for (u16 lx = 0; lx < tw; ++lx) {
            const u16 sx = fh ? static_cast<u16>(tw - 1u - lx) : lx;
            dst[tidx(w, static_cast<u16>(ox + lx), static_cast<u16>(oy + ly))] = a[tidx(tw, sx, sy)];
        }
    }
}

static bool build_fine (
    const f32* src,
    u16 w,
    u16 h,
    f32* fine,
    P1_Gen_FinePerlinRslt* out) 
{
    if (out == nullptr || src == nullptr || fine == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    if ((w % 4u) != 0u || (h % 4u) != 0u) {
        return false;
    }
    const u16 tw = static_cast<u16>(w / 4u);
    const u16 th = static_cast<u16>(h / 4u);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_4B wb_a("P1_Gen_FinePerlin", "tile_a", 0u);
    P1_WB_CHK(wb_a);
    f32* a = reinterpret_cast<f32*>(wb_a.get_iter_ptr());
    if (!compress_avg(src, w, h, tw, th, a)) {
        return false;
    }
    for (u16 tr = 0; tr < 4u; ++tr) {
        const bool fv = (tr & 1u) != 0u;
        const u16 oy = static_cast<u16>(tr * th);
        for (u16 tc = 0; tc < 4u; ++tc) {
            const bool fh = (tc & 1u) != 0u;
            const u16 ox = static_cast<u16>(tc * tw);
            blit_tile(a, tw, th, fh, fv, fine, w, ox, oy);
        }
    }
    if (!out->m_ov.resize(w, h)) {
        return false;
    }
    p1_perlin_field_to_gray(fine, n, out->m_ov.data_w());
    out->m_w = w;
    out->m_h = h;
    out->m_tw = tw;
    out->m_th = th;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_FinePerlin -
//================================================================================================================================

P1_Gen_FinePerlin::P1_Gen_FinePerlin (const P1_RunPrm& prm) :
    m_prm(prm),
    m_ok(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_tw = 0;
    m_rslt.m_th = 0;
}

bool P1_Gen_FinePerlin::generate (const f32* src, u16 w, u16 h, f32* out_fine) {
    PTIME_START(__func__);
    m_ok = false;
    m_rslt.m_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_tw = 0;
    m_rslt.m_th = 0;
    if (src == nullptr || out_fine == nullptr || w != m_prm.m_w || h != m_prm.m_h || !p1_run_prm_ok(m_prm)) {
        PTIME_STOP(__func__);
        return false;
    }
    if (!build_fine(src, w, h, out_fine, &m_rslt)) {
        m_rslt.m_ov.clear();
        PTIME_STOP(__func__);
        return false;
    }
    m_ok = true;
    PTIME_STOP(__func__);
    return true;
}

bool P1_Gen_FinePerlin::is_valid () const {
    return m_ok;
}

const P1_Gen_FinePerlinRslt& P1_Gen_FinePerlin::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
