//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_river_network.h"
#include "generator_constants.h"
#include "p1_gen_coastal_mtn_limits.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static bool is_hills (u8 terr) {
    return terr == TERR_HILLS[0];
}

static bool is_plains (u8 terr) {
    return terr == TERR_PLAINS[0];
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool conn_ok (const P1_RiverSectorNode* from, const P1_RiverSectorNode* to) {
    if (is_hills(from->m_terr) && is_plains(to->m_terr)) {
        return false;
    }
    return true;
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static void glob_seed (const u8* terrain, u16 w, u16 h, u32 x, u32 y, u8* mask, u32* q, u32* qn) {
    if (x >= static_cast<u32>(w) || y >= static_cast<u32>(h)) {
        return;
    }
    const u32 i = tidx(w, x, y);
    if (!is_water(terrain[i]) || mask[i] != 0) {
        return;
    }
    mask[i] = 1;
    q[*qn] = i;
    *qn = *qn + 1u;
}

static u8* build_glob_ocn_mask (const u8* terrain, u16 w, u16 h) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* mask = new u8[n];
    if (mask == nullptr) {
        return nullptr;
    }
    std::memset(mask, 0, n);
    u32* q = new u32[n];
    if (q == nullptr) {
        delete[] mask;
        return nullptr;
    }
    u32 qn = 0;
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 x = 0; x < wi; ++x) {
        glob_seed(terrain, w, h, x, 0, mask, q, &qn);
        glob_seed(terrain, w, h, x, hi - 1u, mask, q, &qn);
    }
    for (u32 y = 0; y < hi; ++y) {
        glob_seed(terrain, w, h, 0, y, mask, q, &qn);
        glob_seed(terrain, w, h, wi - 1u, y, mask, q, &qn);
    }
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == TERR_OCEAN[0]) {
            glob_seed(terrain, w, h, i % wi, i / wi, mask, q, &qn);
        }
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
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
            if (!is_water(terrain[ni]) || mask[ni] != 0) {
                continue;
            }
            mask[ni] = 1;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return mask;
}

static void swap_u16 (u16* a, u16* b) {
    const u16 t = *a;
    *a = *b;
    *b = t;
}

static bool conn_has_pair (const P1_RiverConn* conns, u16 conn_n, u16 a, u16 b) {
    const u16 lo = a < b ? a : b;
    const u16 hi = a < b ? b : a;
    for (u16 i = 0; i < conn_n; ++i) {
        const u16 ca = conns[i].m_a < conns[i].m_b ? conns[i].m_a : conns[i].m_b;
        const u16 cb = conns[i].m_a < conns[i].m_b ? conns[i].m_b : conns[i].m_a;
        if (ca == lo && cb == hi) {
            return true;
        }
    }
    return false;
}

static bool conn_push (P1_RiverConn** conns, u16* conn_n, u16* conn_cap, u16 a, u16 b) {
    if (conn_has_pair(*conns, *conn_n, a, b)) {
        return true;
    }
    if (*conn_n >= *conn_cap) {
        const u16 nc = *conn_cap < 64 ? 64 : static_cast<u16>(*conn_cap * 2u);
        P1_RiverConn* na = new P1_RiverConn[nc];
        if (na == nullptr) {
            return false;
        }
        for (u16 i = 0; i < *conn_n; ++i) {
            na[i] = (*conns)[i];
        }
        delete[] *conns;
        *conns = na;
        *conn_cap = nc;
    }
    (*conns)[*conn_n].m_a = a;
    (*conns)[*conn_n].m_b = b;
    *conn_n = *conn_n + 1u;
    return true;
}

static void build_basins (
    P1_Gen_RiverNetworkRslt* out,
    const P1_Gen_RiverSectorsRslt& sectors) {
    if (out == nullptr || out->m_ov == nullptr || out->m_river_sys == nullptr) {
        return;
    }
    const u32 n = static_cast<u32>(out->m_w) * static_cast<u32>(out->m_h);
    const u32 sector_n = static_cast<u32>(out->m_sector_n);
    u16* sys_idx = new u16[sector_n];
    if (sys_idx == nullptr) {
        return;
    }
    for (u32 si = 0; si < sector_n; ++si) {
        sys_idx[si] = static_cast<u16>(P1_RIVER_BASIN_NONE);
    }
    u16 root_n = 0;
    u16* roots = new u16[sector_n];
    if (roots == nullptr) {
        delete[] sys_idx;
        return;
    }
    for (u32 si = 0; si < sector_n; ++si) {
        const u16 sys = out->m_river_sys[si];
        if (sys == static_cast<u16>(P1_RIVER_SYS_NONE)) {
            continue;
        }
        bool fnd = false;
        for (u16 ri = 0; ri < root_n; ++ri) {
            if (roots[ri] == sys) {
                fnd = true;
                break;
            }
        }
        if (!fnd) {
            roots[root_n++] = sys;
        }
    }
    if (root_n == 0) {
        delete[] roots;
        delete[] sys_idx;
        out->m_basin_n = 0;
        return;
    }
    P1_RiverBasinEntry* recs = new P1_RiverBasinEntry[root_n];
    if (recs == nullptr) {
        delete[] roots;
        delete[] sys_idx;
        return;
    }
    u16 next_idx = 1;
    for (u16 ri = 0; ri < root_n; ++ri) {
        const u16 root = roots[ri];
        recs[ri].m_idx = next_idx;
        recs[ri].m_mouth_x = sectors.m_nodes[root].m_cx;
        recs[ri].m_mouth_y = sectors.m_nodes[root].m_cy;
        recs[ri].m_tile_n = 0;
        for (u32 si = 0; si < sector_n; ++si) {
            if (out->m_river_sys[si] == root) {
                sys_idx[si] = next_idx;
            }
        }
        next_idx++;
    }
    delete[] roots;
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sectors.m_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= out->m_sector_n) {
            continue;
        }
        const u16 bidx = sys_idx[sid];
        if (bidx == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        out->m_ov[i] = bidx;
        for (u16 bi = 0; bi < root_n; ++bi) {
            if (recs[bi].m_idx == bidx) {
                recs[bi].m_tile_n++;
                break;
            }
        }
    }
    delete[] sys_idx;
    for (u16 i = 0; i < root_n; ++i) {
        for (u16 j = static_cast<u16>(i + 1u); j < root_n; ++j) {
            if (recs[j].m_tile_n > recs[i].m_tile_n) {
                P1_RiverBasinEntry t = recs[i];
                recs[i] = recs[j];
                recs[j] = t;
            }
        }
    }
    out->m_basin_n = root_n;
    out->m_basins = recs;
}

static void build_sec_grey_mask (
    u16 w,
    u16 h,
    const u16* sec_ov,
    u16 sector_n,
    const u8* lim_ov,
    bool* sec_grey) 
{
    if (sec_ov == nullptr || lim_ov == nullptr || sec_grey == nullptr) {
        return;
    }
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        sec_grey[si] = false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (lim_ov[i] != static_cast<u8>(P1_COASTAL_MTN_OV_BLK)) {
            continue;
        }
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        sec_grey[sid] = true;
    }
}

static bool build_river_network (
    u32 seed,
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_CoastalMtnLimitsRslt& coast_lim,
    P1_Gen_RiverNetworkRslt* out) {
    if (terrain == nullptr || out == nullptr || sectors.m_ov == nullptr || sectors.m_nodes == nullptr || sectors.m_sector_n == 0) {
        return false;
    }
    const u8* lim_ov = coast_lim.m_limit_ov.data();
    if (lim_ov == nullptr || coast_lim.m_w != w || coast_lim.m_h != h) {
        return false;
    }
    const u32 sector_n = static_cast<u32>(sectors.m_sector_n);
    u8* glob = build_glob_ocn_mask(terrain, w, h);
    if (glob == nullptr) {
        return false;
    }
    bool* has_land = new bool[sector_n];
    bool* touch_glob = new bool[sector_n];
    bool* sec_grey = new bool[sector_n];
    if (has_land == nullptr || touch_glob == nullptr || sec_grey == nullptr) {
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    std::memset(has_land, 0, sector_n * sizeof(bool));
    std::memset(touch_glob, 0, sector_n * sizeof(bool));
    build_sec_grey_mask(w, h, sectors.m_ov, sectors.m_sector_n, lim_ov, sec_grey);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 py = 0; py < static_cast<u32>(h); ++py) {
        for (u32 px = 0; px < static_cast<u32>(w); ++px) {
            const u32 ti = py * static_cast<u32>(w) + px;
            const u16 sid = sectors.m_ov[ti];
            if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
                continue;
            }
            has_land[sid] = true;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (glob[ni] != 0) {
                    touch_glob[sid] = true;
                    break;
                }
            }
        }
    }
    u16* coast_cand = new u16[sector_n];
    if (coast_cand == nullptr) {
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    u32 coast_n = 0;
    for (u32 si = 0; si < sector_n; ++si) {
        if (has_land[si] && touch_glob[si] && !sec_grey[si]) {
            coast_cand[coast_n++] = static_cast<u16>(si);
        }
    }
    Rng32 rng;
    rng_seed(&rng, seed);
    for (u32 i = coast_n; i > 1u; --i) {
        const u32 j = rng_next(&rng) % i;
        swap_u16(&coast_cand[i - 1u], &coast_cand[j]);
    }
    bool* is_mouth = new bool[sector_n];
    u8* ws_claim = new u8[sector_n];
    if (is_mouth == nullptr || ws_claim == nullptr) {
        delete[] ws_claim;
        delete[] is_mouth;
        delete[] coast_cand;
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    std::memset(is_mouth, 0, sector_n * sizeof(bool));
    for (u32 si = 0; si < sector_n; ++si) {
        ws_claim[si] = 0;
    }
    u32 mouth_i = 0;
    for (u32 ci = 0; ci < coast_n; ++ci) {
        const u16 sid = coast_cand[ci];
        if ((ci & 1u) != 0u) {
            continue;
        }
        is_mouth[sid] = true;
        ws_claim[sid] = static_cast<u8>((mouth_i % 4u) + 1u);
        mouth_i++;
    }
    delete[] coast_cand;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u16* overlay = new u16[npx];
    u16* river_sys = new u16[sector_n];
    bool* coastal = new bool[sector_n];
    if (overlay == nullptr || river_sys == nullptr || coastal == nullptr) {
        delete[] overlay;
        delete[] river_sys;
        delete[] coastal;
        delete[] is_mouth;
        delete[] ws_claim;
        delete[] coast_cand;
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    for (u32 i = 0; i < npx; ++i) {
        overlay[i] = static_cast<u16>(P1_RIVER_BASIN_NONE);
    }
    for (u32 si = 0; si < sector_n; ++si) {
        river_sys[si] = static_cast<u16>(P1_RIVER_SYS_NONE);
        coastal[si] = has_land[si] && touch_glob[si];
    }
    bool* clmd = new bool[sector_n];
    if (clmd == nullptr) {
        delete[] overlay;
        delete[] river_sys;
        delete[] coastal;
        delete[] is_mouth;
        delete[] ws_claim;
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    std::memset(clmd, 0, sector_n * sizeof(bool));
    u16* q = new u16[sector_n];
    u16* nq = new u16[sector_n];
    if (q == nullptr || nq == nullptr) {
        delete[] nq;
        delete[] q;
        delete[] clmd;
        delete[] overlay;
        delete[] river_sys;
        delete[] coastal;
        delete[] ws_claim;
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    u32 q_n = 0;
    for (u32 si = 0; si < sector_n; ++si) {
        if (is_mouth[si]) {
            river_sys[si] = static_cast<u16>(si);
            clmd[si] = true;
            q[q_n++] = static_cast<u16>(si);
        }
    }
    delete[] is_mouth;
    P1_RiverConn* conns = nullptr;
    u16 conn_n = 0;
    u16 conn_cap = 0;
    u16* nb = new u16[sector_n];
    if (nb == nullptr) {
        delete[] nq;
        delete[] q;
        delete[] ws_claim;
        delete[] clmd;
        delete[] overlay;
        delete[] river_sys;
        delete[] coastal;
        delete[] conns;
        delete[] sec_grey;
        delete[] glob;
        delete[] has_land;
        delete[] touch_glob;
        return false;
    }
    while (q_n > 0) {
        u32 nq_n = 0;
        u32 round_claims = 0;
        for (u32 qi = 0; qi < q_n; ++qi) {
            const u16 sidx = q[qi];
            const u16 root = river_sys[sidx];
            const P1_RiverSectorNode* sec = &sectors.m_nodes[sidx];
            u32 nb_n = 0;
            for (u16 c = 0; c < sec->m_conn_n; ++c) {
                const u16 aid = sec->m_conn[c];
                if (!clmd[aid] && !sec_grey[aid] && conn_ok(sec, &sectors.m_nodes[aid])) {
                    nb[nb_n++] = aid;
                }
            }
            if (nb_n == 0) {
                continue;
            }
            u32 claim_k = static_cast<u32>(ws_claim[root]);
            if (claim_k == 0u) {
                claim_k = 1u;
            }
            if (claim_k > nb_n) {
                claim_k = nb_n;
            }
            bool claimed_any = false;
            for (u32 ki = 0; ki < claim_k; ++ki) {
                const u16 aid = nb[ki];
                if (clmd[aid]) {
                    continue;
                }
                clmd[aid] = true;
                river_sys[aid] = root;
                if (!conn_push(&conns, &conn_n, &conn_cap, sidx, aid)) {
                    continue;
                }
                nq[nq_n++] = aid;
                claimed_any = true;
                round_claims++;
            }
            if (claimed_any) {
                nq[nq_n++] = sidx;
            }
        }
        if (round_claims == 0) {
            break;
        }
        for (u32 i = 0; i < nq_n; ++i) {
            q[i] = nq[i];
        }
        q_n = nq_n;
    }
    delete[] ws_claim;
    delete[] nq;
    delete[] nb;
    delete[] q;
    delete[] clmd;
    delete[] sec_grey;
    delete[] glob;
    delete[] has_land;
    delete[] touch_glob;
    out->m_w = w;
    out->m_h = h;
    out->m_sector_n = sectors.m_sector_n;
    out->m_conn_n = conn_n;
    out->m_conns = conns;
    out->m_river_sys = river_sys;
    out->m_coastal = coastal;
    out->m_ov = overlay;
    out->m_basin_n = 0;
    out->m_basins = nullptr;
    build_basins(out, sectors);
    return true;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    std::fwrite(rgb, 1, nbytes, fp);
    std::fclose(fp);
    return true;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static void draw_thick_line (
    u8* rgb,
    u16 w,
    u16 h,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    u8 r,
    u8 g,
    u8 b,
    i32 thick) {
    i32 dx = x1 - x2;
    if (dx < 0) {
        dx = -dx;
    }
    i32 dy = y1 - y2;
    if (dy < 0) {
        dy = -dy;
    }
    i32 sx = (x1 < x2) ? 1 : -1;
    i32 sy = (y1 < y2) ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x1;
    i32 y = y1;
    while (true) {
        for (i32 ty = -thick; ty <= thick; ++ty) {
            for (i32 tx = -thick; tx <= thick; ++tx) {
                if (tx * tx + ty * ty > thick * thick) {
                    continue;
                }
                const i32 px = x + tx;
                const i32 py = y + ty;
                if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 p = (static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)) * 3u;
                rgb[p + 0] = r;
                rgb[p + 1] = g;
                rgb[p + 2] = b;
            }
        }
        if (x == x2 && y == y2) {
            break;
        }
        const i32 e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

//================================================================================================================================
//=> - P1_Gen_RiverNetwork -
//================================================================================================================================

P1_Gen_RiverNetwork::P1_Gen_RiverNetwork (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

P1_Gen_RiverNetwork::~P1_Gen_RiverNetwork () {
    clear_rslt();
}

void P1_Gen_RiverNetwork::clear_rslt () {
    delete[] m_rslt.m_conns;
    delete[] m_rslt.m_river_sys;
    delete[] m_rslt.m_coastal;
    delete[] m_rslt.m_ov;
    if (m_rslt.m_basins != nullptr) {
        delete[] m_rslt.m_basins;
    }
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

bool P1_Gen_RiverNetwork::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_CoastalMtnLimitsRslt& coast_lim) 
{
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || sectors.m_sector_n == 0) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_river_network(m_prm.m_seed, terrain, w, h, sectors, coast_lim, &m_rslt)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverNetwork::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverNetworkRslt& P1_Gen_RiverNetwork::result () const {
    return m_rslt;
}

void P1_Gen_RiverNetwork::save_output (cstr path, const u8* terrain, const P1_Gen_RiverSectorsRslt& sectors) const {
    if (!m_valid_generation || path == nullptr || terrain == nullptr || m_rslt.m_ov == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        terr_rgb(terrain[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    u16 pal_cap = static_cast<u16>(m_rslt.m_basin_n + 1u);
    u8* pal = new u8[static_cast<size_t>(pal_cap) * 3u];
    bool* pal_set = new bool[pal_cap];
    if (pal == nullptr || pal_set == nullptr) {
        delete[] pal;
        delete[] pal_set;
        delete[] rgb;
        return;
    }
    for (u16 i = 0; i < pal_cap; ++i) {
        pal_set[i] = false;
    }
    Rng32 rng;
    rng_seed(&rng, m_prm.m_seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        const u16 bid = m_rslt.m_ov[i];
        if (bid == static_cast<u16>(P1_RIVER_BASIN_NONE) || bid >= pal_cap) {
            continue;
        }
        if (!pal_set[bid]) {
            pal[bid * 3u + 0] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 1] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 2] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal_set[bid] = true;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = pal[bid * 3u + 0];
        rgb[p + 1] = pal[bid * 3u + 1];
        rgb[p + 2] = pal[bid * 3u + 2];
    }
    const u8 lr = 0;
    const u8 lg = 100;
    const u8 lb = 255;
    if (m_rslt.m_conns != nullptr && sectors.m_nodes != nullptr) {
        for (u16 ci = 0; ci < m_rslt.m_conn_n; ++ci) {
            const u16 ia = m_rslt.m_conns[ci].m_a;
            const u16 ib = m_rslt.m_conns[ci].m_b;
            draw_thick_line(
                rgb,
                w,
                h,
                sectors.m_nodes[ia].m_cx,
                sectors.m_nodes[ia].m_cy,
                sectors.m_nodes[ib].m_cx,
                sectors.m_nodes[ib].m_cy,
                lr,
                lg,
                lb,
                1);
        }
    }
    save_rgb_ppm(path, rgb, w, h);
    delete[] pal_set;
    delete[] pal;
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
