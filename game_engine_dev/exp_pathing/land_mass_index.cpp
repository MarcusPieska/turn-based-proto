//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "land_mass_index.h"

#include <cstring>

#include "game_map_defs.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_land (u8 cls) {
    if (cls == TERR_NONE[0]) {
        return false;
    }
    return !overlay_is_water_terr(cls);
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static bool flood_land_comp (const u8* terr, u16 w, u16 h, u16 idx, u32 seed_i, u16* ov, u32* q) {
    if (terr == nullptr || ov == nullptr || q == nullptr || seed_i >= static_cast<u32>(w) * static_cast<u32>(h)) {
        return false;
    }
    if (ov[seed_i] != static_cast<u16>(LAND_MASS_IDX_NONE) || !is_land(terr[seed_i])) {
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
            if (ov[ni] != static_cast<u16>(LAND_MASS_IDX_NONE) || !is_land(terr[ni])) {
                continue;
            }
            ov[ni] = idx;
            q[qn++] = ni;
        }
    }
    return true;
}

static u16 find_largest_idx (const u16* ov, u32 n, u16 mass_n) {
    if (ov == nullptr || mass_n == 0u) {
        return static_cast<u16>(LAND_MASS_IDX_NONE);
    }
    u32* cnt = new u32[static_cast<u32>(mass_n) + 1u];
    for (u32 i = 0; i <= static_cast<u32>(mass_n); ++i) {
        cnt[i] = 0u;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 idx = ov[i];
        if (idx == static_cast<u16>(LAND_MASS_IDX_NONE) || idx > mass_n) {
            continue;
        }
        cnt[static_cast<u32>(idx)]++;
    }
    u16 best_idx = static_cast<u16>(LAND_MASS_IDX_NONE);
    u32 best_n = 0u;
    for (u16 idx = 1u; idx <= mass_n; ++idx) {
        const u32 cn = cnt[static_cast<u32>(idx)];
        if (cn > best_n) {
            best_n = cn;
            best_idx = idx;
        }
    }
    delete[] cnt;
    return best_idx;
}

static bool build_land_index (const u8* terr, u16 w, u16 h, u16* ov, u16* mass_n, u16* largest_idx, u32* land_n) {
    if (terr == nullptr || ov == nullptr || mass_n == nullptr || largest_idx == nullptr || land_n == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    std::memset(ov, 0, static_cast<size_t>(n) * sizeof(u16));
    u32 land_cnt = 0;
    u16 idx = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_land(terr[i])) {
            land_cnt++;
        }
        if (!is_land(terr[i]) || ov[i] != static_cast<u16>(LAND_MASS_IDX_NONE)) {
            continue;
        }
        if (idx == U16_KEY_NULL) {
            delete[] q;
            return false;
        }
        idx = static_cast<u16>(idx + 1u);
        if (!flood_land_comp(terr, w, h, idx, i, ov, q)) {
            delete[] q;
            return false;
        }
    }
    *mass_n = idx;
    *land_n = land_cnt;
    *largest_idx = find_largest_idx(ov, n, idx);
    delete[] q;
    return true;
}

//================================================================================================================================
//=> - LandMassIndex -
//================================================================================================================================

LandMassIndex::LandMassIndex () :
    m_ok(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_mass_n = 0;
    m_rslt.m_largest_idx = static_cast<u16>(LAND_MASS_IDX_NONE);
    m_rslt.m_land_n = 0;
    m_rslt.m_ov = nullptr;
}

LandMassIndex::~LandMassIndex () {
    clear_rslt();
}

void LandMassIndex::clear_rslt () {
    delete[] m_rslt.m_ov;
    m_rslt.m_ov = nullptr;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_mass_n = 0;
    m_rslt.m_largest_idx = static_cast<u16>(LAND_MASS_IDX_NONE);
    m_rslt.m_land_n = 0;
    m_ok = false;
}

bool LandMassIndex::generate (const u8* terr, u16 w, u16 h) {
    clear_rslt();
    if (terr == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* ov = new u16[n];
    u16 mass_n = 0;
    u16 largest_idx = static_cast<u16>(LAND_MASS_IDX_NONE);
    u32 land_n = 0;
    if (!build_land_index(terr, w, h, ov, &mass_n, &largest_idx, &land_n)) {
        delete[] ov;
        return false;
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_mass_n = mass_n;
    m_rslt.m_largest_idx = largest_idx;
    m_rslt.m_land_n = land_n;
    m_rslt.m_ov = ov;
    m_ok = true;
    return true;
}

bool LandMassIndex::is_valid () const {
    return m_ok;
}

const LandMassIndexRslt& LandMassIndex::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
