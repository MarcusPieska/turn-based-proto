//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_dynamic_pts.h"

#include "generator_constants.h"
#include "p1_step_log.h"

#define RDP_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverDynamicPts", msg)

//================================================================================================================================
//=> - P1 dynamic pt steps -
//================================================================================================================================

#define P1_RIVER_DYN_STEP_N 4u
#define P1_RIVER_DYN_STEP_0 4u
#define P1_RIVER_DYN_STEP_1 6u
#define P1_RIVER_DYN_STEP_2 8u
#define P1_RIVER_DYN_STEP_3 10u
#define P1_RIVER_DYN_FINE_STEP P1_RIVER_DYN_STEP_0

static const u16 k_dyn_step[P1_RIVER_DYN_STEP_N] = {
    P1_RIVER_DYN_STEP_0,
    P1_RIVER_DYN_STEP_1,
    P1_RIVER_DYN_STEP_2,
    P1_RIVER_DYN_STEP_3};

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static bool is_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool is_land (u8 cls) {
    return !is_wat(cls) && !is_mtn(cls);
}

static u16 band_to_step (u8 band) {
    if (band == 0u) {
        return P1_RIVER_DYN_STEP_0;
    }
    if (band == 1u) {
        return P1_RIVER_DYN_STEP_1;
    }
    if (band == 2u) {
        return P1_RIVER_DYN_STEP_2;
    }
    return P1_RIVER_DYN_STEP_3;
}

static const P1_StepPtLay* lay_for_step (const P1_StepPtLay* lays, u16 step) {
    for (u32 i = 0; i < P1_RIVER_DYN_STEP_N; ++i) {
        if (k_dyn_step[i] == step) {
            return &lays[i];
        }
    }
    return nullptr;
}

static bool merge_dyn_pts (
    const u8* terrain,
    u16 w,
    u16 h,
    const u8* band,
    const P1_StepPtLay* lays,
    u16 ocn_sec_n,
    const WB_QueXY& ocn_que,
    P1_Gen_RiverPtsRslt* out) {
    if (terrain == nullptr || band == nullptr || lays == nullptr || out == nullptr || w == 0 || h == 0) {
        return false;
    }
    out->m_que.clear();
    out->m_ocn_sec_n = ocn_sec_n;
    for (u32 pi = 0; pi < static_cast<u32>(ocn_sec_n); ++pi) {
        if (!out->m_que.push(ocn_que.x_at(pi), ocn_que.y_at(pi))) {
            return false;
        }
    }
    const u32 wi = static_cast<u32>(w);
    const u16 fine = P1_RIVER_DYN_FINE_STEP;
    for (u16 ly = 0; ly < h; ly = static_cast<u16>(ly + fine)) {
        for (u16 lx = 0; lx < w; lx = static_cast<u16>(lx + fine)) {
            u16 cx = static_cast<u16>(lx + fine / 2u);
            u16 cy = static_cast<u16>(ly + fine / 2u);
            if (cx >= w) {
                cx = static_cast<u16>(w - 1u);
            }
            if (cy >= h) {
                cy = static_cast<u16>(h - 1u);
            }
            const u32 cti = static_cast<u32>(cy) * wi + static_cast<u32>(cx);
            if (!is_land(terrain[cti])) {
                continue;
            }
            const u16 pick = band_to_step(band[cti]);
            const u16 alx = static_cast<u16>((static_cast<u32>(lx) / static_cast<u32>(pick)) * static_cast<u32>(pick));
            const u16 aly = static_cast<u16>((static_cast<u32>(ly) / static_cast<u32>(pick)) * static_cast<u32>(pick));
            const P1_StepPtLay* lay = lay_for_step(lays, pick);
            u16 px = 0;
            u16 py = 0;
            if (lay == nullptr || !p1_step_pt_lay_at(lay, alx, aly, &px, &py)) {
                continue;
            }
            if (!out->m_que.push(px, py)) {
                return false;
            }
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_n = out->m_que.count();
    return out->m_que.ok() && out->m_n > static_cast<u32>(out->m_ocn_sec_n);
}

static bool build_dyn_pts (
    const P1_RunPrm& prm,
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverProbRslt& prob,
    const P1_OceanIndexRef& ocean,
    P1_Gen_RiverPtsRslt* out) {
    if (terrain == nullptr || out == nullptr || prob.m_prob == nullptr || prob.m_w != w || prob.m_h != h) {
        RDP_ABORT("build_dyn_pts invalid args");
    }
    P1_StepPtLay lays[P1_RIVER_DYN_STEP_N];
    for (u32 i = 0; i < P1_RIVER_DYN_STEP_N; ++i) {
        lays[i].m_mrk = nullptr;
        lays[i].m_ax = nullptr;
        lays[i].m_ay = nullptr;
    }
    for (u32 i = 0; i < P1_RIVER_DYN_STEP_N; ++i) {
        if (!p1_step_pt_lay_init(&lays[i], w, h, k_dyn_step[i])) {
            for (u32 j = 0; j < i; ++j) {
                p1_step_pt_lay_free(&lays[j]);
            }
            return false;
        }
    }
    for (u32 i = 0; i < P1_RIVER_DYN_STEP_N; ++i) {
        if (!p1_stamp_land_step_lay(terrain, w, h, prm.m_seed, &lays[i])) {
            for (u32 j = 0; j < P1_RIVER_DYN_STEP_N; ++j) {
                p1_step_pt_lay_free(&lays[j]);
            }
            return false;
        }
    }
    WB_QueXY ocn_que;
    u16 ocn_sec_n = 0u;
    if (p1_ocean_ref_ok(ocean) && ocean.m_w == w && ocean.m_h == h && ocean.m_ocean_n > 0u) {
        if (!p1_push_ocean_pts(ocean.m_ov, w, h, ocean.m_ocean_n, ocean.m_largest_idx, &ocn_que, &ocn_sec_n)) {
            for (u32 j = 0; j < P1_RIVER_DYN_STEP_N; ++j) {
                p1_step_pt_lay_free(&lays[j]);
            }
            return false;
        }
    }
    const bool ok = merge_dyn_pts(terrain, w, h, prob.m_prob, lays, ocn_sec_n, ocn_que, out);
    for (u32 j = 0; j < P1_RIVER_DYN_STEP_N; ++j) {
        p1_step_pt_lay_free(&lays[j]);
    }
    return ok;
}

//================================================================================================================================
//=> - P1_Gen_RiverDynamicPts -
//================================================================================================================================

P1_Gen_RiverDynamicPts::P1_Gen_RiverDynamicPts (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_ocn_sec_n = 0;
    m_rslt.m_n = 0u;
}

P1_Gen_RiverDynamicPts::~P1_Gen_RiverDynamicPts () {
    clear_rslt();
}

void P1_Gen_RiverDynamicPts::clear_rslt () {
    m_rslt.m_que.clear();
    m_rslt.m_n = 0u;
    m_rslt.m_ocn_sec_n = 0;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverDynamicPts::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverProbRslt& prob,
    const P1_OceanIndexRef& ocean) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || prob.m_prob == nullptr) {
        RDP_ABORT("generate invalid args");
    }
    if (prob.m_w != w || prob.m_h != h) {
        RDP_ABORT("generate prob size mismatch");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        RDP_ABORT("generate size mismatch");
    }
    if (!build_dyn_pts(m_prm, terrain, w, h, prob, ocean, &m_rslt)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverDynamicPts::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverPtsRslt& P1_Gen_RiverDynamicPts::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
