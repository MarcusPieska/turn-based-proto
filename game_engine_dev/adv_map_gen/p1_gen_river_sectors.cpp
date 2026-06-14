//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <cstring>

#include "p1_gen_river_sectors.h"
#include "generator_constants.h"
#include "whiteboard.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const i32 k_ring_w = 3;

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
    u16 m_conn_cap;
    u16* m_conn;
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
    const u32 qcap = n;
    u32* q = new u32[qcap];
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

static bool sq_init (SectQ* q, u32 cap) {
    q->m_a = new QItem[cap];
    if (q->m_a == nullptr) {
        q->m_n = 0;
        q->m_cap = 0;
        return false;
    }
    q->m_n = 0;
    q->m_cap = cap;
    return true;
}

static void sq_free (SectQ* q) {
    delete[] q->m_a;
    q->m_a = nullptr;
    q->m_n = 0;
    q->m_cap = 0;
}

static bool sq_grow (SectQ* q) {
    const u32 nc = q->m_cap < 64u ? 64u : q->m_cap * 2u;
    QItem* na = new QItem[nc];
    if (na == nullptr) {
        return false;
    }
    for (u32 i = 0; i < q->m_n; ++i) {
        na[i] = q->m_a[i];
    }
    delete[] q->m_a;
    q->m_a = na;
    q->m_cap = nc;
    return true;
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
    if (q->m_n >= q->m_cap && !sq_grow(q)) {
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

static bool sb_init (SectBuild* s) {
    s->m_conn_n = 0;
    s->m_conn_cap = 8;
    s->m_conn = new u16[s->m_conn_cap];
    return s->m_conn != nullptr;
}

static void sb_free (SectBuild* s) {
    delete[] s->m_conn;
    s->m_conn = nullptr;
    s->m_conn_n = 0;
    s->m_conn_cap = 0;
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
        if (a->m_conn_n >= a->m_conn_cap) {
            const u16 nc = static_cast<u16>(a->m_conn_cap * 2u);
            u16* na = new u16[nc];
            if (na == nullptr) {
                return false;
            }
            for (u16 i = 0; i < a->m_conn_n; ++i) {
                na[i] = a->m_conn[i];
            }
            delete[] a->m_conn;
            a->m_conn = na;
            a->m_conn_cap = nc;
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
        if (b->m_conn_n >= b->m_conn_cap) {
            const u16 nc = static_cast<u16>(b->m_conn_cap * 2u);
            u16* na = new u16[nc];
            if (na == nullptr) {
                return false;
            }
            for (u16 i = 0; i < b->m_conn_n; ++i) {
                na[i] = b->m_conn[i];
            }
            delete[] b->m_conn;
            b->m_conn = na;
            b->m_conn_cap = nc;
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
    if (terrain == nullptr || out == nullptr || pts.m_n == 0 || pts.m_pts == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* glob = build_glob_ocn_mask(terrain, w, h);
    if (glob == nullptr) {
        return false;
    }
    const u32 sector_n = pts.m_n;
    SectBuild* secs = new SectBuild[sector_n];
    SectQ* qs = new SectQ[sector_n];
    if (secs == nullptr || qs == nullptr) {
        delete[] glob;
        delete[] secs;
        delete[] qs;
        return false;
    }
    for (u32 si = 0; si < sector_n; ++si) {
        if (!sb_init(&secs[si]) || !sq_init(&qs[si], 64u)) {
            for (u32 j = 0; j <= si; ++j) {
                sb_free(&secs[j]);
                sq_free(&qs[j]);
            }
            delete[] glob;
            delete[] secs;
            delete[] qs;
            return false;
        }
        secs[si].m_cx = pts.m_pts[si].m_x;
        secs[si].m_cy = pts.m_pts[si].m_y;
    }
    u16* clm = Whiteboard::alloc(static_cast<i32>(n));
    u16* overlay = new u16[n];
    if (clm == nullptr || overlay == nullptr) {
        Whiteboard::release(clm);
        delete[] overlay;
        for (u32 si = 0; si < sector_n; ++si) {
            sb_free(&secs[si]);
            sq_free(&qs[si]);
        }
        delete[] glob;
        delete[] secs;
        delete[] qs;
        return false;
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
                    sb_add_conn(&secs[si], &secs[oid], static_cast<u16>(si), oid);
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
            sq_push_sorted(&qs[si], e);
        }
    }
    i32* act = new i32[sector_n];
    if (act == nullptr) {
        Whiteboard::release(clm);
        delete[] overlay;
        for (u32 si = 0; si < sector_n; ++si) {
            sb_free(&secs[si]);
            sq_free(&qs[si]);
        }
        delete[] glob;
        delete[] secs;
        delete[] qs;
        return false;
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
                            sb_add_conn(&secs[static_cast<u32>(si)], &secs[oid], static_cast<u16>(si), oid);
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
                    sq_push_sorted(&qs[static_cast<u32>(si)], ne);
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
    delete[] act;
    Whiteboard::release(clm);
    delete[] glob;
    P1_RiverSectorNode* nodes = new P1_RiverSectorNode[sector_n];
    if (nodes == nullptr) {
        delete[] overlay;
        for (u32 si = 0; si < sector_n; ++si) {
            sb_free(&secs[si]);
            sq_free(&qs[si]);
        }
        delete[] secs;
        delete[] qs;
        return false;
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
                for (u32 sj = 0; sj < sector_n; ++sj) {
                    sb_free(&secs[sj]);
                    sq_free(&qs[sj]);
                }
                delete[] secs;
                delete[] qs;
                return false;
            }
            for (u16 c = 0; c < nodes[si].m_conn_n; ++c) {
                nodes[si].m_conn[c] = secs[si].m_conn[c];
            }
        }
        sb_free(&secs[si]);
        sq_free(&qs[si]);
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

static bool color_used (const u8* clr, u32 sector_n, u8 r, u8 g, u8 b) {
    for (u32 si = 0; si < sector_n; ++si) {
        if (clr[si * 3u + 0] == r && clr[si * 3u + 1] == g && clr[si * 3u + 2] == b) {
            return true;
        }
    }
    return false;
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
    if (terrain == nullptr || !p1_map_size_ok(w, h) || pts.m_n == 0) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
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

void P1_Gen_RiverSectors::save_output (cstr path, const u8* terrain) const {
    if (!m_valid_generation || path == nullptr || terrain == nullptr || m_rslt.m_ov == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 sector_n = static_cast<u32>(m_rslt.m_sector_n);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        const u8 cls = terrain[i];
        if (cls == TERR_OCEAN[0]) {
            r = TERR_OCEAN[1]; g = TERR_OCEAN[2]; b = TERR_OCEAN[3];
        } else if (cls == TERR_SEA[0]) {
            r = TERR_SEA[1]; g = TERR_SEA[2]; b = TERR_SEA[3];
        } else if (cls == TERR_COASTAL[0]) {
            r = TERR_COASTAL[1]; g = TERR_COASTAL[2]; b = TERR_COASTAL[3];
        } else if (cls == TERR_PLAINS[0]) {
            r = TERR_PLAINS[1]; g = TERR_PLAINS[2]; b = TERR_PLAINS[3];
        } else if (cls == TERR_HILLS[0]) {
            r = TERR_HILLS[1]; g = TERR_HILLS[2]; b = TERR_HILLS[3];
        } else if (cls == TERR_MOUNTAINS[0]) {
            r = TERR_MOUNTAINS[1]; g = TERR_MOUNTAINS[2]; b = TERR_MOUNTAINS[3];
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    u8* clr = new u8[sector_n * 3u];
    if (clr == nullptr) {
        delete[] rgb;
        return;
    }
    Rng32 rng;
    rng_seed(&rng, m_prm.m_seed);
    for (u32 si = 0; si < sector_n; ++si) {
        u8 cr = 0;
        u8 cg = 0;
        u8 cb = 0;
        for (i32 tries = 0; tries < 256; ++tries) {
            cr = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            cg = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            cb = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            if (!color_used(clr, si, cr, cg, cb)) {
                break;
            }
        }
        clr[si * 3u + 0] = cr;
        clr[si * 3u + 1] = cg;
        clr[si * 3u + 2] = cb;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = m_rslt.m_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
            continue;
        }
        const u32 p = i * 3u;
        const u32 c = static_cast<u32>(sid) * 3u;
        rgb[p + 0] = clr[c + 0];
        rgb[p + 1] = clr[c + 1];
        rgb[p + 2] = clr[c + 2];
    }
    delete[] clr;
    save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
