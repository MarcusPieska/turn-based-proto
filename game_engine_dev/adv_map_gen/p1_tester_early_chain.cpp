//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_tester_early_chain.h"

#include "p1_adj_outline_fill.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_shaped_outline.h"

#include <ctime>
#include <cstring>

//================================================================================================================================
//=> - P1_EarlyChainRslt -
//================================================================================================================================

void p1_free_early_chain (P1_EarlyChainRslt* r) {
    if (r == nullptr) {
        return;
    }
    r->m_ov.clear();
    delete[] r->m_perlin_f32;
    r->m_perlin_f32 = nullptr;
    r->m_perlin_n = 0u;
    r->m_perlin_gray.clear();
    delete[] r->m_fill_ter;
    r->m_fill_ter = nullptr;
    r->m_ld_wl.clear();
    r->m_ld_dist.clear();
    delete[] r->m_ter;
    r->m_ter = nullptr;
    delete r->m_pts_que;
    r->m_pts_que = nullptr;
    r->m_pts_n = 0u;
    r->m_w = 0; 
    r->m_h = 0;
}

static bool early_fill_ter (const P1_RunPrm& prm, const u8* ov, u16 w, u16 h, u8** out_ter) {
    if (out_ter == nullptr || ov == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* ter = new u8[n];
    if (ter == nullptr) {
        return false;
    }
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(ter, w, h, ov) || !fill.is_valid()) {
        delete[] ter;
        return false;
    }
    *out_ter = ter;
    return true;
}

bool p1_build_early_chain (const P1_RunPrm& prm, const P1_MapConfig& cfg, u16 last_step, P1_EarlyChainRslt* out, double* sec) {
    if (out == nullptr || !p1_run_prm_ok(prm) || last_step < 1u) {
        return false;
    }
    p1_free_early_chain(out);
    const clock_t t0 = clock();
    if (!p1_gen_step01_ov(prm, &out->m_ov)) {
        return false;
    }
    const u16 w = out->m_ov.width();
    const u16 h = out->m_ov.height();
    const u8* ov = out->m_ov.data();
    if (ov == nullptr || !p1_map_size_ok(w, h)) {
        p1_free_early_chain(out);
        return false;
    }
    out->m_w = w;
    out->m_h = h;
    if (last_step == 1u) {
        if (sec != nullptr) {
            const clock_t t1 = clock();
            *sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
        }
        return true;
    }
    if (last_step >= 3u) {
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        f32* combo = new f32[n];
        if (combo == nullptr) {
            p1_free_early_chain(out);
            return false;
        }
        const P1_Gen_NoisePerlinPrm nprm = p1_gen_noise_perlin_prm_from_cfg(cfg, w, h);
        if (!out->m_perlin_gray.resize(w, h)) {
            delete[] combo;
            p1_free_early_chain(out);
            return false;
        }
        if (!p1_build_perlin_field_f32(prm.m_seed, nprm, combo, out->m_perlin_gray.data_w())) {
            delete[] combo;
            p1_free_early_chain(out);
            return false;
        }
        out->m_perlin_f32 = combo;
        out->m_perlin_n = n;
    }
    if (last_step >= 4u) {
        P1_Gen_LandDepth depth_gen(prm);
        if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
            p1_free_early_chain(out);
            return false;
        }
        const P1_Gen_LandDepthRslt& dr = depth_gen.result();
        if (!out->m_ld_wl.assign_copy(w, h, dr.m_wl.data())
            || !out->m_ld_dist.assign_copy(w, h, dr.m_dist.data())) {
            p1_free_early_chain(out);
            return false;
        }
    }
    if (last_step >= 2u) {
        u8* ter = nullptr;
        if (!early_fill_ter(prm, ov, w, h, &ter)) {
            p1_free_early_chain(out);
            return false;
        }
        out->m_fill_ter = ter;
    }
    if (last_step >= 7u) {
        if (out->m_fill_ter == nullptr || out->m_ld_dist.data() == nullptr || out->m_perlin_f32 == nullptr) {
            p1_free_early_chain(out);
            return false;
        }
        const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
        u8* ter = new u8[n];
        if (ter == nullptr) {
            p1_free_early_chain(out);
            return false;
        }
        std::memcpy(ter, out->m_fill_ter, static_cast<size_t>(n));
        const P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_from_cfg(cfg);
        P1_Gen_ShapedOutline shaped(prm);
        shaped.bind_perlin_field(out->m_perlin_f32, w, h);
        if (!shaped.apply(sp, ter, w, h, ov, out->m_ld_dist.data())) {
            delete[] ter;
            p1_free_early_chain(out);
            return false;
        }
        out->m_ter = ter;
    }
    if (last_step >= 8u) {
        if (out->m_ter == nullptr) {
            p1_free_early_chain(out);
            return false;
        }
        P1_Gen_RiverPts pts_gen(prm);
        if (!pts_gen.generate(out->m_ter, w, h) || !pts_gen.is_valid()) {
            p1_free_early_chain(out);
            return false;
        }
        const P1_Gen_RiverPtsRslt& pr = pts_gen.result();
        if (pr.m_n == 0u || !pr.m_que.ok()) {
            p1_free_early_chain(out);
            return false;
        }
        out->m_pts_que = new WB_QueXY();
        if (out->m_pts_que == nullptr || !out->m_pts_que->ok()) {
            delete out->m_pts_que;
            out->m_pts_que = nullptr;
            p1_free_early_chain(out);
            return false;
        }
        for (u32 pi = 0; pi < pr.m_n; ++pi) {
            if (!out->m_pts_que->push(pr.m_que.x_at(pi), pr.m_que.y_at(pi))) {
                p1_free_early_chain(out);
                return false;
            }
        }
        out->m_pts_n = pr.m_n;
    }
    if (sec != nullptr) {
        const clock_t t1 = clock();
        *sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
