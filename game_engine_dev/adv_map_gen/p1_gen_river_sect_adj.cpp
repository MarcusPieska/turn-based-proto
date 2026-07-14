//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_sect_adj.h"

#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

#include <cstring>

#define SA_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverSectAdj", msg)

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_adj_all_max = 64u;
static const u8 k_nb_unproc = 255u;

static bool sect_proc (u8 nb_n_v) {
    return nb_n_v != k_nb_unproc;
}

static bool nb_has (const u16* nb, u8 n, u16 nid) {
    for (u32 i = 0; i < static_cast<u32>(n); ++i) {
        if (nb[i] == nid) {
            return true;
        }
    }
    return false;
}

static bool nb_has (const u16* nb, const u8* nb_n, u16 si, u16 nid, u16 stride) {
    const u32 n = static_cast<u32>(nb_n[si]);
    const u32 base = static_cast<u32>(si) * static_cast<u32>(stride);
    for (u32 i = 0; i < n; ++i) {
        if (nb[base + i] == nid) {
            return true;
        }
    }
    return false;
}

static bool full_add (u16* full, u8* fn, u16 nid) {
    if (nb_has(full, *fn, nid)) {
        return true;
    }
    if (*fn >= static_cast<u8>(k_adj_all_max)) {
        return false;
    }
    full[*fn] = nid;
    (*fn)++;
    return true;
}

static u8 subsample_row (const u16* full, u8 nn, u16* row) {
    for (u8 k = 0; k < static_cast<u8>(P1_RIVER_SECT_ADJ_MAX); ++k) {
        row[k] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
    }
    if (nn == 0u) {
        return 0u;
    }
    if (nn <= static_cast<u8>(P1_RIVER_SECT_ADJ_MAX)) {
        for (u8 k = 0; k < nn; ++k) {
            row[k] = full[k];
        }
        return nn;
    }
    for (u8 tk = 0; tk < static_cast<u8>(P1_RIVER_SECT_ADJ_MAX); ++tk) {
        const u32 idx = (static_cast<u32>(tk) * static_cast<u32>(nn)) / static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
        row[tk] = full[idx];
    }
    return static_cast<u8>(P1_RIVER_SECT_ADJ_MAX);
}

static bool nb_row_try_add (u16* nb, u8* nb_n, u16 si, u16 nid) {
    if (!sect_proc(nb_n[si])) {
        return false;
    }
    const u32 base = static_cast<u32>(si) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
    const u8 n = nb_n[si];
    if (nb_has(nb + base, n, nid)) {
        return true;
    }
    if (n >= static_cast<u8>(P1_RIVER_SECT_ADJ_MAX)) {
        return false;
    }
    nb[base + static_cast<u32>(n)] = nid;
    nb_n[si] = static_cast<u8>(n + 1u);
    return true;
}

static void sync_proc_nb (u16* nb, u8* nb_n, u16 si) {
    const u32 bi = static_cast<u32>(si) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
    const u8 ni = nb_n[si];
    for (u8 k = 0; k < ni; ++k) {
        const u16 nid = nb[bi + static_cast<u32>(k)];
        if (nid == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
            continue;
        }
        if (sect_proc(nb_n[nid])) {
            nb_row_try_add(nb, nb_n, nid, si);
        }
    }
}

static bool flood_sect (
    u16 w,
    u16 h,
    const u16* sec_ov,
    u16 sector_n,
    u16 si,
    u16 sx,
    u16 sy,
    u8* inq,
    WB_QueXY& bfs,
    u16* full,
    u8* fn) {
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    bfs.clear();
    *fn = 0;
    const u32 sti = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    if (sec_ov[sti] != si) {
        return false;
    }
    if (inq[sti] != 0u) {
        return false;
    }
    if (!bfs.push(sx, sy)) {
        return false;
    }
    inq[sti] = 1u;
    u32 qi = 0u;
    while (qi < bfs.count()) {
        const u16 px = bfs.x_at(qi);
        const u16 py = bfs.y_at(qi);
        qi++;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            const u16 ov = sec_ov[ni];
            if (ov == static_cast<u16>(P1_RIVER_SECTOR_NONE) || ov >= sector_n) {
                continue;
            }
            if (ov != si) {
                full_add(full, fn, ov);
                continue;
            }
            if (inq[ni] != 0u) {
                continue;
            }
            inq[ni] = 1u;
            if (!bfs.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                return false;
            }
        }
    }
    return true;
}

static bool build_sect_adj (
    u16 w,
    u16 h,
    const u16* sec_ov,
    u16 sector_n,
    u8* nb_n,
    u16* nb) {
    if (sec_ov == nullptr || nb_n == nullptr || nb == nullptr || sector_n == 0) {
        return false;
    }
    const u32 out_slab_n = static_cast<u32>(sector_n) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
    if (out_slab_n > WhiteboardMng::tile_n() || static_cast<u32>(sector_n) > WhiteboardMng::tile_n()) {
        SA_ABORT("sector adj out slab exceeds whiteboard capacity");
    }
    Whiteboard_1B wb_inq("P1_Gen_RiverSectAdj", "inq", 0u);
    P1_WB_CHK(wb_inq);
    u8* inq = wb_inq.raw();
    if (inq == nullptr) {
        return false;
    }
    WB_QueXY bfs;
    if (!bfs.ok()) {
        SA_ABORT("sector adj bfs queue failed");
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(inq, 0, npx);
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        nb_n[si] = k_nb_unproc;
    }
    for (u32 i = 0; i < out_slab_n; ++i) {
        nb[i] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    u16 full[k_adj_all_max];
    for (u32 py = 0; py < hi; ++py) {
        for (u32 px = 0; px < wi; ++px) {
            const u32 ti = py * wi + px;
            const u16 sid = sec_ov[ti];
            if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
                continue;
            }
            if (sect_proc(nb_n[sid])) {
                continue;
            }
            u8 fn = 0u;
            if (!flood_sect(w, h, sec_ov, sector_n, sid, static_cast<u16>(px), static_cast<u16>(py), inq, bfs, full, &fn)) {
                return false;
            }
            const u32 base = static_cast<u32>(sid) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
            const u8 out_n = subsample_row(full, fn, nb + base);
            nb_n[sid] = out_n;
            sync_proc_nb(nb, nb_n, sid);
        }
    }
    for (u16 si = 0; si < sector_n; ++si) {
        u8 out_n = nb_n[si];
        const u32 base = static_cast<u32>(si) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
        u32 wr = 0;
        for (u32 k = 0; k < static_cast<u32>(out_n); ++k) {
            const u16 nid = nb[base + k];
            if (nid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || nid >= sector_n) {
                continue;
            }
            if (!nb_has(nb, nb_n, nid, si, static_cast<u16>(P1_RIVER_SECT_ADJ_MAX))) {
                continue;
            }
            nb[base + wr] = nid;
            wr++;
        }
        for (u32 k = wr; k < static_cast<u32>(P1_RIVER_SECT_ADJ_MAX); ++k) {
            nb[base + k] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
        }
        nb_n[si] = static_cast<u8>(wr);
    }
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverSectAdj -
//================================================================================================================================

P1_Gen_RiverSectAdj::P1_Gen_RiverSectAdj (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt(),
    m_wb_nb_n(nullptr),
    m_wb_nb(nullptr) {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
    m_wb_nb_n = new Whiteboard_1B("P1_Gen_RiverSectAdj", "nb_n", prm.m_seed);
    m_wb_nb = new Whiteboard_2B("P1_Gen_RiverSectAdj", "nb", prm.m_seed);
    P1_WB_CHK(*m_wb_nb_n);
    P1_WB_CHK(*m_wb_nb);
}

P1_Gen_RiverSectAdj::~P1_Gen_RiverSectAdj () {
    clear_rslt();
    delete m_wb_nb;
    delete m_wb_nb_n;
    m_wb_nb = nullptr;
    m_wb_nb_n = nullptr;
}

void P1_Gen_RiverSectAdj::clear_rslt () { 
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sector_n = 0;
    m_rslt.m_nb_n = nullptr;
    m_rslt.m_nb = nullptr;
    m_valid_generation = false;
}

bool P1_Gen_RiverSectAdj::generate (const P1_Gen_RiverSectorsRslt& sectors) {
    m_valid_generation = false;
    clear_rslt();
    if (sectors.m_ov == nullptr || sectors.m_sector_n == 0 || !p1_map_size_ok(sectors.m_w, sectors.m_h)) {
        return false;
    }
    if (sectors.m_w != m_prm.m_w || sectors.m_h != m_prm.m_h) {
        return false;
    }
    if (m_wb_nb_n == nullptr || m_wb_nb == nullptr || !m_wb_nb_n->ok() || !m_wb_nb->ok()) {
        SA_ABORT("whiteboard checkout failed");
    }
    u8* nb_n = m_wb_nb_n->raw();
    u16* nb = m_wb_nb->get_iter_ptr();
    if (nb_n == nullptr || nb == nullptr) {
        return false;
    }
    if (!build_sect_adj(sectors.m_w, sectors.m_h, sectors.m_ov, sectors.m_sector_n, nb_n, nb)) {
        return false;
    }
    m_rslt.m_w = sectors.m_w;
    m_rslt.m_h = sectors.m_h;
    m_rslt.m_sector_n = sectors.m_sector_n;
    m_rslt.m_nb_n = nb_n;
    m_rslt.m_nb = nb;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverSectAdj::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverSectAdjRslt& P1_Gen_RiverSectAdj::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
