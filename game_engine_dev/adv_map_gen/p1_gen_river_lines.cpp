//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <ctime>

#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_wb_util.h"
#include "generator_constants.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Paint helpers -
//================================================================================================================================

static const u8 k_dist_inf = 0xFFu;
static const u8 k_dov_none = 0xFFu;
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dx8[8] = {-1, 1, 0, 0, -1, 1, -1, 1};
static const i32 k_dy8[8] = {0, 0, -1, 1, -1, -1, 1, 1};

struct RivLineProf {
    double m_flood_sec;
    double m_walk_sec;
    u32 m_flood_n;
    u32 m_walk_n;
    u32 m_flood_tiles;
};

struct MouthProf {
    u32 m_n;
    u32 m_placed;
    u32 m_moved;
};

static u32 tidx (u16 w, i32 x, i32 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_open_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0];
}

static bool is_sec_land (const u8* terrain, const u16* ocn, u16 glob_ocn, u32 ti) {
    if (terrain[ti] == TERR_MOUNTAINS[0] || is_open_wat(terrain[ti])) {
        return false;
    }
    if (glob_ocn != static_cast<u16>(P1_OCEAN_IDX_NONE) && ocn != nullptr && ocn[ti] == glob_ocn) {
        return false;
    }
    return true;
}

static bool sec_land_adj_glob (const u8* terrain, const u16* ocn, u16 glob_ocn, u16 w, u16 h, i32 x, i32 y) {
    if (glob_ocn == static_cast<u16>(P1_OCEAN_IDX_NONE)) {
        return false;
    }
    const u32 ti = tidx(w, x, y);
    if (!is_sec_land(terrain, ocn, glob_ocn, ti)) {
        return false;
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = x + k_dx4[k];
        const i32 ny = y + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ni = tidx(w, nx, ny);
        if (ocn != nullptr && ocn[ni] == glob_ocn) {
            return true;
        }
    }
    return false;
}

static double clk_sec (clock_t t0, clock_t t1) {
    return static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
}

static bool sec_cxy (const P1_Gen_RiverPtsRslt& pts, u16 sid, u16* cx, u16* cy) {
    if (sid >= pts.m_n || !pts.m_que.ok()) {
        return false;
    }
    *cx = pts.m_que.x_at(sid);
    *cy = pts.m_que.y_at(sid);
    return true;
}

static bool sec_tile_ok (const u16* sec_ov, u32 ti, u16 sa, u16 sb) {
    const u16 s = sec_ov[ti];
    return s == sa || s == sb;
}

static bool tile_use8 (i32 x, i32 y) {
    return ((x + y) & 1) == 1;
}

static bool dist_flood_sec (
    u8* dist,
    u32* gen,
    u32 tag,
    u32* vis_ix,
    u32* vis_n,
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 sa,
    u16 sb) 
{
    *vis_n = 0u;
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u32 si = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    if (!sec_tile_ok(sec_ov, si, sa, sb)) {
        std::printf("P1_Gen_RiverLines flood: seed sector mismatch at (%u,%u) sectors %u->%u\n", static_cast<u32>(sx), static_cast<u32>(sy), static_cast<u32>(sa), static_cast<u32>(sb));
        return false;
    }
    WB_QueXY q;
    if (!q.ok() || !q.push(sx, sy)) {
        std::printf("P1_Gen_RiverLines flood: queue init failed sectors %u->%u\n", static_cast<u32>(sa), static_cast<u32>(sb));
        return false;
    }
    gen[si] = tag;
    dist[si] = 0u;
    vis_ix[(*vis_n)++] = si;
    while (q.count() > 0u) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1u);
        const u32 ti = static_cast<u32>(py) * wi + static_cast<u32>(px);
        const u8 cd = dist[ti];
        if (cd >= k_dist_inf - 1u) {
            continue;
        }
        const u8 nd = static_cast<u8>(cd + 1u);
        const bool use8 = tile_use8(static_cast<i32>(px), static_cast<i32>(py));
        const i32* dx = use8 ? k_dx8 : k_dx4;
        const i32* dy = use8 ? k_dy8 : k_dy4;
        const i32 nn = use8 ? 8 : 4;
        for (i32 k = 0; k < nn; ++k) {
            const i32 nx = static_cast<i32>(px) + dx[k];
            const i32 ny = static_cast<i32>(py) + dy[k];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (gen[ni] == tag || !sec_tile_ok(sec_ov, ni, sa, sb)) {
                continue;
            }
            gen[ni] = tag;
            dist[ni] = nd;
            vis_ix[(*vis_n)++] = ni;
            if (!q.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                std::printf("P1_Gen_RiverLines flood: queue push failed sectors %u->%u\n", static_cast<u32>(sa), static_cast<u32>(sb));
                return false;
            }
        }
    }
    return true;
}

static void merge_link_dist (u8* dov, const u8* dist, const u32* vis_ix, u32 vis_n) {
    for (u32 k = 0; k < vis_n; ++k) {
        const u32 i = vis_ix[k];
        const u8 d = dist[i];
        if (dov[i] == k_dov_none || d < dov[i]) {
            dov[i] = d;
        }
    }
}

static void paint_walk_down (u8* ov, const u8* dist, const u32* gen, u32 tag, u16 w, u16 h, i32 x, i32 y) {
    while (in_map(w, h, x, y)) {
        const u32 i = tidx(w, x, y);
        if (gen[i] != tag) {
            break;
        }
        ov[i] = 1u;
        const u8 cd = dist[i];
        if (cd == 0u) {
            break;
        }
        u8 best = cd;
        i32 bx = x;
        i32 by = y;
        bool found = false;
        const bool use8 = tile_use8(x, y);
        const i32* dx = use8 ? k_dx8 : k_dx4;
        const i32* dy = use8 ? k_dy8 : k_dy4;
        const i32 nn = use8 ? 8 : 4;
        for (i32 k = 0; k < nn; ++k) {
            const i32 nx = x + dx[k];
            const i32 ny = y + dy[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (gen[ni] != tag) {
                continue;
            }
            const u8 nd = dist[ni];
            if (nd < best) {
                best = nd;
                bx = nx;
                by = ny;
                found = true;
            }
        }
        if (!found) {
            break;
        }
        if (bx != x && by != y) {
            i32 cx1 = bx;
            i32 cy1 = y;
            i32 cx2 = x;
            i32 cy2 = by;
            if ((y % 3) == 0) {
                cx1 = x;
                cy1 = by;
                cx2 = bx;
                cy2 = y;
            }
            bool crn = false;
            if (in_map(w, h, cx1, cy1) && gen[tidx(w, cx1, cy1)] == tag) {
                ov[tidx(w, cx1, cy1)] = 1u;
                crn = true;
            } else if (in_map(w, h, cx2, cy2) && gen[tidx(w, cx2, cy2)] == tag) {
                ov[tidx(w, cx2, cy2)] = 1u;
                crn = true;
            }
            if (!crn) {
                best = cd;
                bx = x;
                by = y;
                found = false;
                for (i32 k = 0; k < 4; ++k) {
                    const i32 nx = x + k_dx4[k];
                    const i32 ny = y + k_dy4[k];
                    if (!in_map(w, h, nx, ny)) {
                        continue;
                    }
                    const u32 ni = tidx(w, nx, ny);
                    if (gen[ni] != tag) {
                        continue;
                    }
                    const u8 nd = dist[ni];
                    if (nd < best) {
                        best = nd;
                        bx = nx;
                        by = ny;
                        found = true;
                    }
                }
                if (!found) {
                    break;
                }
            }
        }
        x = bx;
        y = by;
    }
}

static bool paint_seg (
    u8* ov,
    u8* dov,
    u8* dist,
    u32* gen,
    u32* vis_ix,
    u32* tag,
    const u8* terrain,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 fx,
    u16 fy,
    u16 tx,
    u16 ty,
    u16 sa,
    u16 sb,
    RivLineProf* prof) 
{
    const u32 cur = (*tag)++;
    u32 vis_n = 0u;
    clock_t t0 = clock();
    if (!dist_flood_sec(dist, gen, cur, vis_ix, &vis_n, terrain, sec_ov, w, h, tx, ty, sa, sb)) {
        std::printf(
            "P1_Gen_RiverLines paint: flood failed %u->%u from (%u,%u) to (%u,%u)\n",
            static_cast<u32>(sa),
            static_cast<u32>(sb),
            static_cast<u32>(fx),
            static_cast<u32>(fy),
            static_cast<u32>(tx),
            static_cast<u32>(ty));
        return false;
    }
    merge_link_dist(dov, dist, vis_ix, vis_n);
    prof->m_flood_sec += clk_sec(t0, clock());
    prof->m_flood_n++;
    prof->m_flood_tiles += vis_n;
    if (gen[tidx(w, fx, fy)] != cur) {
        std::printf(
            "P1_Gen_RiverLines paint: upstream unreachable %u->%u from (%u,%u) to (%u,%u)\n",
            static_cast<u32>(sa),
            static_cast<u32>(sb),
            static_cast<u32>(fx),
            static_cast<u32>(fy),
            static_cast<u32>(tx),
            static_cast<u32>(ty));
        return false;
    }
    t0 = clock();
    paint_walk_down(ov, dist, gen, cur, w, h, static_cast<i32>(fx), static_cast<i32>(fy));
    prof->m_walk_sec += clk_sec(t0, clock());
    prof->m_walk_n++;
    return true;
}

static bool paint_riv_lines (
    u8* ov,
    u8* dov,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network,
    const u8* terrain,
    RivLineProf* prof) 
{
    Whiteboard_1B wb_dist("P1_Gen_RiverLines", "link_dist", 0u);
    Whiteboard_4B wb_gen("P1_Gen_RiverLines", "flood_gen", 0u);
    Whiteboard_4B wb_vis_ix("P1_Gen_RiverLines", "vis_ix", 0u);
    P1_WB_CHK(wb_dist);
    P1_WB_CHK(wb_gen);
    P1_WB_CHK(wb_vis_ix);
    u8* dist = wb_dist.raw();
    u32* gen = wb_gen.get_iter_ptr();
    u32* vis_ix = wb_vis_ix.get_iter_ptr();
    u32 tag = 1u;
    const u16* sec_ov = sectors.m_ov;
    const u16 sector_n = network.m_sector_n;
    for (u16 si = 0; si < sector_n; ++si) {
        const u16 down = network.m_downstream[si];
        if (down == static_cast<u16>(P1_RIVER_DOWN_UNDEF) || down == static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            continue;
        }
        if (down >= sector_n) {
            std::printf("P1_Gen_RiverLines paint: link %u downstream %u out of range\n", static_cast<u32>(si), static_cast<u32>(down));
            continue;
        }
        u16 ax = 0;
        u16 ay = 0;
        u16 bx = 0;
        u16 by = 0;
        if (!sec_cxy(pts, si, &ax, &ay)) {
            std::printf("P1_Gen_RiverLines paint: missing center for sector %u\n", static_cast<u32>(si));
            continue;
        }
        if (!sec_cxy(pts, down, &bx, &by)) {
            std::printf("P1_Gen_RiverLines paint: missing center for downstream %u\n", static_cast<u32>(down));
            continue;
        }
        if (!paint_seg(ov, dov, dist, gen, vis_ix, &tag, terrain, sec_ov, w, h, ax, ay, bx, by, si, down, prof)) {
            continue;
        }
    }
    return true;
}

static bool find_mouth_coast (
    const u8* terrain,
    const u16* ocn,
    u16 glob_ocn,
    u16 w,
    u16 h,
    const P1_Gen_RiverPtsRslt& pts,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network,
    u32* best_d,
    u16* best_x,
    u16* best_y,
    u32* coast_n) 
{
    if (glob_ocn == static_cast<u16>(P1_OCEAN_IDX_NONE)) {
        std::printf("P1_Gen_RiverLines mouth coast: glob_ocn is none\n");
        return false;
    }
    const u16* sec_ov = sectors.m_ov;
    const u16* down = network.m_downstream;
    const u16 sector_n = network.m_sector_n;
    if (sector_n > static_cast<u16>(P1_RIVER_PTS_MAX)) {
        std::printf("P1_Gen_RiverLines mouth coast: sector_n %u exceeds max\n", static_cast<u32>(sector_n));
        return false;
    }
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        best_d[si] = 0xFFFFFFFFu;
        best_x[si] = 0;
        best_y[si] = 0;
        coast_n[si] = 0u;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 py = 0; py < hi; ++py) {
        for (u32 px = 0; px < wi; ++px) {
            if (!sec_land_adj_glob(terrain, ocn, glob_ocn, w, h, static_cast<i32>(px), static_cast<i32>(py))) {
                continue;
            }
            const u32 ti = py * wi + px;
            const u16 sid = sec_ov[ti];
            if (sid >= sector_n || down[sid] != static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
                continue;
            }
            coast_n[sid]++;
            const i32 x0 = static_cast<i32>(pts.m_que.x_at(sid));
            const i32 y0 = static_cast<i32>(pts.m_que.y_at(sid));
            const i32 dx = static_cast<i32>(px) - x0;
            const i32 dy = static_cast<i32>(py) - y0;
            const u32 adx = static_cast<u32>(dx < 0 ? -dx : dx);
            const u32 ady = static_cast<u32>(dy < 0 ? -dy : dy);
            const u32 d = adx + ady;
            if (d < best_d[sid]) {
                best_d[sid] = d;
                best_x[sid] = static_cast<u16>(px);
                best_y[sid] = static_cast<u16>(py);
            }
        }
    }
    return true;
}

static bool paint_mouth_stubs (
    u8* ov,
    u8* dov,
    u16 w,
    u16 h,
    P1_Gen_RiverPtsRslt& pts,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network,
    const u8* terrain,
    const u16* ocn,
    u16 glob_ocn,
    MouthProf* prof,
    RivLineProf* line_prof) 
{
    u32 best_d[P1_RIVER_PTS_MAX];
    u16 best_x[P1_RIVER_PTS_MAX];
    u16 best_y[P1_RIVER_PTS_MAX];
    u32 coast_n[P1_RIVER_PTS_MAX];
    if (!find_mouth_coast(terrain, ocn, glob_ocn, w, h, pts, sectors, network, best_d, best_x, best_y, coast_n)) {
        return false;
    }
    Whiteboard_1B wb_dist("P1_Gen_RiverLines", "mouth_dist", 0u);
    Whiteboard_4B wb_gen("P1_Gen_RiverLines", "mouth_gen", 0u);
    Whiteboard_4B wb_vis_ix("P1_Gen_RiverLines", "mouth_vis_ix", 0u);
    P1_WB_CHK(wb_dist);
    P1_WB_CHK(wb_gen);
    P1_WB_CHK(wb_vis_ix);
    u8* dist = wb_dist.raw();
    u32* gen = wb_gen.get_iter_ptr();
    u32* vis_ix = wb_vis_ix.get_iter_ptr();
    u32 tag = 1u;
    const u16* down = network.m_downstream;
    const u16 sector_n = network.m_sector_n;
    const u16* sec_ov = sectors.m_ov;
    for (u16 si = 0; si < sector_n; ++si) {
        if (down[si] != static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            continue;
        }
        if (si >= pts.m_n) {
            std::printf("P1_Gen_RiverLines mouth stub: sector %u >= pts.m_n %u\n", static_cast<u32>(si), pts.m_n);
            return false;
        }
        prof->m_n++;
        const u16 x0 = pts.m_que.x_at(si);
        const u16 y0 = pts.m_que.y_at(si);
        if (best_d[si] == 0xFFFFFFFFu) {
            std::printf(
                "P1_Gen_RiverLines mouth stub: sector %u coast_tiles %u glob_ocn %u center (%u,%u)\n",
                static_cast<u32>(si),
                coast_n[si],
                static_cast<u32>(glob_ocn),
                static_cast<u32>(x0),
                static_cast<u32>(y0));
            return false;
        }
        prof->m_placed++;
        const u16 nx = best_x[si];
        const u16 ny = best_y[si];
        if (nx != x0 || ny != y0) {
            if (!paint_seg(ov, dov, dist, gen, vis_ix, &tag, terrain, sec_ov, w, h, x0, y0, nx, ny, si, si, line_prof)) {
                std::printf(
                    "P1_Gen_RiverLines mouth stub: paint failed sector %u from (%u,%u) to (%u,%u)\n",
                    static_cast<u32>(si),
                    static_cast<u32>(x0),
                    static_cast<u32>(y0),
                    static_cast<u32>(nx),
                    static_cast<u32>(ny));
                return false;
            }
            prof->m_moved++;
        }
        pts.m_que.set_at(si, nx, ny);
    }
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverLines -
//================================================================================================================================

P1_Gen_RiverLines::P1_Gen_RiverLines (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

P1_Gen_RiverLines::~P1_Gen_RiverLines () {
    clear_rslt();
}

void P1_Gen_RiverLines::clear_rslt () {
    delete[] m_rslt.m_ov;
    delete[] m_rslt.m_dov;
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

bool P1_Gen_RiverLines::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    P1_Gen_RiverPtsRslt& pts,
    const P1_Gen_RiverSectorsRslt& sectors,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_OceanIndexRef& ocean) 
{
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h) || sectors.m_ov == nullptr) {
        std::printf("P1_Gen_RiverLines generate: invalid terrain or sectors\n");
        return false;
    }
    if (network.m_downstream == nullptr || network.m_sector_n == 0) {
        std::printf("P1_Gen_RiverLines generate: invalid network downstream\n");
        return false;
    }
    if (!p1_ocean_ref_ok(ocean)) {
        std::printf("P1_Gen_RiverLines generate: invalid ocean index\n");
        return false;
    }
    if (pts.m_n == 0u || !pts.m_que.ok() || w != m_prm.m_w || h != m_prm.m_h) {
        std::printf("P1_Gen_RiverLines generate: invalid pts or size mismatch\n");
        return false;
    }
    MouthProf mouth = {};
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_ov = new u8[n];
    m_rslt.m_dov = new u8[n];
    if (m_rslt.m_ov == nullptr || m_rslt.m_dov == nullptr) {
        std::printf("P1_Gen_RiverLines generate: overlay alloc failed\n");
        clear_rslt();
        return false;
    }
    std::memset(m_rslt.m_ov, 0, n);
    for (u32 i = 0; i < n; ++i) {
        m_rslt.m_dov[i] = k_dov_none;
    }
    RivLineProf prof = {};
    if (!paint_riv_lines(m_rslt.m_ov, m_rslt.m_dov, w, h, pts, sectors, network, terrain, &prof)) {
        std::printf("P1_Gen_RiverLines generate: paint_riv_lines failed\n");
        clear_rslt();
        return false;
    }
    if (!paint_mouth_stubs(
            m_rslt.m_ov,
            m_rslt.m_dov,
            w,
            h,
            pts,
            sectors,
            network,
            terrain,
            ocean.m_ov,
            ocean.m_largest_idx,
            &mouth,
            &prof)) {
        std::printf("P1_Gen_RiverLines generate: mouth stubs failed\n");
        clear_rslt();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverLines::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverLinesRslt& P1_Gen_RiverLines::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
