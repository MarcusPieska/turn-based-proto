//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_sect_adj.h"

#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "whiteboard_mng.h"

#include <cstring>

#define SA_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverSectAdj", msg)

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_adj_all_max = 64u;

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

static void nb_add (u16* nb, u8* nb_n, u16 si, u16 nid, u16 stride, u16 cap) {
    if (nb_has(nb, nb_n, si, nid, stride)) {
        return;
    }
    const u32 n = static_cast<u32>(nb_n[si]);
    if (n >= static_cast<u32>(cap)) {
        return;
    }
    const u32 base = static_cast<u32>(si) * static_cast<u32>(stride);
    nb[base + n] = nid;
    nb_n[si] = static_cast<u8>(n + 1u);
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
    const u32 all_slab_n = static_cast<u32>(sector_n) * static_cast<u32>(k_adj_all_max);
    const u32 out_slab_n = static_cast<u32>(sector_n) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
    if (all_slab_n > WhiteboardMng::tile_n() || out_slab_n > WhiteboardMng::tile_n() || static_cast<u32>(sector_n) > WhiteboardMng::tile_n()) {
        SA_ABORT("sector adj slab exceeds whiteboard capacity");
    }
    Whiteboard_1B wb_all_n("P1_Gen_RiverSectAdj", "all_n", 0u);
    Whiteboard_2B wb_all("P1_Gen_RiverSectAdj", "all", 0u);
    P1_WB_CHK(wb_all_n);
    P1_WB_CHK(wb_all);
    u8* all_n = wb_all_n.raw();
    u16* all_nb = wb_all.get_iter_ptr();
    if (all_n == nullptr || all_nb == nullptr) {
        return false;
    }
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        all_n[si] = 0;
        nb_n[si] = 0;
    }
    for (u32 i = 0; i < all_slab_n; ++i) {
        all_nb[i] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
    }
    for (u32 i = 0; i < out_slab_n; ++i) {
        nb[i] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 py = 0; py < hi; ++py) {
        for (u32 px = 0; px < wi; ++px) {
            const u32 ti = py * wi + px;
            const u16 sid = sec_ov[ti];
            if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
                continue;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                    continue;
                }
                const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
                const u16 nid = sec_ov[ni];
                if (nid == sid || nid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || nid >= sector_n) {
                    continue;
                }
                nb_add(all_nb, all_n, sid, nid, k_adj_all_max, k_adj_all_max);
                nb_add(all_nb, all_n, nid, sid, k_adj_all_max, k_adj_all_max);
            }
        }
    }
    for (u16 si = 0; si < sector_n; ++si) {
        const u16 nn = static_cast<u16>(all_n[si]);
        if (nn <= static_cast<u16>(P1_RIVER_SECT_ADJ_MAX)) {
            const u32 b0 = static_cast<u32>(si) * static_cast<u32>(k_adj_all_max);
            const u32 b1 = static_cast<u32>(si) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
            for (u16 k = 0; k < nn; ++k) {
                nb[b1 + static_cast<u32>(k)] = all_nb[b0 + static_cast<u32>(k)];
            }
            nb_n[si] = static_cast<u8>(nn);
        } else {
            const u32 b0 = static_cast<u32>(si) * static_cast<u32>(k_adj_all_max);
            const u32 b1 = static_cast<u32>(si) * static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
            for (u16 tk = 0; tk < static_cast<u16>(P1_RIVER_SECT_ADJ_MAX); ++tk) {
                const u32 idx = (static_cast<u32>(tk) * static_cast<u32>(nn)) / static_cast<u32>(P1_RIVER_SECT_ADJ_MAX);
                nb[b1 + static_cast<u32>(tk)] = all_nb[b0 + idx];
            }
            nb_n[si] = static_cast<u8>(P1_RIVER_SECT_ADJ_MAX);
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
