//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <cstring>

#include "p1_gen_rich_coast_fertility.h"
#include "game_map_defs.h"
#include "perlin_noise.h"
#include "wb_que_xy.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_dist_inf = 0xFFFFu;
static const u32 k_brush_cap = static_cast<u32>(P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u)
    * static_cast<u32>(P1_RICH_COAST_BRUSH_MAX_RAD * 2u + 1u);

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]
        || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool is_shallow_src (u8 cls) {
    return cls == TERR_COASTAL[0] || cls == TERR_INLAND_SEA[0] || cls == TERR_INLAND_LAKE[0];
}

static bool land_adj_water (const u8* terrain, u16 w, u16 h, u16 x, u16 y) {
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (x > 0 && is_water(terrain[i - 1u])) {
        return true;
    }
    if (static_cast<u32>(x) + 1u < static_cast<u32>(w) && is_water(terrain[i + 1u])) {
        return true;
    }
    if (y > 0 && is_water(terrain[i - static_cast<u32>(w)])) {
        return true;
    }
    if (static_cast<u32>(y) + 1u < static_cast<u32>(h) && is_water(terrain[i + static_cast<u32>(w)])) {
        return true;
    }
    return false;
}

static bool bfs_land_dist_coast (
    u16 w,
    u16 h,
    const u8* terrain,
    u16 stamp_lim,
    u16* dist,
    WB_QueXY& q) 
{
    if (terrain == nullptr || dist == nullptr || !q.ok()) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_dist_inf;
    }
    q.clear();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (!is_land(terrain[i]) || !land_adj_water(terrain, w, h, x, y)) {
                continue;
            }
            dist[i] = 0;
            if (!q.push(x, y)) {
                return false;
            }
        }
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u16 cur = dist[i];
        if (cur >= stamp_lim) {
            continue;
        }
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_land(terrain[j]) && dist[j] == k_dist_inf) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px - 1), py)) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_land(terrain[j]) && dist[j] == k_dist_inf) {
                dist[j] = nxt;
                if (!q.push(static_cast<u16>(px + 1), py)) {
                    return false;
                }
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_land(terrain[j]) && dist[j] == k_dist_inf) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py - 1))) {
                    return false;
                }
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_land(terrain[j]) && dist[j] == k_dist_inf) {
                dist[j] = nxt;
                if (!q.push(px, static_cast<u16>(py + 1))) {
                    return false;
                }
            }
        }
    }
    return true;
}

static u16 sat_add (u16 a, u16 b) {
    const u32 s = static_cast<u32>(a) + static_cast<u32>(b);
    return s > 65535u ? static_cast<u16>(65535u) : static_cast<u16>(s);
}

static u16 build_fert_ov (
    const u8* terrain,
    u16 w,
    u16 h,
    i32 brush_rad,
    u16 stamp_lim,
    u16 brush_sp_n,
    const i8* brush_sp_dx,
    const i8* brush_sp_dy,
    const u16* brush_sp_v,
    const u16* dist_coast,
    u16* ov) 
{
    if (terrain == nullptr || brush_sp_dx == nullptr || brush_sp_dy == nullptr
        || brush_sp_v == nullptr || dist_coast == nullptr || ov == nullptr || brush_rad < 0) {
        return 0;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        ov[i] = 0;
    }
    u16 peak = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 si = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (!is_shallow_src(terrain[si])) {
                continue;
            }
            const i32 sx = static_cast<i32>(x);
            const i32 sy = static_cast<i32>(y);
            for (u16 k = 0; k < brush_sp_n; ++k) {
                const i32 tx = sx + static_cast<i32>(brush_sp_dx[k]);
                const i32 ty = sy + static_cast<i32>(brush_sp_dy[k]);
                if (!in_map(w, h, tx, ty)) {
                    continue;
                }
                const u32 ti = static_cast<u32>(ty) * static_cast<u32>(w) + static_cast<u32>(tx);
                if (!is_land(terrain[ti])) {
                    continue;
                }
                const u16 dc = dist_coast[ti];
                if (dc == k_dist_inf || dc > stamp_lim) {
                    continue;
                }
                const u16 nv = sat_add(ov[ti], brush_sp_v[k]);
                ov[ti] = nv;
                if (nv > peak) {
                    peak = nv;
                }
            }
        }
    }
    return peak;
}

static bool save_ov_gray_pgm (cstr path, const u16* ov, u16 w, u16 h, u16 peak) {
    if (path == nullptr || ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P5\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const u32 pk = static_cast<u32>(peak > 0 ? peak : 1u);
    u8 row[4096];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            row[x] = static_cast<u8>((static_cast<u32>(ov[i]) * 255u) / pk);
        }
        if (std::fwrite(row, 1, static_cast<size_t>(w), fp) != static_cast<size_t>(w)) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RichCoastFertility -
//================================================================================================================================

P1_Gen_RichCoastFertility::P1_Gen_RichCoastFertility (
    const P1_RunPrm& prm,
    const P1_Gen_RichCoastFertilityPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt(),
    m_ov_wb(nullptr),
    m_brush_w(0),
    m_brush_h(0),
    m_brush_sp_n(0) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_peak = 0;
    m_rslt.m_ov = nullptr;
    m_ov_wb = new Whiteboard_2B("P1_Gen_RichCoastFertility", "ov", prm.m_seed);
    P1_WB_CHK(*m_ov_wb);
    build_brush();
}

P1_Gen_RichCoastFertility::~P1_Gen_RichCoastFertility () {
    delete m_ov_wb;
    m_ov_wb = nullptr;
}

bool P1_Gen_RichCoastFertility::build_brush () {
    m_brush_w = 0;
    m_brush_h = 0;
    m_brush_sp_n = 0;
    const u16 rad = m_sp.m_brush_rad;
    const u16 peak = m_sp.m_brush_peak;
    if (rad == 0 || peak == 0 || rad > static_cast<u16>(P1_RICH_COAST_BRUSH_MAX_RAD)) {
        return false;
    }
    const u16 bw = static_cast<u16>(rad * 2u + 1u);
    const f32 frad = static_cast<f32>(rad);
    const f32 fpeak = static_cast<f32>(peak);
    const i32 irad = static_cast<i32>(rad);
    u16 sp_n = 0;
    for (u16 by = 0; by < bw; ++by) {
        for (u16 bx = 0; bx < bw; ++bx) {
            const i32 dx = static_cast<i32>(bx) - irad;
            const i32 dy = static_cast<i32>(by) - irad;
            const f32 dist = std::sqrt(static_cast<f32>(dx * dx + dy * dy));
            u16 val = 0;
            if (dist <= frad) {
                val = static_cast<u16>(std::lrint(fpeak * (dist / frad)));
            }
            m_brush[static_cast<u32>(by) * static_cast<u32>(bw) + static_cast<u32>(bx)] = val;
            if (val != 0 && sp_n < k_brush_cap) {
                m_brush_sp_dx[sp_n] = static_cast<i8>(dx);
                m_brush_sp_dy[sp_n] = static_cast<i8>(dy);
                m_brush_sp_v[sp_n] = val;
                ++sp_n;
            }
        }
    }
    m_brush_w = bw;
    m_brush_h = bw;
    m_brush_sp_n = sp_n;
    return true;
}

bool P1_Gen_RichCoastFertility::generate (const u8* terrain, u16 w, u16 h) {
    m_valid_generation = false;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_peak = 0;
    m_rslt.m_ov = nullptr;
    if (terrain == nullptr || !p1_map_size_ok(w, h) || m_brush_w == 0 || m_brush_sp_n == 0 || m_sp.m_brush_rad == 0) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (m_ov_wb == nullptr || !m_ov_wb->ok()) {
        p1_wb_fail("P1_Gen_RichCoastFertility", "ov");
    }
    Whiteboard_2B wb_dist("P1_Gen_RichCoastFertility", "dist", m_prm.m_seed);
    P1_WB_CHK(wb_dist);
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
    u16* dist_coast = wb_dist.get_iter_ptr();
    u16* ov = m_ov_wb->get_iter_ptr();
    if (!bfs_land_dist_coast(w, h, terrain, m_sp.m_stamp_lim, dist_coast, que)) {
        return false;
    }
    const u16 peak = build_fert_ov(terrain, w, h, static_cast<i32>(m_sp.m_brush_rad), m_sp.m_stamp_lim,
        m_brush_sp_n, m_brush_sp_dx, m_brush_sp_dy, m_brush_sp_v, dist_coast, ov);
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_rslt.m_peak = peak;
    m_rslt.m_ov = ov;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RichCoastFertility::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RichCoastFertilityRslt& P1_Gen_RichCoastFertility::result () const {
    return m_rslt;
}

u16 P1_Gen_RichCoastFertility::brush_w () const {
    return m_brush_w;
}

u16 P1_Gen_RichCoastFertility::brush_h () const {
    return m_brush_h;
}

const u16* P1_Gen_RichCoastFertility::brush_data () const {
    return m_brush;
}

void P1_Gen_RichCoastFertility::save_brush (cstr path) const {
    if (path == nullptr || m_brush_w == 0 || m_brush_h == 0 || m_sp.m_brush_peak == 0) {
        return;
    }
    const u32 n = static_cast<u32>(m_brush_w) * static_cast<u32>(m_brush_h);
    const u32 pk = static_cast<u32>(m_sp.m_brush_peak);
    u8 gray[65u * 65u];
    if (n > k_brush_cap) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        gray[i] = static_cast<u8>((static_cast<u32>(m_brush[i]) * 255u) / pk);
    }
    save_perlin_gray_pgm(path, gray, m_brush_w, m_brush_h);
}

void P1_Gen_RichCoastFertility::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr || m_rslt.m_ov == nullptr) {
        return;
    }
    char pgm_path[360];
    std::snprintf(pgm_path, sizeof(pgm_path), "%s", path);
    const size_t len = std::strlen(pgm_path);
    if (len >= 4 && pgm_path[len - 4] == '.' && pgm_path[len - 3] == 'p'
        && pgm_path[len - 2] == 'p' && pgm_path[len - 1] == 'm') {
        pgm_path[len - 3] = 'p';
        pgm_path[len - 2] = 'g';
        pgm_path[len - 1] = 'm';
    }
    save_ov_gray_pgm(pgm_path, m_rslt.m_ov, m_rslt.m_w, m_rslt.m_h, m_rslt.m_peak);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
