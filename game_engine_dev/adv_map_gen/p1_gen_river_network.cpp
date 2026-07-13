//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_river_network.h"
#include "generator_constants.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "whiteboard_mng.h"

#define RN_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverNetwork", msg)

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

static bool is_open_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0];
}

static void build_glob_coast_secs (
    const u8* terrain,
    const u16* ocn,
    u16 glob_ocn,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sector_n,
    u8* coast_sec) 
{
    std::memset(coast_sec, 0, static_cast<size_t>(sector_n));
    if (glob_ocn == static_cast<u16>(P1_OCEAN_IDX_NONE) || ocn == nullptr || sec_ov == nullptr || coast_sec == nullptr) {
        return;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 py = 0; py < hi; ++py) {
        for (u32 px = 0; px < wi; ++px) {
            const u32 ti = py * wi + px;
            if (terrain[ti] == TERR_MOUNTAINS[0] || is_open_wat(terrain[ti]) || ocn[ti] == glob_ocn) {
                continue;
            }
            bool adj = false;
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
                if (ocn[ni] == glob_ocn) {
                    adj = true;
                    break;
                }
            }
            if (!adj) {
                continue;
            }
            const u16 sid = sec_ov[ti];
            if (sid != static_cast<u16>(P1_RIVER_SECTOR_NONE) && sid < sector_n) {
                coast_sec[sid] = 1u;
            }
        }
    }
}

static u8 terr_at (
    const u8* terrain,
    u16 w,
    const P1_Gen_RiverPtsRslt& pts,
    u16 si) {
    const u16 x = pts.m_que.x_at(static_cast<u32>(si));
    const u16 y = pts.m_que.y_at(static_cast<u32>(si));
    return terrain[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
}

static bool conn_ok (
    const u8* terrain,
    u16 w,
    const P1_Gen_RiverPtsRslt& pts,
    u16 from_si,
    u16 to_si) {
    if (is_hills(terr_at(terrain, w, pts, from_si)) && is_plains(terr_at(terrain, w, pts, to_si))) {
        return false;
    }
    return true;
}

static void build_sec_grey_mask (
    u16 w,
    u16 h,
    const u16* sec_ov,
    u16 sector_n,
    const u8* lim_ov,
    u8* sec_grey) {
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        sec_grey[si] = 0;
    }
    if (sec_ov == nullptr || lim_ov == nullptr || sec_grey == nullptr) {
        return;
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
        sec_grey[sid] = 1;
    }
}

static void swap_u16 (u16* a, u16* b) {
    const u16 t = *a;
    *a = *b;
    *b = t;
}

static u16 mouth_root (const u16* down, u16 si) {
    u16 cur = si;
    for (u32 guard = 0; guard < 65536u; ++guard) {
        const u16 nxt = down[cur];
        if (nxt == static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            return cur;
        }
        if (nxt == static_cast<u16>(P1_RIVER_DOWN_UNDEF)) {
            return static_cast<u16>(P1_RIVER_SECTOR_NONE);
        }
        cur = nxt;
    }
    return static_cast<u16>(P1_RIVER_SECTOR_NONE);
}

static bool fill_basin_ov (
    u16 w,
    u16 h,
    const u16* sec_ov,
    u16 sector_n,
    const u16* down,
    u16* basin_ov,
    u16* mouth_n,
    u16* claim_n) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_root("P1_Gen_RiverNetwork", "root", 0u);
    P1_WB_CHK(wb_root);
    u16* root_to_basin = wb_root.get_iter_ptr();
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        root_to_basin[si] = static_cast<u16>(P1_RIVER_BASIN_NONE);
    }
    u16 basin_n = 0;
    u16 mouths = 0;
    u16 claimed = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        if (down[si] == static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            mouths++;
        }
        if (down[si] != static_cast<u16>(P1_RIVER_DOWN_UNDEF)) {
            claimed++;
        }
    }
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        if (down[si] == static_cast<u16>(P1_RIVER_DOWN_UNDEF)) {
            continue;
        }
        const u16 root = mouth_root(down, static_cast<u16>(si));
        if (root == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
            continue;
        }
        if (root_to_basin[root] == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            basin_n++;
            root_to_basin[root] = basin_n;
        }
    }
    for (u32 i = 0; i < n; ++i) {
        basin_ov[i] = static_cast<u16>(P1_RIVER_BASIN_NONE);
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        if (down[sid] == static_cast<u16>(P1_RIVER_DOWN_UNDEF)) {
            continue;
        }
        const u16 root = mouth_root(down, sid);
        if (root == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
            continue;
        }
        basin_ov[i] = root_to_basin[root];
    }
    *mouth_n = mouths;
    *claim_n = claimed;
    return true;
}

static bool build_river_network (
    u32 seed,
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverSectAdjRslt& sect_adj,
    const P1_Gen_CoastalMtnLimitsRslt& coast_lim,
    const P1_OceanIndexRef& ocean,
    Whiteboard_2B* wb_down,
    Whiteboard_2B* wb_basin,
    P1_Gen_RiverNetworkRslt* out) {
    if (terrain == nullptr || out == nullptr || sectors.m_ov == nullptr || sectors.m_sector_n == 0
        || sect_adj.m_nb == nullptr || sect_adj.m_nb_n == nullptr || sect_adj.m_sector_n != sectors.m_sector_n
        || !p1_ocean_ref_ok(ocean) || ocean.m_w != w || ocean.m_h != h
        || wb_down == nullptr || wb_basin == nullptr || !wb_down->ok() || !wb_basin->ok() || !pts.m_que.ok()) {
        std::printf("P1_Gen_RiverNetwork build: invalid args\n");
        RN_ABORT("build_river_network null args");
    }
    const u16* ocn = ocean.m_ov;
    const u16 sector_n = sectors.m_sector_n;
    const u32 sec_n = static_cast<u32>(sector_n);
    if (sec_n > static_cast<u32>(P1_RIVER_PTS_MAX)) {
        std::printf("P1_Gen_RiverNetwork build: sector_n %u exceeds max\n", static_cast<u32>(sector_n));
        RN_ABORT("build_river_network sector_n exceeds P1_RIVER_PTS_MAX");
    }
    const u8* lim_ov = coast_lim.m_limit_ov.data();
    const u8* nb_n = sect_adj.m_nb_n;
    const u16* nb = sect_adj.m_nb;
    u8 sec_grey[P1_RIVER_PTS_MAX];
    u8 coast_sec[P1_RIVER_PTS_MAX];
    u8 ws_claim[P1_RIVER_PTS_MAX];
    u8 clmd[P1_RIVER_PTS_MAX];
    u16* down = wb_down->get_iter_ptr();
    u16* basin_ov = wb_basin->get_iter_ptr();
    if (lim_ov != nullptr && coast_lim.m_w == w && coast_lim.m_h == h) {
        build_sec_grey_mask(w, h, sectors.m_ov, sector_n, lim_ov, sec_grey);
    } else {
        std::memset(sec_grey, 0, sec_n);
    }
    Whiteboard_2B wb_coast("P1_Gen_RiverNetwork", "coast", 0u);
    Whiteboard_2B wb_sys("P1_Gen_RiverNetwork", "sys", 0u);
    Whiteboard_2B wb_fq("P1_Gen_RiverNetwork", "fq", 0u);
    Whiteboard_2B wb_nq("P1_Gen_RiverNetwork", "nq", 0u);
    P1_WB_CHK(wb_coast);
    P1_WB_CHK(wb_sys);
    P1_WB_CHK(wb_fq);
    P1_WB_CHK(wb_nq);
    u16* coast_cand = wb_coast.get_iter_ptr();
    u16* river_sys = wb_sys.get_iter_ptr();
    u16* fq = wb_fq.get_iter_ptr();
    u16* nq = wb_nq.get_iter_ptr();
    const u16 glob_ocn = ocean.m_largest_idx;
    for (u32 si = 0; si < sec_n; ++si) {
        down[si] = static_cast<u16>(P1_RIVER_DOWN_UNDEF);
        river_sys[si] = static_cast<u16>(P1_RIVER_SYS_NONE);
        ws_claim[si] = 0;
    }
    u32 mouth_i = 0;
    u32 coast_n = 0;
    if (glob_ocn == static_cast<u16>(P1_OCEAN_IDX_NONE)) {
        std::printf("P1_Gen_RiverNetwork: glob_ocn is none, no mouths assigned\n");
    } else {
        build_glob_coast_secs(terrain, ocn, glob_ocn, sectors.m_ov, w, h, sector_n, coast_sec);
        for (u32 si = 0; si < sec_n; ++si) {
            if (sec_grey[si] != 0 || coast_sec[si] == 0u) {
                continue;
            }
            coast_cand[coast_n++] = static_cast<u16>(si);
        }
        Rng32 rng;
        rng_seed(&rng, seed ^ (static_cast<u32>(glob_ocn) * 0x9E3779B9u));
        for (u32 i = coast_n; i > 1u; --i) {
            const u32 j = rng_next(&rng) % i;
            swap_u16(&coast_cand[i - 1u], &coast_cand[j]);
        }
        for (u32 ci = 0; ci < coast_n; ++ci) {
            if ((ci & 1u) != 0u) {
                continue;
            }
            const u16 sid = coast_cand[ci];
            down[sid] = static_cast<u16>(P1_RIVER_DOWN_MOUTH);
            river_sys[sid] = sid;
            ws_claim[sid] = static_cast<u8>((mouth_i % 4u) + 1u);
            mouth_i++;
        }
        if (coast_n == 0u) {
            std::printf("P1_Gen_RiverNetwork: glob_ocn %u but no global coast sectors\n", static_cast<u32>(glob_ocn));
        }
    }
    std::printf("P1_Gen_RiverNetwork glob_ocn %u coast_cand %u mouths %u\n", static_cast<u32>(glob_ocn), coast_n, mouth_i);
    std::memset(clmd, 0, sec_n);
    u32 q_n = 0;
    for (u32 si = 0; si < sec_n; ++si) {
        if (down[si] == static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            clmd[si] = 1;
            fq[q_n++] = static_cast<u16>(si);
        }
    }
    while (q_n > 0) {
        u32 nq_n = 0;
        u32 round_claims = 0;
        for (u32 qi = 0; qi < q_n; ++qi) {
            const u16 sidx = fq[qi];
            const u16 root = river_sys[sidx];
            u32 nb_cnt = static_cast<u32>(nb_n[sidx]);
            const u32 nb_base = static_cast<u32>(sidx) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
            u32 pick_n = 0;
            u16 pick[P1_RIVER_SECT_ADJ_MAX];
            for (u32 ni = 0; ni < nb_cnt; ++ni) {
                const u16 aid = nb[nb_base + ni];
                if (clmd[aid] != 0 || sec_grey[aid] != 0) {
                    continue;
                }
                if (!conn_ok(terrain, w, pts, sidx, aid)) {
                    continue;
                }
                pick[pick_n++] = aid;
            }
            if (pick_n == 0) {
                continue;
            }
            u32 claim_k = static_cast<u32>(ws_claim[root]);
            if (claim_k == 0u) {
                claim_k = 1u;
            }
            if (claim_k > pick_n) {
                claim_k = pick_n;
            }
            bool claimed_any = false;
            for (u32 ki = 0; ki < claim_k; ++ki) {
                const u16 aid = pick[ki];
                if (clmd[aid] != 0) {
                    continue;
                }
                clmd[aid] = 1;
                down[aid] = sidx;
                river_sys[aid] = root;
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
            fq[i] = nq[i];
        }
        q_n = nq_n;
    }
    u16 mouth_n = 0;
    u16 claim_n = 0;
    if (!fill_basin_ov(w, h, sectors.m_ov, sector_n, down, basin_ov, &mouth_n, &claim_n)) {
        RN_ABORT("build_river_network basin fill failed");
    }
    out->m_w = w;
    out->m_h = h;
    out->m_sector_n = sector_n;
    out->m_mouth_n = mouth_n;
    out->m_claim_n = claim_n;
    out->m_downstream = down;
    out->m_ov = basin_ov;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverNetwork -
//================================================================================================================================

P1_Gen_RiverNetwork::P1_Gen_RiverNetwork (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt(),
    m_wb_down(nullptr),
    m_wb_basin(nullptr) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sector_n = 0;
    m_rslt.m_mouth_n = 0;
    m_rslt.m_claim_n = 0;
    m_rslt.m_downstream = nullptr;
    m_rslt.m_ov = nullptr;
    m_wb_down = new Whiteboard_2B("P1_Gen_RiverNetwork", "down", prm.m_seed);
    m_wb_basin = new Whiteboard_2B("P1_Gen_RiverNetwork", "basin", prm.m_seed);
    P1_WB_CHK(*m_wb_down);
    P1_WB_CHK(*m_wb_basin);
}

P1_Gen_RiverNetwork::~P1_Gen_RiverNetwork () {
    clear_rslt();
    delete m_wb_basin;
    delete m_wb_down;
    m_wb_basin = nullptr;
    m_wb_down = nullptr;
}

void P1_Gen_RiverNetwork::clear_rslt () {
    m_rslt.m_downstream = nullptr;
    m_rslt.m_ov = nullptr;
    m_rslt.m_sector_n = 0;
    m_rslt.m_mouth_n = 0;
    m_rslt.m_claim_n = 0;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverNetwork::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverSectAdjRslt& sect_adj,
    const P1_Gen_CoastalMtnLimitsRslt& coast_lim,
    const P1_OceanIndexRef& ocean) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || sectors.m_sector_n == 0 || sectors.m_ov == nullptr || !pts.m_que.ok()) {
        std::printf("P1_Gen_RiverNetwork generate: invalid args\n");
        RN_ABORT("generate invalid args");
    }
    if (sect_adj.m_nb == nullptr || sect_adj.m_nb_n == nullptr || sect_adj.m_sector_n != sectors.m_sector_n) {
        std::printf("P1_Gen_RiverNetwork generate: invalid sector adjacency\n");
        RN_ABORT("generate invalid sector adjacency");
    }
    if (!p1_ocean_ref_ok(ocean) || ocean.m_w != w || ocean.m_h != h) {
        std::printf("P1_Gen_RiverNetwork generate: invalid ocean index\n");
        RN_ABORT("generate invalid ocean index");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        std::printf("P1_Gen_RiverNetwork generate: size mismatch\n");
        RN_ABORT("generate size mismatch");
    }
    if (!build_river_network(m_prm.m_seed, terrain, w, h, pts, sectors, sect_adj, coast_lim, ocean, m_wb_down, m_wb_basin, &m_rslt)) {
        std::printf("P1_Gen_RiverNetwork generate: build_river_network failed\n");
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

//================================================================================================================================
//=> - End -
//================================================================================================================================
