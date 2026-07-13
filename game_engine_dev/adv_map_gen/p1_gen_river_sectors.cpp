//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "p1_gen_river_sectors.h"
#include "generator_constants.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

#define RS_ABORT(msg) P1_STEP_ABORT("P1_Gen_RiverSectors", msg)

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u8 k_dist_inf = 255u;
static const u8 k_dist_max = 254u;

static bool is_mountain (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool tile_blocked (const u8* terrain, const u16* ocn, u16 glob_ocn, u32 ti) {
    if (is_mountain(terrain[ti])) {
        return true;
    }
    if (ocn != nullptr && ocn[ti] != static_cast<u16>(P1_OCEAN_IDX_NONE)) {
        return true;
    }
    return false;
}

static u32 tidx (u16 w, u32 x, u32 y) {
    return y * static_cast<u32>(w) + x;
}

static u32 isqrt_u32 (u32 v) {
    u32 r = 0;
    u32 bit = 1u << 30;
    while (bit > v) {
        bit >>= 2;
    }
    while (bit != 0u) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

static void lay_off_dist_u8 (u8* out) {
    const u32 span = static_cast<u32>(P1_RIVER_SECTOR_OFF_SPAN);
    for (u32 oy = 0; oy < span; ++oy) {
        for (u32 ox = 0; ox < span; ++ox) {
            const i32 dx = static_cast<i32>(ox) - P1_RIVER_SECTOR_OFF_RAD;
            const i32 dy = static_cast<i32>(oy) - P1_RIVER_SECTOR_OFF_RAD;
            const u32 dsq = static_cast<u32>(dx * dx + dy * dy);
            out[oy * span + ox] = static_cast<u8>(isqrt_u32(dsq));
        }
    }
}

static bool bfs_enq (WB_QueXY& bfs, u8* inq, u32 ni, u16 x, u16 y) {
    if (inq[ni] != 0u) {
        return true;
    }
    if (!bfs.push(x, y)) {
        return false;
    }
    inq[ni] = 1u;
    return true;
}

static void lay_ocn_secs (
    const u16* ocn,
    u16 ocean_n,
    u16 glob_ocn,
    u16 w,
    u16 h,
    u16 ocn_sec_n,
    u16* ov) {
    if (ocn == nullptr || ov == nullptr || ocn_sec_n == 0u || ocean_n == 0u) {
        return;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    u16 si = 0u;
    for (u16 ocid = 1u; ocid <= ocean_n; ++ocid) {
        if (ocid == glob_ocn) {
            continue;
        }
        for (u32 py = 0; py < hi; ++py) {
            for (u32 px = 0; px < wi; ++px) {
                const u32 ti = py * wi + px;
                if (ocn[ti] == ocid) {
                    ov[ti] = si;
                }
            }
        }
        si++;
        if (si >= ocn_sec_n) {
            break;
        }
    }
}

static bool claim_sector_overlay (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    const u16* ocn,
    u16 ocean_n,
    u16 glob_ocn,
    Whiteboard_2B* wb_ov,
    Whiteboard_1B* wb_dist,
    u8* dist_out,
    P1_Gen_RiverSectorsRslt* out) {
    if (terrain == nullptr || out == nullptr || ocn == nullptr || wb_ov == nullptr || wb_dist == nullptr || dist_out == nullptr
        || !wb_ov->ok() || !wb_dist->ok() || pts.m_n == 0 || !pts.m_que.ok()) {
        RS_ABORT("claim_sector_overlay null args or empty pts");
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 sector_n = pts.m_n;
    const u16 ocn_sec_n = pts.m_ocn_sec_n;
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    Whiteboard_1B wb_inq("P1_Gen_RiverSectors", "inq", 0u);
    P1_WB_CHK(wb_inq);
    u16* ov = wb_ov->get_iter_ptr();
    u8* dist = wb_dist->raw();
    u8* inq = wb_inq.raw();
    WB_QueXY bfs;
    if (!bfs.ok()) {
        RS_ABORT("claim_sector_overlay bfs queue failed");
    }
    std::memset(dist, k_dist_inf, n);
    std::memset(inq, 0, n);
    for (u32 i = 0; i < n; ++i) {
        ov[i] = static_cast<u16>(P1_RIVER_SECTOR_NONE);
    }
    lay_ocn_secs(ocn, ocean_n, glob_ocn, w, h, ocn_sec_n, ov);
    for (u32 si = static_cast<u32>(ocn_sec_n); si < sector_n; ++si) {
        const u16 cx = pts.m_que.x_at(si);
        const u16 cy = pts.m_que.y_at(si);
        if (cx >= w || cy >= h) {
            continue;
        }
        const u32 ti = static_cast<u32>(cy) * wi + static_cast<u32>(cx);
        if (tile_blocked(terrain, ocn, glob_ocn, ti)) {
            continue;
        }
        ov[ti] = static_cast<u16>(si);
        dist[ti] = 0;
        if (!bfs_enq(bfs, inq, ti, cx, cy)) {
            RS_ABORT("claim_sector_overlay seed queue overflow");
        }
    }
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    u32 qi = 0;
    while (qi < bfs.count()) {
        const u16 px = bfs.x_at(qi);
        const u16 py = bfs.y_at(qi);
        qi++;
        const u32 ti = static_cast<u32>(py) * wi + static_cast<u32>(px);
        inq[ti] = 0u;
        const u16 si = ov[ti];
        const u8 d = dist[ti];
        if (si == static_cast<u16>(P1_RIVER_SECTOR_NONE) || d == k_dist_inf || d >= k_dist_max) {
            continue;
        }
        const u8 nd = static_cast<u8>(d + 1u);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + dx4[k];
            const i32 ny = static_cast<i32>(py) + dy4[k];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (tile_blocked(terrain, ocn, glob_ocn, ni)) {
                continue;
            }
            const u16 cur = ov[ni];
            const u8 cd = dist[ni];
            if (cur != static_cast<u16>(P1_RIVER_SECTOR_NONE) && cd <= nd) {
                continue;
            }
            ov[ni] = si;
            dist[ni] = nd;
            if (!bfs_enq(bfs, inq, ni, static_cast<u16>(nx), static_cast<u16>(ny))) {
                RS_ABORT("claim_sector_overlay bfs queue overflow");
            }
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_ocn_sec_n = ocn_sec_n;
    out->m_sector_n = static_cast<u16>(sector_n);
    out->m_ov = ov;
    lay_off_dist_u8(dist_out);
    out->m_dist_ov = dist_out;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverSectors -
//================================================================================================================================

P1_Gen_RiverSectors::P1_Gen_RiverSectors (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt(),
    m_wb_ov(nullptr),
    m_wb_dist(nullptr) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_ocn_sec_n = 0;
    m_rslt.m_sector_n = 0;
    m_rslt.m_ov = nullptr;
    m_rslt.m_dist_ov = nullptr;
    m_wb_ov = new Whiteboard_2B("P1_Gen_RiverSectors", "ov", prm.m_seed);
    m_wb_dist = new Whiteboard_1B("P1_Gen_RiverSectors", "dist", prm.m_seed);
    P1_WB_CHK(*m_wb_ov);
    P1_WB_CHK(*m_wb_dist);
}

P1_Gen_RiverSectors::~P1_Gen_RiverSectors () {
    clear_rslt();
    delete m_wb_dist;
    delete m_wb_ov;
    m_wb_dist = nullptr;
    m_wb_ov = nullptr;
}

void P1_Gen_RiverSectors::clear_rslt () {
    m_rslt.m_ov = nullptr;
    m_rslt.m_dist_ov = nullptr;
    m_rslt.m_sector_n = 0;
    m_rslt.m_ocn_sec_n = 0;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverSectors::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    const P1_OceanIndexRef& ocean) {
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
    if (!p1_ocean_ref_ok(ocean) || ocean.m_w != w || ocean.m_h != h) {
        RS_ABORT("invalid ocean index input");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        RS_ABORT("size mismatch");
    }
    if (!claim_sector_overlay(terrain, w, h, pts, ocean.m_ov, ocean.m_ocean_n, ocean.m_largest_idx, m_wb_ov, m_wb_dist, m_dist_u8, &m_rslt)) {
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
