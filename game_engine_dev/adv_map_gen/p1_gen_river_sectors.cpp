//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstring>

#include "p1_gen_river_sectors.h"
#include "generator_constants.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"

#define RS_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverSectors", msg)

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const i32 k_ring_w = 3;
static const u16 k_sb_conn_lim = 16u;
static const u32 k_sq_slot = 256u;

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

struct QItem {
    i32 m_dsq;
    u16 m_x;
    u16 m_y;
};

struct SectQ {
    QItem* m_a;
    u32 m_n;
    u32 m_cap;
};

struct SectBuild {
    u16 m_cx;
    u16 m_cy;
    u8 m_terr;
    u16 m_conn_n;
    u16 m_conn[k_sb_conn_lim];
};

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_mountain (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool tile_blocked (const u8* terrain, const u8* glob, u32 ti) {
    if (is_mountain(terrain[ti])) {
        return true;
    }
    if (glob[ti] != 0) {
        return true;
    }
    return false;
}

static u8 sect_terr (const u8* terrain, const u8* glob, u32 ti) {
    const u8 c = terrain[ti];
    if (is_water(c) && glob[ti] == 0) {
        return TERR_PLAINS[0];
    }
    return c;
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

static bool fill_glob_ocn_mask (const u8* terrain, u16 w, u16 h, u8* mask, u32* q) {
    if (terrain == nullptr || mask == nullptr || q == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(mask, 0, n);
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
    return true;
}

static void sq_bind (SectQ* q, QItem* base, u32 cap) {
    q->m_a = base;
    q->m_n = 0;
    q->m_cap = cap;
}

static bool sq_has_xy (const SectQ* q, u16 x, u16 y) {
    for (u32 i = 0; i < q->m_n; ++i) {
        if (q->m_a[i].m_x == x && q->m_a[i].m_y == y) {
            return true;
        }
    }
    return false;
}

static bool sq_push_sorted (SectQ* q, QItem it) {
    if (sq_has_xy(q, it.m_x, it.m_y)) {
        return true;
    }
    if (q->m_n >= q->m_cap) {
        return false;
    }
    u32 pos = q->m_n;
    for (u32 i = 0; i < q->m_n; ++i) {
        if (q->m_a[i].m_dsq > it.m_dsq) {
            pos = i;
            break;
        }
    }
    for (u32 i = q->m_n; i > pos; --i) {
        q->m_a[i] = q->m_a[i - 1u];
    }
    q->m_a[pos] = it;
    q->m_n++;
    return true;
}

static void sq_erase_at (SectQ* q, u32 idx) {
    if (idx >= q->m_n) {
        return;
    }
    for (u32 i = idx + 1u; i < q->m_n; ++i) {
        q->m_a[i - 1u] = q->m_a[i];
    }
    q->m_n--;
}

static bool sb_add_conn (SectBuild* a, SectBuild* b, u16 ai, u16 bi) {
    bool fnd = false;
    for (u16 i = 0; i < a->m_conn_n; ++i) {
        if (a->m_conn[i] == bi) {
            fnd = true;
            break;
        }
    }
    if (!fnd) {
        if (a->m_conn_n >= k_sb_conn_lim) {
            return false;
        }
        a->m_conn[a->m_conn_n++] = bi;
    }
    fnd = false;
    for (u16 i = 0; i < b->m_conn_n; ++i) {
        if (b->m_conn[i] == ai) {
            fnd = true;
            break;
        }
    }
    if (!fnd) {
        if (b->m_conn_n >= k_sb_conn_lim) {
            return false;
        }
        b->m_conn[b->m_conn_n++] = ai;
    }
    return true;
}

static bool claim_river_sectors (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    P1_Gen_RiverSectorsRslt* out) {
    if (terrain == nullptr || out == nullptr || pts.m_n == 0 || !pts.m_que.ok()) {
        RS_ABORT("claim_river_sectors null args or empty pts");
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_1B wb_glob("P1_Gen_RiverSectors", "glob", 0u);
    Whiteboard_4B wb_q("P1_Gen_RiverSectors", "glob_q", 0u);
    P1_WB_CHK(wb_glob);
    P1_WB_CHK(wb_q);
    u8* glob = wb_glob.raw();
    u32* q = reinterpret_cast<u32*>(wb_q.get_iter_ptr());
    if (!fill_glob_ocn_mask(terrain, w, h, glob, q)) {
        RS_ABORT("claim_river_sectors glob ocean mask failed");
    }
    const u32 sector_n = pts.m_n;
    if (sector_n * k_sq_slot > WhiteboardMng::tile_n()) {
        RS_ABORT("claim_river_sectors sq pool overflow");
    }
    Whiteboard_8B wb_sq("P1_Gen_RiverSectors", "sq_pool", 0u);
    Whiteboard_4B wb_act("P1_Gen_RiverSectors", "act", 0u);
    P1_WB_CHK(wb_sq);
    P1_WB_CHK(wb_act);
    QItem* sq_pool = reinterpret_cast<QItem*>(wb_sq.get_iter_ptr());
    i32* act = reinterpret_cast<i32*>(wb_act.get_iter_ptr());
    SectBuild* secs = new SectBuild[sector_n];
    SectQ* qs = new SectQ[sector_n];
    if (secs == nullptr || qs == nullptr) {
        delete[] secs;
        delete[] qs;
        RS_ABORT("claim_river_sectors sector alloc failed");
    }
    for (u32 si = 0; si < sector_n; ++si) {
        secs[si].m_conn_n = 0;
        sq_bind(&qs[si], sq_pool + si * k_sq_slot, k_sq_slot);
        secs[si].m_cx = pts.m_que.x_at(si);
        secs[si].m_cy = pts.m_que.y_at(si);
    }
    Whiteboard_2B wb_clm("P1_Gen_RiverSectors", "clm", 0u);
    P1_WB_CHK(wb_clm);
    u16* overlay = new u16[n];
    u16* clm = wb_clm.get_iter_ptr();
    if (overlay == nullptr) {
        delete[] secs;
        delete[] qs;
        RS_ABORT("claim_river_sectors overlay alloc failed");
    }
    for (u32 i = 0; i < n; ++i) {
        clm[i] = 0;
        overlay[i] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 si = 0; si < sector_n; ++si) {
        const u16 px = secs[si].m_cx;
        const u16 py = secs[si].m_cy;
        if (px >= w || py >= h) {
            continue;
        }
        const u32 ti = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        if (tile_blocked(terrain, glob, ti)) {
            continue;
        }
        secs[si].m_terr = sect_terr(terrain, glob, ti);
        clm[ti] = 1;
        overlay[ti] = static_cast<u16>(si);
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (tile_blocked(terrain, glob, ni)) {
                continue;
            }
            if (clm[ni] != 0) {
                const u16 oid = overlay[ni];
                if (oid != static_cast<u16>(P1_RIVER_SECTOR_NONE) && oid != static_cast<u16>(si)) {
                    if (!sb_add_conn(&secs[si], &secs[oid], static_cast<u16>(si), oid)) {
                        RS_ABORT("claim_river_sectors sector conn overflow");
                    }
                }
                continue;
            }
            if (sect_terr(terrain, glob, ni) != secs[si].m_terr) {
                continue;
            }
            QItem e;
            e.m_dsq = (nx - static_cast<i32>(px)) * (nx - static_cast<i32>(px))
                + (ny - static_cast<i32>(py)) * (ny - static_cast<i32>(py));
            e.m_x = static_cast<u16>(nx);
            e.m_y = static_cast<u16>(ny);
            if (!sq_push_sorted(&qs[si], e)) {
                RS_ABORT("claim_river_sectors sector queue overflow");
            }
        }
    }
    u32 act_n = 0;
    for (u32 si = 0; si < sector_n; ++si) {
        act[act_n++] = static_cast<i32>(si);
    }
    while (act_n > 0) {
        i32 min_dsq = -1;
        for (u32 ai = 0; ai < act_n; ++ai) {
            const i32 si = act[ai];
            if (qs[static_cast<u32>(si)].m_n == 0) {
                continue;
            }
            for (u32 qi = 0; qi < qs[static_cast<u32>(si)].m_n; ++qi) {
                const QItem& e = qs[static_cast<u32>(si)].m_a[qi];
                const u32 ti = static_cast<u32>(e.m_y) * static_cast<u32>(w) + static_cast<u32>(e.m_x);
                if (clm[ti] == 0) {
                    min_dsq = (min_dsq == -1 || e.m_dsq < min_dsq) ? e.m_dsq : min_dsq;
                    break;
                }
            }
        }
        if (min_dsq == -1) {
            break;
        }
        const i32 min_d = static_cast<i32>(std::sqrt(static_cast<double>(min_dsq)));
        const i32 max_d = min_d + k_ring_w;
        const i32 max_dsq = max_d * max_d;
        u32 new_act_n = 0;
        i32 claimed_cnt = 0;
        for (u32 ai = 0; ai < act_n; ++ai) {
            const i32 si = act[ai];
            if (qs[static_cast<u32>(si)].m_n == 0) {
                continue;
            }
            const u16 px = secs[static_cast<u32>(si)].m_cx;
            const u16 py = secs[static_cast<u32>(si)].m_cy;
            u32 qidx = 0;
            while (qidx < qs[static_cast<u32>(si)].m_n) {
                const QItem e = qs[static_cast<u32>(si)].m_a[qidx];
                const u16 x = e.m_x;
                const u16 y = e.m_y;
                const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
                if (clm[ti] != 0) {
                    sq_erase_at(&qs[static_cast<u32>(si)], qidx);
                    continue;
                }
                if (e.m_dsq < min_dsq || e.m_dsq > max_dsq) {
                    qidx++;
                    continue;
                }
                if (tile_blocked(terrain, glob, ti) || sect_terr(terrain, glob, ti) != secs[static_cast<u32>(si)].m_terr) {
                    sq_erase_at(&qs[static_cast<u32>(si)], qidx);
                    continue;
                }
                sq_erase_at(&qs[static_cast<u32>(si)], qidx);
                clm[ti] = 1;
                overlay[ti] = static_cast<u16>(si);
                claimed_cnt++;
                for (i32 d = 0; d < 4; ++d) {
                    const i32 nx = static_cast<i32>(x) + dx4[d];
                    const i32 ny = static_cast<i32>(y) + dy4[d];
                    if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                        continue;
                    }
                    const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                    if (tile_blocked(terrain, glob, ni)) {
                        continue;
                    }
                    if (clm[ni] != 0) {
                        const u16 oid = overlay[ni];
                        if (oid != static_cast<u16>(P1_RIVER_SECTOR_NONE) && oid != static_cast<u16>(si)) {
                            if (!sb_add_conn(&secs[static_cast<u32>(si)], &secs[oid], static_cast<u16>(si), oid)) {
                                RS_ABORT("claim_river_sectors sector conn overflow");
                            }
                        }
                        continue;
                    }
                    if (sect_terr(terrain, glob, ni) != secs[static_cast<u32>(si)].m_terr) {
                        continue;
                    }
                    QItem ne;
                    ne.m_dsq = (nx - static_cast<i32>(px)) * (nx - static_cast<i32>(px))
                        + (ny - static_cast<i32>(py)) * (ny - static_cast<i32>(py));
                    ne.m_x = static_cast<u16>(nx);
                    ne.m_y = static_cast<u16>(ny);
                    if (!sq_push_sorted(&qs[static_cast<u32>(si)], ne)) {
                        RS_ABORT("claim_river_sectors sector queue overflow");
                    }
                }
                break;
            }
            bool has_unclaimed = false;
            for (u32 qi = 0; qi < qs[static_cast<u32>(si)].m_n; ++qi) {
                const QItem& e = qs[static_cast<u32>(si)].m_a[qi];
                const u32 ti = static_cast<u32>(e.m_y) * static_cast<u32>(w) + static_cast<u32>(e.m_x);
                if (clm[ti] == 0) {
                    has_unclaimed = true; 
                    break;
                }
            }
            if (has_unclaimed) {
                act[new_act_n++] = si;
            }
        }
        if (claimed_cnt == 0) {
            break;
        }
        act_n = new_act_n;
    }
    P1_RiverSectorNode* nodes = new P1_RiverSectorNode[sector_n];
    if (nodes == nullptr) {
        delete[] overlay;
        delete[] secs;
        delete[] qs;
        RS_ABORT("claim_river_sectors nodes alloc failed");
    }
    for (u32 si = 0; si < sector_n; ++si) {
        nodes[si].m_cx = secs[si].m_cx;
        nodes[si].m_cy = secs[si].m_cy;
        nodes[si].m_terr = secs[si].m_terr;
        nodes[si].m_conn_n = secs[si].m_conn_n;
        nodes[si].m_conn = nullptr;
        if (nodes[si].m_conn_n > 0) {
            nodes[si].m_conn = new u16[nodes[si].m_conn_n];
            if (nodes[si].m_conn == nullptr) {
                for (u32 j = 0; j < si; ++j) {
                    delete[] nodes[j].m_conn;
                }
                delete[] nodes;
                delete[] overlay;
                delete[] secs;
                delete[] qs;
                RS_ABORT("claim_river_sectors node conn alloc failed");
            }
            for (u16 c = 0; c < nodes[si].m_conn_n; ++c) {
                nodes[si].m_conn[c] = secs[si].m_conn[c];
            }
        }
    }
    delete[] secs;
    delete[] qs;
    out->m_w = w;
    out->m_h = h;
    out->m_sector_n = static_cast<u16>(sector_n);
    out->m_ov = overlay;
    out->m_nodes = nodes;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverSectors -
//================================================================================================================================

P1_Gen_RiverSectors::P1_Gen_RiverSectors (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sector_n = 0;
    m_rslt.m_ov = nullptr;
    m_rslt.m_nodes = nullptr;
}

P1_Gen_RiverSectors::~P1_Gen_RiverSectors () {
    clear_rslt();
}

void P1_Gen_RiverSectors::clear_rslt () {
    if (m_rslt.m_nodes != nullptr) {
        for (u32 i = 0; i < static_cast<u32>(m_rslt.m_sector_n); ++i) {
            delete[] m_rslt.m_nodes[i].m_conn;
        }
        delete[] m_rslt.m_nodes;
    }
    delete[] m_rslt.m_ov;
    m_rslt.m_nodes = nullptr;
    m_rslt.m_ov = nullptr;
    m_rslt.m_sector_n = 0;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverSectors::generate (const u8* terrain, u16 w, u16 h, const P1_Gen_RiverPtsRslt& pts) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || pts.m_n == 0 || !pts.m_que.ok()) {
        if (terrain == nullptr) {
            RS_ABORT("null terrain");
        }
        if (!p1_map_size_ok(w, h)) {
            RS_ABORT("invalid map size");
        }
        RS_ABORT("empty river pts");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        RS_ABORT("size mismatch");
    }
    if (!claim_river_sectors(terrain, w, h, pts, &m_rslt)) {
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverSectors::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverSectorsRslt& P1_Gen_RiverSectors::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
