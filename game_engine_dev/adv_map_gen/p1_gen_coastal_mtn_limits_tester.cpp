//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "game_map_defs.h"
#include "game_primitives.h"
#include "p1_tester_util.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

#define P1_TESTER_COASTAL_MTN_CIRCUIT_MAX 40u
#define P1_TESTER_COASTAL_MTN_CIRCUIT_TILE_MAX 2000u
#define P1_TESTER_COASTAL_MTN_LIMIT_DEPTH 0u
#define P1_TESTER_LIM_PEND 255u

//================================================================================================================================
//=> - Circuit debug rebuild (tester only) -
//================================================================================================================================

struct P1_TesterCoastalMtnCircuit {
    u16 m_start_x;
    u16 m_start_y;
    u32 m_size;
    u8 m_ov_val;
};

static const u16 k_dbg_inf = 0xFFFFu;
static const i32 k_dbg_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dbg_dy4[4] = {0, 0, -1, 1};

static bool dbg_is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool dbg_in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
}

static void dbg_glob_seed (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u16* mask,
    WB_QueXY& que) 
{
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (!dbg_is_water(terrain[i]) || mask[i] != 0) {
        return;
    }
    mask[i] = 1;
    que.push(x, y);
}

static bool dbg_build_glob_ocn_mask (const u8* terrain, u16 w, u16 h, u16* mask, WB_QueXY& que) {
    if (terrain == nullptr || mask == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        mask[i] = 0;
    }
    que.clear();
    for (u32 x = 0; x < wi; ++x) {
        dbg_glob_seed(terrain, w, h, static_cast<u16>(x), 0, mask, que);
        dbg_glob_seed(terrain, w, h, static_cast<u16>(x), static_cast<u16>(hi - 1u), mask, que);
    }
    for (u32 y = 0; y < hi; ++y) {
        dbg_glob_seed(terrain, w, h, 0, static_cast<u16>(y), mask, que);
        dbg_glob_seed(terrain, w, h, static_cast<u16>(wi - 1u), static_cast<u16>(y), mask, que);
    }
    for (u32 i = 0; i < n; ++i) {
        if (terrain[i] == TERR_OCEAN[0]) {
            dbg_glob_seed(terrain, w, h, static_cast<u16>(i % wi), static_cast<u16>(i / wi), mask, que);
        }
    }
    while (que.count() > 0u) {
        const u16 px = que.x_at(0);
        const u16 py = que.y_at(0);
        que.drop(1);
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dbg_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dbg_dy4[k];
            if (!dbg_in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (!dbg_is_water(terrain[ni]) || mask[ni] != 0) {
                continue;
            }
            mask[ni] = 1;
            que.push(static_cast<u16>(nx), static_cast<u16>(ny));
        }
    }
    return true;
}

static bool dbg_tile_adj_glob_ocn (const u16* glob, u16 w, u16 h, u16 x, u16 y) {
    if (glob == nullptr) {
        return false;
    }
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dbg_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dbg_dy4[k];
        if (!dbg_in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (glob[j] != 0) {
            return true;
        }
    }
    return false;
}

static bool dbg_bfs_sec_coast_dist (
    const u16* glob,
    u16 w,
    u16 h,
    const u16* ov,
    u16 sector_n,
    const P1_RiverSectorNode* nodes,
    u16* sec_dist,
    u16* sec_que,
    u16* max_dist) 
{
    if (glob == nullptr || ov == nullptr || sec_dist == nullptr || sec_que == nullptr || max_dist == nullptr) {
        return false;
    }
    *max_dist = 0;
    for (u32 si = 0; si < static_cast<u32>(sector_n); ++si) {
        sec_dist[si] = k_dbg_inf;
    }
    u32 qh = 0;
    u32 qn = 0;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        if (sec_dist[sid] != k_dbg_inf) {
            continue;
        }
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!dbg_tile_adj_glob_ocn(glob, w, h, x, y)) {
            continue;
        }
        sec_dist[sid] = 0;
        sec_que[qn++] = sid;
    }
    while (qh < qn) {
        const u16 sid = sec_que[qh++];
        const u16 cur = sec_dist[sid];
        if (nodes == nullptr || sid >= sector_n) {
            continue;
        }
        const P1_RiverSectorNode& nd = nodes[sid];
        const u16 nxt = static_cast<u16>(cur + 1u);
        for (u16 c = 0; c < nd.m_conn_n; ++c) {
            const u16 nb = nd.m_conn[c];
            if (nb >= sector_n || sec_dist[nb] != k_dbg_inf) {
                continue;
            }
            sec_dist[nb] = nxt;
            if (nxt > *max_dist) {
                *max_dist = nxt;
            }
            sec_que[qn++] = nb;
        }
    }
    return true;
}

static bool dbg_is_depth_pair (u16 da, u16 db) {
    const u16 dep_lo = static_cast<u16>(P1_TESTER_COASTAL_MTN_LIMIT_DEPTH);
    const u16 dep_hi = static_cast<u16>(P1_TESTER_COASTAL_MTN_LIMIT_DEPTH + 1u);
    return (da == dep_lo && db == dep_hi) || (da == dep_hi && db == dep_lo);
}

static bool dbg_mark_depth_limit (
    u16 w,
    u16 h,
    const u16* ov,
    u16 sector_n,
    const u16* sec_dist,
    u8* lim,
    u32* lim_n) 
{
    if (ov == nullptr || sec_dist == nullptr || lim == nullptr || lim_n == nullptr) {
        return false;
    }
    const u16 dep_lo = static_cast<u16>(P1_TESTER_COASTAL_MTN_LIMIT_DEPTH);
    const u16 dep_hi = static_cast<u16>(P1_TESTER_COASTAL_MTN_LIMIT_DEPTH + 1u);
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *lim_n = 0;
    for (u32 i = 0; i < n; ++i) {
        lim[i] = 0;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u16 sa = ov[i];
            if (sa == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sa >= sector_n) {
                continue;
            }
            const u16 da = sec_dist[sa];
            if (da != dep_lo && da != dep_hi) {
                continue;
            }
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(x) + k_dbg_dx4[k];
                const i32 ny = static_cast<i32>(y) + k_dbg_dy4[k];
                if (!dbg_in_map(w, h, nx, ny)) {
                    continue;
                }
                const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                const u16 sb = ov[j];
                if (sb == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sb >= sector_n || sb == sa) {
                    continue;
                }
                const u16 db = sec_dist[sb];
                if ((da == dep_lo && db == dep_hi) || (da == dep_hi && db == dep_lo)) {
                    if (lim[i] == 0) {
                        lim[i] = P1_TESTER_LIM_PEND;
                        ++*lim_n;
                    }
                    if (lim[j] == 0) {
                        lim[j] = P1_TESTER_LIM_PEND;
                        ++*lim_n;
                    }
                }
            }
        }
    }
    return true;
}

static void dbg_sort_circuits_by_size (P1_TesterCoastalMtnCircuit* circ, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        P1_TesterCoastalMtnCircuit key = circ[i];
        u32 j = i;
        while (j > 0u && circ[j - 1u].m_size < key.m_size) {
            circ[j] = circ[j - 1u];
            --j;
        }
        circ[j] = key;
    }
}

static void dbg_record_circuit (
    P1_TesterCoastalMtnCircuit* circ,
    u32* circ_n,
    u16 start_x,
    u16 start_y,
    u32 sz,
    u8 ov_val) 
{
    if (sz == 0 || *circ_n >= static_cast<u32>(P1_TESTER_COASTAL_MTN_CIRCUIT_MAX)) {
        return;
    }
    P1_TesterCoastalMtnCircuit* c = &circ[*circ_n];
    c->m_start_x = start_x;
    c->m_start_y = start_y;
    c->m_size = sz;
    c->m_ov_val = ov_val;
    ++*circ_n;
}

static bool dbg_flood_label_lim_circuits (
    u16 w,
    u16 h,
    u8* lim,
    P1_TesterCoastalMtnCircuit* circ,
    u32* circ_n,
    u32* circ_found,
    WB_QueXY& que) 
{
    if (lim == nullptr || circ == nullptr || circ_n == nullptr || circ_found == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *circ_n = 0;
    *circ_found = 0;
    for (u32 i = 0; i < n; ++i) {
        if (lim[i] != P1_TESTER_LIM_PEND) {
            continue;
        }
        u32 sz = 0;
        u16 start_x = 0;
        u16 start_y = 0;
        u8 ov_val = 0;
        que.clear();
        que.push(static_cast<u16>(i % static_cast<u32>(w)), static_cast<u16>(i / static_cast<u32>(w)));
        while (que.count() > 0u) {
            const u16 cx = que.x_at(0);
            const u16 cy = que.y_at(0);
            que.drop(1);
            const u32 cur = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
            if (lim[cur] != P1_TESTER_LIM_PEND) {
                continue;
            }
            if (sz == 0) {
                ++*circ_found;
                ov_val = static_cast<u8>((*circ_found < 255u) ? *circ_found : 254u);
                start_x = cx;
                start_y = cy;
            }
            lim[cur] = ov_val;
            ++sz;
            for (i32 k = 0; k < 4; ++k) {
                const i32 nx = static_cast<i32>(cx) + k_dbg_dx4[k];
                const i32 ny = static_cast<i32>(cy) + k_dbg_dy4[k];
                if (!dbg_in_map(w, h, nx, ny)) {
                    continue;
                }
                const u32 nk = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (lim[nk] == P1_TESTER_LIM_PEND) {
                    que.push(static_cast<u16>(nx), static_cast<u16>(ny));
                }
            }
            if (sz >= static_cast<u32>(P1_TESTER_COASTAL_MTN_CIRCUIT_TILE_MAX)) {
                dbg_record_circuit(circ, circ_n, start_x, start_y, sz, ov_val);
                sz = 0;
            }
        }
        if (sz > 0) {
            dbg_record_circuit(circ, circ_n, start_x, start_y, sz, ov_val);
        }
    }
    if (*circ_n > 1u) {
        dbg_sort_circuits_by_size(circ, *circ_n);
    }
    return true;
}

static bool tester_rebuild_circ_dbg (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverSectorsRslt& sectors,
    MapArrayOverlay* circ_ov,
    P1_TesterCoastalMtnCircuit* circ,
    u32* circ_n,
    u32* circ_found) 
{
    if (terrain == nullptr || circ_ov == nullptr || circ == nullptr || circ_n == nullptr || circ_found == nullptr) {
        return false;
    }
    const u16 sector_n = sectors.m_sector_n;
    if (sector_n == 0 || sectors.m_ov == nullptr) {
        return false;
    }
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 wb_n = static_cast<i32>(tn);
    u16* sec_dist = new u16[static_cast<u32>(sector_n)];
    u16* sec_que = new u16[static_cast<u32>(sector_n)];
    if (sec_dist == nullptr || sec_que == nullptr) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    Whiteboard_2B wb_glob("P1_CoastalMtnLimitsTester", "glob", 0u);
    P1_WB_CHK(wb_glob);
    WB_QueXY que;
    if (!que.ok()) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    if (!dbg_build_glob_ocn_mask(terrain, w, h, wb_glob.get_iter_ptr(), que)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    u16 max_dist = 0;
    if (!dbg_bfs_sec_coast_dist(wb_glob.get_iter_ptr(), w, h, sectors.m_ov, sector_n, sectors.m_nodes,
            sec_dist, sec_que, &max_dist)) {
        delete[] sec_dist;
        delete[] sec_que;
        return false;
    }
    (void)max_dist;
    delete[] sec_que;
    if (!circ_ov->resize(w, h)) {
        delete[] sec_dist;
        return false;
    }
    u8* lim = circ_ov->data_w();
    u32 lim_n = 0;
    if (!dbg_mark_depth_limit(w, h, sectors.m_ov, sector_n, sec_dist, lim, &lim_n)) {
        delete[] sec_dist;
        return false;
    }
    (void)lim_n;
    delete[] sec_dist;
    return dbg_flood_label_lim_circuits(w, h, lim, circ, circ_n, circ_found, que);
}

static void tester_log_circuits (u32 circ_found, u32 circ_n, const P1_TesterCoastalMtnCircuit* circ) {
    std::printf("coastal mtn circuits (found %u, tracked %u):\n", circ_found, circ_n);
    for (u32 i = 0; i < circ_n; ++i) {
        const P1_TesterCoastalMtnCircuit& c = circ[i];
        std::printf("  #%u ov %u size %u start (%u,%u)\n",
            i,
            static_cast<u32>(c.m_ov_val),
            c.m_size,
            static_cast<u32>(c.m_start_x),
            static_cast<u32>(c.m_start_y));
    }
}

static void tester_log_selection (u32 sel_n, u32 limit_n) {
    const u32 pct = (limit_n > 0u) ? ((sel_n * 100u) / limit_n) : 0u;
    std::printf("coastal mtn selection: %u / %u tiles (%u%%)\n", sel_n, limit_n, pct);
}

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool build_step5_terrain (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ov,
    const u16* land_depth) {
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(terrain, w, h, ov) || !fill.is_valid()) {
        return false;
    }
    const P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_def();
    return p1_apply_shaped_outline(prm, sp, terrain, w, h, ov, land_depth);
}

static const u8 k_circuit_rgb[40][3] = {
    {255, 0, 0}, {0, 255, 0}, {0, 128, 255}, {255, 255, 0}, {255, 0, 255},
    {0, 255, 255}, {255, 128, 0}, {128, 0, 255}, {0, 200, 120}, {200, 200, 0},
    {255, 80, 80}, {80, 255, 80}, {80, 80, 255}, {255, 180, 60}, {60, 180, 255},
    {180, 60, 255}, {255, 60, 180}, {60, 255, 180}, {180, 255, 60}, {120, 60, 200},
    {200, 120, 60}, {60, 200, 120}, {220, 100, 40}, {40, 220, 100}, {100, 40, 220},
    {220, 40, 100}, {100, 220, 40}, {160, 0, 80}, {80, 160, 0}, {0, 80, 160},
    {160, 80, 0}, {80, 0, 160}, {0, 160, 80}, {240, 140, 140}, {140, 240, 140},
    {140, 140, 240}, {240, 200, 100}, {100, 240, 200}, {200, 100, 240}, {240, 100, 200},
};

static void circuit_rgb (u8 ov_val, u8* r, u8* g, u8* b) {
    if (ov_val == 0 || r == nullptr || g == nullptr || b == nullptr) {
        return;
    }
    const u32 idx = (static_cast<u32>(ov_val) - 1u) % static_cast<u32>(P1_TESTER_COASTAL_MTN_CIRCUIT_MAX);
    *r = k_circuit_rgb[idx][0];
    *g = k_circuit_rgb[idx][1];
    *b = k_circuit_rgb[idx][2];
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool save_coastal_mtn_limits_viz (
    cstr path,
    const u8* terrain,
    const u8* circ,
    const u8* out_ov,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || circ == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 40;
        u8 g = 40;
        u8 b = 40;
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
        if (circ[i] != 0) {
            circuit_rgb(circ[i], &r, &g, &b);
        }
        if (out_ov != nullptr && out_ov[i] == static_cast<u8>(P1_COASTAL_MTN_OV_SEL)) {
            r = 255;
            g = 255;
            b = 80;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

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

static bool color_used (const u8* clr, u32 sector_n, u8 r, u8 g, u8 b) {
    for (u32 si = 0; si < sector_n; ++si) {
        if (clr[si * 3u + 0] == r && clr[si * 3u + 1] == g && clr[si * 3u + 2] == b) {
            return true;
        }
    }
    return false;
}

static bool save_coastal_mtn_sel_sector_viz (
    cstr path,
    u32 seed,
    const u8* terrain,
    const u16* sec_ov,
    u16 sector_n,
    const u8* out_ov,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || sec_ov == nullptr || out_ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
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
    const u32 sn = static_cast<u32>(sector_n);
    u8* clr = new u8[sn * 3u];
    if (clr == nullptr) {
        delete[] rgb;
        return false;
    }
    Rng32 rng;
    rng_seed(&rng, seed);
    for (u32 si = 0; si < sn; ++si) {
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
        const u16 sid = sec_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        const u32 p = i * 3u;
        const u32 c = static_cast<u32>(sid) * 3u;
        rgb[p + 0] = clr[c + 0];
        rgb[p + 1] = clr[c + 1];
        rgb[p + 2] = clr[c + 2];
    }
    delete[] clr;
    for (u32 i = 0; i < n; ++i) {
        if (out_ov[i] != static_cast<u8>(P1_COASTAL_MTN_OV_SEL)) {
            continue;
        }
        rgb[i * 3u + 0] = 0;
        rgb[i * 3u + 1] = 0;
        rgb[i * 3u + 2] = 0;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_coastal_mtn_out_viz (cstr path, const u8* out_ov, u16 w, u16 h) {
    if (path == nullptr || out_ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 v = out_ov[i];
        rgb[i * 3u + 0] = v;
        rgb[i * 3u + 1] = v;
        rgb[i * 3u + 2] = v;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_p1_gen_coastal_mtn_limits_basic (const P1_RunPrm& prm) {
    char out_path[320];
    char sel_sec_path[320];
    char out_ov_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    if (!p1_tester_make_step_out(prm.m_seed, 10u, "coastal_mtn_sel_sectors", sel_sec_path, sizeof(sel_sec_path))) {
        std::printf("failed to build selection sector viz path\n");
        return -1;
    }
    if (!p1_tester_make_step_out(prm.m_seed, 10u, "coastal_mtn_out", out_ov_path, sizeof(out_ov_path))) {
        std::printf("failed to build output overlay viz path\n");
        return -1;
    }
    const clock_t t0i = clock();
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        std::printf("P1_Gen_ContOutlines failed for step 10 input\n");
        return -1;
    }
    const u16 w = ov_map.width();
    const u16 h = ov_map.height();
    const u8* ov = ov_map.data();
    if (ov == nullptr || w == 0 || h == 0) {
        std::printf("invalid outline overlay\n");
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        std::printf("P1_Gen_LandDepth failed for step 10 input\n");
        return -1;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        std::printf("invalid land depth data\n");
        return -1;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    if (!build_step5_terrain(prm, terrain, w, h, ov, land_depth)) {
        std::printf("P1_Gen_ShapedOutline failed for step 10 input\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h) || !pts_gen.is_valid()) {
        std::printf("P1_Gen_RiverPts failed for step 10 input\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result()) || !sec_gen.is_valid()) {
        std::printf("P1_Gen_RiverSectors failed for step 10 input\n");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_CoastalMtnLimits lim_gen(prm);
    const clock_t t0 = clock();
    const bool ok = lim_gen.generate(terrain, w, h, sec_gen.result());
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !lim_gen.is_valid()) {
        std::printf("P1_Gen_CoastalMtnLimits failed to generate\n");
        delete[] terrain;
        return -1;
    }
    const P1_Gen_CoastalMtnLimitsRslt& r = lim_gen.result();
    P1_TesterCoastalMtnCircuit dbg_circ[P1_TESTER_COASTAL_MTN_CIRCUIT_MAX];
    u32 dbg_circ_n = 0;
    u32 dbg_circ_found = 0;
    MapArrayOverlay circ_map;
    if (!tester_rebuild_circ_dbg(terrain, w, h, sec_gen.result(), &circ_map, dbg_circ, &dbg_circ_n, &dbg_circ_found)) {
        std::printf("failed to rebuild circuit debug overlay\n");
        delete[] terrain;
        return -1;
    }
    std::printf("P1 steps 1-9 input time: %.6f s\n", sec_i);
    std::printf("P1_Gen_CoastalMtnLimits gen time: %.6f s (sectors %u, max sec dist %u, limit tiles %u, circuits %u/%u, selected %u, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_sector_n),
        static_cast<u32>(r.m_max_sec_dist),
        r.m_limit_n,
        dbg_circ_n,
        dbg_circ_found,
        r.m_sel_n,
        static_cast<u32>(w),
        static_cast<u32>(h));
    tester_log_circuits(dbg_circ_found, dbg_circ_n, dbg_circ);
    tester_log_selection(r.m_sel_n, r.m_limit_n);
    const u8* circ = circ_map.data();
    const u8* out_ov = r.m_limit_ov.data();
    const P1_Gen_RiverSectorsRslt& sec_r = sec_gen.result();
    if (circ == nullptr || out_ov == nullptr
        || !save_coastal_mtn_limits_viz(out_path, terrain, circ, out_ov, w, h)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] terrain;
        return -1;
    }
    if (sec_r.m_ov == nullptr
        || !save_coastal_mtn_sel_sector_viz(sel_sec_path, prm.m_seed, terrain, sec_r.m_ov, sec_r.m_sector_n, out_ov, w, h)) {
        std::printf("failed to save map: %s\n", sel_sec_path);
        delete[] terrain;
        return -1;
    }
    if (!save_coastal_mtn_out_viz(out_ov_path, out_ov, w, h)) {
        std::printf("failed to save map: %s\n", out_ov_path);
        delete[] terrain;
        return -1;
    }
    delete[] terrain;
    std::printf("saved: %s\n", out_path);
    std::printf("saved: %s\n", sel_sec_path);
    std::printf("saved: %s\n", out_ov_path);
    if (!p1_tester_whiteboard_chk()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (!p1_tester_checkout(argc, argv)) {
        return -1;
    }
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    return test_p1_gen_coastal_mtn_limits_basic(prm); 
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
