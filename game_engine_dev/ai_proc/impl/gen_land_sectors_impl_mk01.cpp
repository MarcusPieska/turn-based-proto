//================================================================================================================================
//=> - Includes (mk01: staggered lattice + small-mass seeds, whiteboard scratch) -
//================================================================================================================================

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "tile_attr_tables.h"

//================================================================================================================================
//=> - Local (mk01) -
//================================================================================================================================

#define GLS_LAT 25u
#define GLS_JIT_PCT 30u
#define GLS_MIN_MASS (static_cast<u32>(GLS_LAT) * static_cast<u32>(GLS_LAT) * 2u)

static bool gls_is_mass_land (u8 terr) {
    if (terr == TERR_NONE[0]) {
        return false;
    }
    return !overlay_is_water_terr(terr);
}

static bool gls_is_walk_land (u8 terr) {
    if (overlay_is_water_terr(terr) || terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0]
        || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static u16 gls_clamp_u16 (i32 v, u16 lo, u16 hi) {
    if (v < static_cast<i32>(lo)) {
        return lo;
    }
    if (v > static_cast<i32>(hi)) {
        return hi;
    }
    return static_cast<u16>(v);
}

static i32 gls_rng_jit (u32* rng, i32 jit) {
    *rng = (*rng * 1103515245u) + 12345u;
    const i32 span = jit * 2 + 1;
    return static_cast<i32>((*rng >> 16) % static_cast<u32>(span)) - jit;
}

static void gls_fill_land_sec_n (LandSectorSeedPt* pts, u16 pn) {
    if (pts == nullptr || pn == 0u) {
        return;
    }
    for (u16 i = 0; i < pn; ++i) {
        pts[i].m_land_sec_n = 0u;
    }
    for (u16 i = 0; i < pn; ++i) {
        if (pts[i].m_land_sec_n != 0u) {
            continue;
        }
        const u16 land = pts[i].m_land;
        u16 n = 0u;
        for (u16 j = i; j < pn; ++j) {
            if (pts[j].m_land == land) {
                ++n;
            }
        }
        for (u16 j = i; j < pn; ++j) {
            if (pts[j].m_land == land) {
                pts[j].m_land_sec_n = n;
            }
        }
    }
}

static u32 gls_tile_yield_sum (const GameArraySimple& map, u16 x, u16 y) {
    if (!TileAttrTables::ready()) {
        return 0u;
    }
    i32 food = 0;
    i32 prod = 0;
    i32 comm = 0;
    const TileAttributeStaticDataStruct& terr = TileAttrTables::terr(map.get_terrain(x, y));
    const TileAttributeStaticDataStruct& clim = TileAttrTables::clim(map.get_climate(x, y));
    const TileAttributeStaticDataStruct& ov = TileAttrTables::ov(map.get_overlay(x, y));
    food += static_cast<i32>(terr.food) + static_cast<i32>(clim.food) + static_cast<i32>(ov.food);
    prod += static_cast<i32>(terr.production) + static_cast<i32>(clim.production) + static_cast<i32>(ov.production);
    comm += static_cast<i32>(terr.commerce) + static_cast<i32>(clim.commerce) + static_cast<i32>(ov.commerce);
    if (map.get_river(x, y) != 0u) {
        const TileAttributeStaticDataStruct& riv = TileAttrTables::riv();
        food += static_cast<i32>(riv.food);
        prod += static_cast<i32>(riv.production);
        comm += static_cast<i32>(riv.commerce);
    }
    if (food < 0) {
        food = 0;
    }
    if (prod < 0) {
        prod = 0;
    }
    if (comm < 0) {
        comm = 0;
    }
    return static_cast<u32>(food + prod + comm);
}

static void gls_fill_sector_stats (
    LandSectorSeedPt* pts,
    u16 pn,
    const Whiteboard_2B& sec,
    const GameArraySimple& map)
{
    if (pts == nullptr || pn == 0u || !sec.ok()) {
        return;
    }
    for (u16 i = 0; i < pn; ++i) {
        pts[i].m_tiles = 0u;
        pts[i].m_yields = 0u;
        pts[i].m_res = 0u;
    }
    Whiteboard_2B wb_own("GenLandSectors", "tag_own", 0u);
    if (!wb_own.ok()) {
        return;
    }
    const u16 w = map.width();
    const u32 n = WhiteboardMng::tile_n();
    const u32 wi = static_cast<u32>(w);
    std::memset(wb_own.get_iter_ptr(), 0xff, static_cast<size_t>(n) * sizeof(u16));
    for (u16 i = 0; i < pn; ++i) {
        const u16 tag = sec.rd(pts[i].m_x, pts[i].m_y);
        if (tag != GLS_IDX_NONE) {
            wb_own.wr_i(static_cast<u32>(tag), i);
        }
    }
    for (u32 ti = 0; ti < n; ++ti) {
        const u16 tag = sec.rd_i(ti);
        if (tag == GLS_IDX_NONE) {
            continue;
        }
        const u16 si = wb_own.rd_i(static_cast<u32>(tag));
        if (si == U16_KEY_NULL || si >= pn) {
            continue;
        }
        const u16 y = static_cast<u16>(ti / wi);
        const u16 x = static_cast<u16>(ti - static_cast<u32>(y) * wi);
        pts[si].m_tiles = static_cast<u32>(pts[si].m_tiles + 1u);
        pts[si].m_yields = static_cast<u32>(pts[si].m_yields + gls_tile_yield_sum(map, x, y));
        if (map.get_res(x, y) != U16_KEY_NULL) {
            pts[si].m_res = static_cast<u32>(pts[si].m_res + 1u);
        }
    }
}

//================================================================================================================================
//=> - GenLandSectors mk01 -
//================================================================================================================================

bool GenLandSectors::gen_seeds (u32 rng_seed, LandSectorSeeds* out) {
    GAME_EXPECT_RET(m_ok && m_map != nullptr, false, "GenLandSectors gen_seeds not begun");
    if (out == nullptr) {
        return false;
    }
    free_seeds(out);
    const GameArraySimple& map = *m_map;
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u || GLS_LAT == 0u) {
        return false;
    }
    Whiteboard_2B wb_mass("GenLandSectors", "mass", 0u);
    Whiteboard_4B wb_q("GenLandSectors", "q", 0u);
    Whiteboard_4B wb_sz("GenLandSectors", "sz", 0u);
    Whiteboard_4B wb_samp("GenLandSectors", "samp", 0u);
    Whiteboard_1B wb_used("GenLandSectors", "used", 0u);
    if (!wb_mass.ok() || !wb_q.ok() || !wb_sz.ok() || !wb_samp.ok() || !wb_used.ok()) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    std::memset(wb_mass.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u16));
    std::memset(wb_sz.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u32));
    std::memset(wb_samp.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u32));
    std::memset(wb_used.get_iter_ptr(), 0, static_cast<size_t>(n) * sizeof(u8));
    u16* mass = wb_mass.get_iter_ptr();
    u32* q = wb_q.get_iter_ptr();
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    u16 mass_n = 0u;
    u16 small_n = 0u;
    for (u32 i = 0; i < n; ++i) {
        if (mass[i] != 0u) {
            continue;
        }
        const u16 y0 = static_cast<u16>(i / wi);
        const u16 x0 = static_cast<u16>(i - static_cast<u32>(y0) * wi);
        if (!gls_is_mass_land(map.get_terrain(x0, y0))) {
            continue;
        }
        if (mass_n == U16_KEY_NULL) {
            return false;
        }
        const u16 mid = static_cast<u16>(mass_n + 1u);
        ++mass_n;
        u32 qn = 0u;
        mass[i] = mid;
        q[qn++] = i;
        u32 tiles = 0u;
        u32 samp_i = 0u;
        for (u32 qh = 0u; qh < qn; ++qh) {
            const u32 cur = q[qh];
            ++tiles;
            const u32 py = cur / wi;
            const u32 px = cur - py * wi;
            const u16 ux0 = static_cast<u16>(px);
            const u16 uy0 = static_cast<u16>(py);
            if (samp_i == 0u && gls_is_walk_land(map.get_terrain(ux0, uy0))) {
                samp_i = cur + 1u;
            }
            for (i32 d = 0; d < 4; ++d) {
                const i32 nx = static_cast<i32>(px) + dx4[d];
                const i32 ny = static_cast<i32>(py) + dy4[d];
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                const u32 ni = static_cast<u32>(uy) * wi + static_cast<u32>(ux);
                if (mass[ni] != 0u || !gls_is_mass_land(map.get_terrain(ux, uy))) {
                    continue;
                }
                mass[ni] = mid;
                q[qn++] = ni;
            }
        }
        wb_sz.wr_i(static_cast<u32>(mid), tiles);
        wb_samp.wr_i(static_cast<u32>(mid), samp_i);
        if (tiles < GLS_MIN_MASS && samp_i != 0u) {
            ++small_n;
        }
    }
    const u32 lat_cap = (static_cast<u32>(w) / static_cast<u32>(GLS_LAT) + 1u)
        * (static_cast<u32>(h) / static_cast<u32>(GLS_LAT) + 1u);
    const u32 cap = lat_cap + static_cast<u32>(small_n) + 1u;
    LandSectorSeedPt* pts = new LandSectorSeedPt[cap];
    u16 pn = 0u;
    const i32 jit = static_cast<i32>((static_cast<u32>(GLS_LAT) * static_cast<u32>(GLS_JIT_PCT)) / 100u);
    const i32 half = static_cast<i32>(GLS_LAT / 2u);
    u32 rng = rng_seed ^ 0xA5A5A5A5u;
    u16 row_i = 0u;
    for (u16 gy = 0; gy < h; gy = static_cast<u16>(gy + GLS_LAT), ++row_i) {
        const i32 stag = (row_i & 1u) != 0u ? half : -half;
        for (u16 gx = 0; gx < w; gx = static_cast<u16>(gx + GLS_LAT)) {
            const u16 x = gls_clamp_u16(
                static_cast<i32>(gx) + stag + gls_rng_jit(&rng, jit), 0u, static_cast<u16>(w - 1u));
            const u16 y = gls_clamp_u16(
                static_cast<i32>(gy) + gls_rng_jit(&rng, jit), 0u, static_cast<u16>(h - 1u));
            const u32 ti = static_cast<u32>(y) * wi + static_cast<u32>(x);
            if (wb_used.rd_i(ti) != 0u) {
                continue;
            }
            if (!gls_is_walk_land(map.get_terrain(x, y))) {
                continue;
            }
            const u16 mid = mass[ti];
            if (mid == 0u || wb_sz.rd_i(static_cast<u32>(mid)) < GLS_MIN_MASS) {
                continue;
            }
            if (pn >= cap) {
                break;
            }
            pts[pn].m_x = x;
            pts[pn].m_y = y;
            pts[pn].m_land = mid;
            pts[pn].m_land_sec_n = 0u;
            pts[pn].m_tiles = 0u;
            pts[pn].m_yields = 0u;
            pts[pn].m_res = 0u;
            ++pn;
            wb_used.wr_i(ti, 1u);
        }
    }
    for (u16 mid = 1u; mid <= mass_n; ++mid) {
        if (wb_sz.rd_i(static_cast<u32>(mid)) >= GLS_MIN_MASS) {
            continue;
        }
        const u32 samp_i = wb_samp.rd_i(static_cast<u32>(mid));
        if (samp_i == 0u) {
            continue;
        }
        const u32 ti = samp_i - 1u;
        const u16 y = static_cast<u16>(ti / wi);
        const u16 x = static_cast<u16>(ti - static_cast<u32>(y) * wi);
        if (wb_used.rd_i(ti) != 0u) {
            continue;
        }
        if (pn >= cap) {
            break;
        }
        pts[pn].m_x = x;
        pts[pn].m_y = y;
        pts[pn].m_land = mid;
        pts[pn].m_land_sec_n = 0u;
        pts[pn].m_tiles = 0u;
        pts[pn].m_yields = 0u;
        pts[pn].m_res = 0u;
        ++pn;
        wb_used.wr_i(ti, 1u);
    }
    if (pn == 0u) {
        delete[] pts;
        out->m_pts = nullptr;
        out->m_n = 0u;
        return true;
    }
    gls_fill_land_sec_n(pts, pn);
    out->m_pts = pts;
    out->m_n = pn;
    if (!build(*out)) {
        free_seeds(out);
        return false;
    }
    gls_fill_sector_stats(out->m_pts, out->m_n, m_sec, map);
    return true;
}

bool GenLandSectors::build (const LandSectorSeeds& seeds) {
    GAME_EXPECT_RET(m_ok && m_map != nullptr, false, "GenLandSectors build not begun");
    clr();
    if (seeds.m_n == 0u || seeds.m_pts == nullptr) {
        return true;
    }
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    const u32 wi = static_cast<u32>(w);
    Whiteboard_4B wb_q("GenLandSectors", "flood_q", 0u);
    if (!wb_q.ok()) {
        return false;
    }
    u32* q = wb_q.get_iter_ptr();
    u32 qn = 0u;
    for (u16 i = 0; i < seeds.m_n; ++i) {
        const u16 x = seeds.m_pts[i].m_x;
        const u16 y = seeds.m_pts[i].m_y;
        if (x >= w || y >= h || !gls_is_walk_land(m_map->get_terrain(x, y))) {
            continue;
        }
        const u32 ti = static_cast<u32>(y) * wi + static_cast<u32>(x);
        if (m_sec.rd_i(ti) != GLS_IDX_NONE) {
            continue;
        }
        const u16 tag = static_cast<u16>(m_sec_n + 1u);
        m_sec.wr_i(ti, tag);
        q[qn++] = ti;
        ++m_paint_n;
        ++m_sec_n;
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 sid = m_sec.rd_i(i);
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = static_cast<u32>(uy) * wi + static_cast<u32>(ux);
            if (m_sec.rd_i(ni) != GLS_IDX_NONE) {
                continue;
            }
            if (!gls_is_walk_land(m_map->get_terrain(ux, uy))) {
                continue;
            }
            if (m_map->get_mtn_line(static_cast<u16>(px), static_cast<u16>(py)) != 0u
                && m_map->get_mtn_line(ux, uy) == 0u) {
                continue;
            }
            m_sec.wr_i(ni, sid);
            q[qn++] = ni;
            ++m_paint_n;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
