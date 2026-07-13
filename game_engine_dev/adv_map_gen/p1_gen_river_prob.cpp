//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstring>

#include "p1_gen_river_prob.h"
#include "generator_constants.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

#define RP_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverProb", msg)

//================================================================================================================================
//=> - P1 river prob band limits -
//================================================================================================================================

#define P1_RIVER_PROB_WAT_WT 5u
#define P1_RIVER_PROB_EQ_WT 1u

#define P1_RIVER_PROB_BAND_N 10u

#define P1_RIVER_PROB_LIM_0 25
#define P1_RIVER_PROB_LIM_1 50
#define P1_RIVER_PROB_LIM_2 75
#define P1_RIVER_PROB_LIM_3 100
#define P1_RIVER_PROB_LIM_4 125
#define P1_RIVER_PROB_LIM_5 150
#define P1_RIVER_PROB_LIM_6 175
#define P1_RIVER_PROB_LIM_7 200
#define P1_RIVER_PROB_LIM_8 250

static const u16 k_unv = 0xFFFFu;

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static bool is_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_land (u8 cls) {
    return !is_wat(cls) && cls != TERR_MOUNTAINS[0];
}

static f32 dist_eq (f32 px, f32 py, u16 w, u16 h) {
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

static f32 max_eq_d (u16 w, u16 h) {
    return std::sqrt(static_cast<f32>(w) * static_cast<f32>(w) + static_cast<f32>(h) * static_cast<f32>(h)) * 0.15f;
}

static bool build_wat_dist (const u8* terrain, u16 w, u16 h, u16* out) {
    if (terrain == nullptr || out == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u32 n = wi * hi;
    for (u32 i = 0; i < n; ++i) {
        if (!is_land(terrain[i])) {
            out[i] = 0u;
        } else {
            out[i] = k_unv;
        }
    }
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 ti = static_cast<u32>(py) * wi + static_cast<u32>(px);
            if (!is_land(terrain[ti])) {
                continue;
            }
            bool adj = false;
            if (px > 0 && is_wat(terrain[ti - 1u])) {
                adj = true;
            }
            if (!adj && static_cast<u32>(px) + 1u < wi && is_wat(terrain[ti + 1u])) {
                adj = true;
            }
            if (!adj && py > 0 && is_wat(terrain[ti - wi])) {
                adj = true;
            }
            if (!adj && static_cast<u32>(py) + 1u < hi && is_wat(terrain[ti + wi])) {
                adj = true;
            }
            if (adj) {
                out[ti] = 0u;
                que.push(px, py);
            }
        }
    }
    u32 qi = 0u;
    while (qi < que.count()) {
        const u16 px = que.x_at(qi);
        const u16 py = que.y_at(qi);
        qi++;
        const u32 ti = static_cast<u32>(py) * wi + static_cast<u32>(px);
        const u16 cur = out[ti];
        if (cur >= 65534u) {
            continue;
        }
        const u16 nd = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 ni = ti - 1u;
            if (is_land(terrain[ni]) && out[ni] == k_unv) {
                out[ni] = nd;
                que.push(static_cast<u16>(px - 1u), py);
            }
        }
        if (static_cast<u32>(px) + 1u < wi) {
            const u32 ni = ti + 1u;
            if (is_land(terrain[ni]) && out[ni] == k_unv) {
                out[ni] = nd;
                que.push(static_cast<u16>(px + 1u), py);
            }
        }
        if (py > 0) {
            const u32 ni = ti - wi;
            if (is_land(terrain[ni]) && out[ni] == k_unv) {
                out[ni] = nd;
                que.push(px, static_cast<u16>(py - 1u));
            }
        }
        if (static_cast<u32>(py) + 1u < hi) {
            const u32 ni = ti + wi;
            if (is_land(terrain[ni]) && out[ni] == k_unv) {
                out[ni] = nd;
                que.push(px, static_cast<u16>(py + 1u));
            }
        }
    }
    for (u32 i = 0; i < n; ++i) {
        if (is_land(terrain[i]) && out[i] == k_unv) {
            out[i] = 0u;
        }
    }
    return true;
}

static bool build_eq_near (u16 w, u16 h, u16* out) {
    if (out == nullptr || w == 0 || h == 0) {
        return false;
    }
    const f32 md = max_eq_d(w, h);
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 py = 0; py < hi; ++py) {
        for (u32 px = 0; px < wi; ++px) {
            const f32 d = dist_eq(static_cast<f32>(px) + 0.5f, static_cast<f32>(py) + 0.5f, w, h);
            f32 near = md - d;
            if (near < 0.f) {
                near = 0.f;
            }
            const u32 ti = py * wi + px;
            out[ti] = static_cast<u16>(near + 0.5f);
        }
    }
    return true;
}

static u8 lin_to_band (u8 lin) {
    if (lin < P1_RIVER_PROB_LIM_0) {
        return 0u;
    }
    if (lin < P1_RIVER_PROB_LIM_1) {
        return 1u;
    }
    if (lin < P1_RIVER_PROB_LIM_2) {
        return 2u;
    }
    if (lin < P1_RIVER_PROB_LIM_3) {
        return 3u;
    }
    if (lin < P1_RIVER_PROB_LIM_4) {
        return 4u;
    }
    if (lin < P1_RIVER_PROB_LIM_5) {
        return 5u;
    }
    if (lin < P1_RIVER_PROB_LIM_6) {
        return 6u;
    }
    if (lin < P1_RIVER_PROB_LIM_7) {
        return 7u;
    }
    if (lin < P1_RIVER_PROB_LIM_8) {
        return 8u;
    }
    return static_cast<u8>(P1_RIVER_PROB_BAND_N - 1u);
}

static void build_bands (u32 n, u8* prob) {
    for (u32 i = 0; i < n; ++i) {
        prob[i] = lin_to_band(prob[i]);
    }
}

static bool build_wgt_prob (u16 w, u16 h, const u16* wat, const u16* eq, u16* wgt, u8* prob) {
    if (wat == nullptr || eq == nullptr || wgt == nullptr || prob == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 vmin = 0xFFFFFFFFu;
    u32 vmax = 0u;
    for (u32 i = 0; i < n; ++i) {
        const u32 v = static_cast<u32>(wat[i]) * P1_RIVER_PROB_WAT_WT + static_cast<u32>(eq[i]) * P1_RIVER_PROB_EQ_WT;
        wgt[i] = static_cast<u16>(v > 65535u ? 65535u : v);
        if (v < vmin) {
            vmin = v;
        }
        if (v > vmax) {
            vmax = v;
        }
    }
    if (vmax <= vmin) {
        std::memset(prob, 0, n);
        return true;
    }
    const u32 span = vmax - vmin;
    for (u32 i = 0; i < n; ++i) {
        const u32 v = static_cast<u32>(wgt[i]);
        prob[i] = static_cast<u8>(((v - vmin) * 255u) / span);
    }
    build_bands(n, prob);
    return true;
}

static bool build_river_prob (const u8* terrain, u16 w, u16 h, u16* wat, u16* eq, u16* wgt, u8* prob) {
    if (!build_wat_dist(terrain, w, h, wat)) {
        return false;
    }
    if (!build_eq_near(w, h, eq)) {
        return false;
    }
    if (!build_wgt_prob(w, h, wat, eq, wgt, prob)) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverProb -
//================================================================================================================================

P1_Gen_RiverProb::P1_Gen_RiverProb (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt(),
    m_wb_wat(nullptr),
    m_wb_eq(nullptr),
    m_wb_wgt(nullptr),
    m_wb_prob(nullptr) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_wat_dist = nullptr;
    m_rslt.m_eq_near = nullptr;
    m_rslt.m_wgt_sum = nullptr;
    m_rslt.m_prob = nullptr;
    m_wb_wat = new Whiteboard_2B("P1_Gen_RiverProb", "wat", prm.m_seed);
    m_wb_eq = new Whiteboard_2B("P1_Gen_RiverProb", "eq", prm.m_seed);
    m_wb_wgt = new Whiteboard_2B("P1_Gen_RiverProb", "wgt", prm.m_seed);
    m_wb_prob = new Whiteboard_1B("P1_Gen_RiverProb", "prob", prm.m_seed);
    P1_WB_CHK(*m_wb_wat);
    P1_WB_CHK(*m_wb_eq);
    P1_WB_CHK(*m_wb_wgt);
    P1_WB_CHK(*m_wb_prob);
}

P1_Gen_RiverProb::~P1_Gen_RiverProb () {
    clear_rslt();
    delete m_wb_prob;
    delete m_wb_wgt;
    delete m_wb_eq;
    delete m_wb_wat;
    m_wb_prob = nullptr;
    m_wb_wgt = nullptr;
    m_wb_eq = nullptr;
    m_wb_wat = nullptr;
}

void P1_Gen_RiverProb::clear_rslt () {
    m_rslt.m_wat_dist = nullptr;
    m_rslt.m_eq_near = nullptr;
    m_rslt.m_wgt_sum = nullptr;
    m_rslt.m_prob = nullptr;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverProb::generate (const u8* terrain, u16 w, u16 h) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        RP_ABORT("generate invalid args");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        RP_ABORT("generate size mismatch");
    }
    u16* wat = m_wb_wat->get_iter_ptr();
    u16* eq = m_wb_eq->get_iter_ptr();
    u16* wgt = m_wb_wgt->get_iter_ptr();
    u8* prob = m_wb_prob->raw();
    if (wat == nullptr || eq == nullptr || wgt == nullptr || prob == nullptr) {
        RP_ABORT("generate whiteboard null");
    }
    if (!build_river_prob(terrain, w, h, wat, eq, wgt, prob)) {
        return false;
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_wat_dist = wat;
    m_rslt.m_eq_near = eq;
    m_rslt.m_wgt_sum = wgt;
    m_rslt.m_prob = prob;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverProb::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverProbRslt& P1_Gen_RiverProb::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
