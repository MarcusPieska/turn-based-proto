//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_ocean_index.h"

#include <cstring>

#include "game_map_defs.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "whiteboard_mng.h"

#define OI_ABORT(msg) P1_STEP_ABORT("P1_Gen_OceanIndex", msg)

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static bool is_ocn_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_inland_wat (u8 cls) {
    return cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static bool flood_ocean_comp (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 idx,
    u32 seed_i,
    u16* ov,
    u32* q) {
    if (terrain == nullptr || ov == nullptr || q == nullptr || seed_i >= static_cast<u32>(w) * static_cast<u32>(h)) {
        return false;
    }
    if (ov[seed_i] != static_cast<u16>(P1_OCEAN_IDX_NONE) || !is_ocn_wat(terrain[seed_i])) {
        return false;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u32 qn = 0;
    ov[seed_i] = idx;
    q[qn++] = seed_i;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u32>(nx), static_cast<u32>(ny));
            if (ov[ni] != static_cast<u16>(P1_OCEAN_IDX_NONE) || !is_ocn_wat(terrain[ni])) {
                continue;
            }
            ov[ni] = idx;
            q[qn++] = ni;
        }
    }
    return true;
}

static u16 find_largest_ocean_idx (const u16* ov, u32 n, u16 ocean_n) {
    if (ov == nullptr || ocean_n == 0u) {
        return static_cast<u16>(P1_OCEAN_IDX_NONE);
    }
    u32 cnt[P1_OCEAN_IDX_MAX + 1u];
    for (u32 i = 0; i <= static_cast<u32>(P1_OCEAN_IDX_MAX); ++i) {
        cnt[i] = 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 idx = ov[i];
        if (idx == static_cast<u16>(P1_OCEAN_IDX_NONE) || idx > ocean_n) {
            continue;
        }
        cnt[static_cast<u32>(idx)]++;
    }
    u16 best_idx = static_cast<u16>(P1_OCEAN_IDX_NONE);
    u32 best_n = 0u;
    for (u16 idx = 1u; idx <= ocean_n; ++idx) {
        const u32 cn = cnt[static_cast<u32>(idx)];
        if (cn > best_n) {
            best_n = cn;
            best_idx = idx;
        }
    }
    return best_idx;
}

static bool build_ocean_index (
    const u8* terrain,
    u16 w,
    u16 h,
    u16* ov,
    u16* ocean_n,
    u16* largest_idx,
    u32* wat_n) {
    if (terrain == nullptr || ov == nullptr || ocean_n == nullptr || largest_idx == nullptr || wat_n == nullptr || w == 0 || h == 0) {
        OI_ABORT("build_ocean_index null args");
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_4B wb_q("P1_Gen_OceanIndex", "flood_q", 0u);
    P1_WB_CHK(wb_q);
    u32* q = wb_q.get_iter_ptr();
    std::memset(ov, 0, static_cast<size_t>(n) * sizeof(u16));
    u32 wat_cnt = 0;
    u16 idx = 0;
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = terrain[i];
        if (is_ocn_wat(cls)) {
            wat_cnt++;
        }
        if (is_inland_wat(cls)) {
            continue;
        }
        if (!is_ocn_wat(cls) || ov[i] != static_cast<u16>(P1_OCEAN_IDX_NONE)) {
            continue;
        }
        if (idx >= static_cast<u16>(P1_OCEAN_IDX_MAX)) {
            break;
        }
        idx = static_cast<u16>(idx + 1u);
        if (!flood_ocean_comp(terrain, w, h, idx, i, ov, q)) {
            OI_ABORT("build_ocean_index flood failed");
        }
    }
    *ocean_n = idx;
    *wat_n = wat_cnt;
    *largest_idx = find_largest_ocean_idx(ov, n, idx);
    return true;
}

//================================================================================================================================
//=> - P1_Gen_OceanIndex -
//================================================================================================================================

P1_Gen_OceanIndex::P1_Gen_OceanIndex (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_ocean_n = 0;
    m_rslt.m_largest_idx = static_cast<u16>(P1_OCEAN_IDX_NONE);
    m_rslt.m_wat_n = 0;
}

void P1_Gen_OceanIndex::clear_rslt () {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_ocean_n = 0;
    m_rslt.m_largest_idx = static_cast<u16>(P1_OCEAN_IDX_NONE);
    m_rslt.m_wat_n = 0;
    m_rslt.m_ov.clear();
}

bool P1_Gen_OceanIndex::generate (const u8* terrain, u16 w, u16 h) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        OI_ABORT("generate invalid args");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        OI_ABORT("generate size mismatch");
    }
    if (!m_rslt.m_ov.resize(w, h)) {
        OI_ABORT("generate overlay resize failed");
    }
    u16* ov = m_rslt.m_ov.data_w();
    if (ov == nullptr) {
        OI_ABORT("generate overlay null");
    }
    u16 ocean_n = 0;
    u16 largest_idx = static_cast<u16>(P1_OCEAN_IDX_NONE);
    u32 wat_n = 0;
    if (!build_ocean_index(terrain, w, h, ov, &ocean_n, &largest_idx, &wat_n)) {
        return false;
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_ocean_n = ocean_n;
    m_rslt.m_largest_idx = largest_idx;
    m_rslt.m_wat_n = wat_n;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_OceanIndex::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_OceanIndexRslt& P1_Gen_OceanIndex::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
