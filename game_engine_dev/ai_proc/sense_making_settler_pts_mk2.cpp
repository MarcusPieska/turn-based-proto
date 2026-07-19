//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sense_making_settler_pts.h"

#include "circular_tile_areas.h"
#include "city_blocking_mask.h"
#include "game_array_simple.h"
#include "../simple_map_gen/generator_constants.h"

//================================================================================================================================
//=> - Circ band -
//================================================================================================================================

static const u32 k_band_stamp_per_src = 256u;

static u32* g_tag = nullptr;
static u32 g_px_n = 0;
static u32 g_pass = 0;

static bool scratch_resize (u32 n) {
    if (g_px_n >= n) {
        return true;
    }
    u32* nt = new u32[n];
    if (nt == nullptr) {
        return false;
    }
    delete[] g_tag;
    g_tag = nt;
    g_px_n = n;
    return true;
}

static u32 scratch_pass (u32 n) {
    if (!scratch_resize(n)) {
        return 0;
    }
    g_pass = g_pass + 1u;
    if (g_pass == 0) {
        for (u32 i = 0; i < n; ++i) {
            g_tag[i] = 0;
        }
        g_pass = 1;
    }
    return g_pass;
}

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_u16_inf = 0xFFFFu;

static bool is_settle_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static bool is_own_ok (u8 own, u16 player) {
    return own == 0 || static_cast<u16>(own) == player;
}

static bool tile_ok (u8 cls, u8 own, u8 blk, u16 player) {
    return is_settle_land(cls) && is_own_ok(own, player) && blk == 0;
}

static u32 tile_idx (u16 w, u16 px, u16 py) {
    return static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_hill (u8 cls) {
    return cls == TERR_HILLS[0];
}

static bool adj_coastal (u16 w, u16 h, const u8* cls, u16 px, u16 py) {
    const u32 i = tile_idx(w, px, py);
    if (px > 0 && is_water(cls[i - 1u])) {
        return true;
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(w) && is_water(cls[i + 1u])) {
        return true;
    }
    if (py > 0 && is_water(cls[i - static_cast<u32>(w)])) {
        return true;
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(h) && is_water(cls[i + static_cast<u32>(w)])) {
        return true;
    }
    return false;
}

static u16 dist_to_cap (u16 px, u16 py, u16 cx, u16 cy) {
    const u16 dx = px >= cx ? static_cast<u16>(px - cx) : static_cast<u16>(cx - px);
    const u16 dy = py >= cy ? static_cast<u16>(py - cy) : static_cast<u16>(cy - py);
    return static_cast<u16>(dx + dy);
}

static i32 l2w_cost (const MapArrayDistance& l2w, u16 w, u16 h, const u8* cls, u16 px, u16 py) {
    if (l2w.data() == nullptr || l2w.width() != w || l2w.height() != h) {
        return 0;
    }
    const u32 i = tile_idx(w, px, py);
    const u16 raw = l2w.data()[i];
    if (raw == k_u16_inf) {
        if (is_water(cls[i])) {
            return -1;
        }
        return 32767;
    }
    return static_cast<i32>(raw);
}

static i32 pt_cost (
    u16 w,
    u16 h,
    const u8* cls,
    const MapArrayDistance& l2w,
    u16 px,
    u16 py,
    u16 cx,
    u16 cy) 
{
    i32 cost = static_cast<i32>(dist_to_cap(px, py, cx, cy));
    const u32 i = tile_idx(w, px, py);
    if (is_hill(cls[i])) {
        cost = cost + 10;
    }
    cost = cost + ((l2w_cost(l2w, w, h, cls, px, py)) >> 1);
    if (adj_coastal(w, h, cls, px, py)) {
        cost = cost >> 1;
    }
    return cost;
}

static void push_pt (SmSettlerPtResult* out, u16 px, u16 py) {
    if (out->n >= SMS_MAX_SETTLER_PTS) {
        return;
    }
    out->pts[out->n].x = px;
    out->pts[out->n].y = py;
    out->n = out->n + 1u;
}

static void stamp_band (
    u16 w,
    u16 h,
    const u8* cls,
    const u8* own,
    const u8* blk,
    u16 player,
    u16 sx,
    u16 sy,
    u16 inner_r,
    u16 outer_r,
    u32 pass,
    u32* tag,
    u32* touched,
    u32* touched_n,
    u32 touched_cap)
{
    const CircArea inner = CircularTileAreas::get(inner_r);
    const CircArea outer = CircularTileAreas::get(outer_r);
    if (outer.m_lim == 0 || outer.m_brd == nullptr || inner.m_lim > outer.m_lim) {
        return;
    }
    for (u16 i = inner.m_lim; i < outer.m_lim; ++i) {
        const i32 px = static_cast<i32>(sx) + static_cast<i32>(outer.m_brd[i][0]);
        const i32 py = static_cast<i32>(sy) + static_cast<i32>(outer.m_brd[i][1]);
        if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
            continue;
        }
        const u32 ti = tile_idx(w, static_cast<u16>(px), static_cast<u16>(py));
        if (!tile_ok(cls[ti], own[ti], blk[ti], player)) {
            continue;
        }
        if (tag[ti] == pass) {
            continue;
        }
        tag[ti] = pass;
        if (*touched_n < touched_cap) {
            touched[*touched_n] = ti;
            *touched_n = *touched_n + 1u;
        }
    }
}

static void collect_touched (
    u16 w,
    const u8* cls,
    const u8* own,
    const u8* blk,
    u16 player,
    u32 pass,
    const u32* tag,
    const u32* touched,
    u32 touched_n,
    SmSettlerPtResult* out)
{
    for (u32 k = 0; k < touched_n; ++k) {
        const u32 ti = touched[k];
        if (tag[ti] != pass) {
            continue;
        }
        if (!tile_ok(cls[ti], own[ti], blk[ti], player)) {
            continue;
        }
        const u16 px = static_cast<u16>(ti % static_cast<u32>(w));
        const u16 py = static_cast<u16>(ti / static_cast<u32>(w));
        push_pt(out, px, py);
    }
}

//================================================================================================================================
//=> - SenseMakingSettlerPt -
//================================================================================================================================

SmSettlerPtResult SenseMakingSettlerPt::select (
    const MapTerrainData& map,
    const MapTerrainData& own,
    const MapTerrainData& blk,
    u16 player,
    u16 min_dist,
    u16 max_dist,
    const SpgCoordPair* src,
    u32 src_n)
{
    SmSettlerPtResult out = {};
    if (map.data() == nullptr || map.width() == 0 || map.height() == 0) {
        return out;
    }
    if (own.data() == nullptr || own.width() != map.width() || own.height() != map.height()) {
        return out;
    }
    if (blk.data() == nullptr || blk.width() != map.width() || blk.height() != map.height()) {
        return out;
    }
    if (min_dist >= max_dist || src == nullptr || src_n == 0) {
        return out;
    }
    const CircArea inner = CircularTileAreas::get(min_dist);
    const CircArea outer = CircularTileAreas::get(max_dist);
    if (outer.m_lim == 0 || inner.m_lim > outer.m_lim) {
        return out;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* cls = map.data();
    const u8* own_rows = own.data();
    const u8* blk_rows = blk.data();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 pass = scratch_pass(n);
    if (pass == 0) {
        return out;
    }
    const u32 touched_cap = src_n * k_band_stamp_per_src + 64u;
    u32* touched = new u32[touched_cap];
    if (touched == nullptr) {
        return out;
    }
    u32 touched_n = 0;
    for (u32 s = 0; s < src_n; ++s) {
        const u16 sx = src[s].x;
        const u16 sy = src[s].y;
        if (sx >= w || sy >= h) {
            continue;
        }
        const u32 si = tile_idx(w, sx, sy);
        if (!is_settle_land(cls[si]) || !is_own_ok(own_rows[si], player)) {
            continue;
        }
        stamp_band(w, h, cls, own_rows, blk_rows, player, sx, sy, min_dist, max_dist, pass, g_tag, touched,
            &touched_n, touched_cap);
    }
    collect_touched(w, cls, own_rows, blk_rows, player, pass, g_tag, touched, touched_n, &out);
    delete[] touched;
    return out;
}

SmSettlerBestPts SenseMakingSettlerPt::pick (
    MapTerrainData& map,
    const SmSettlerPtResult& cand,
    const SpgCoordPair& cap,
    const MapArrayDistance& l2w,
    const GameArraySimple* gmap)
{
    SmSettlerBestPts out = {};
    out.n = 0;
    for (u16 i = 0; i < SMS_BEST_N; ++i) {
        out.pts[i].x = U16_KEY_NULL;
        out.pts[i].y = U16_KEY_NULL;
    }
    if (map.data() == nullptr || map.width() == 0 || map.height() == 0 || cand.n == 0) {
        return out;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* cls = map.data();
    const CircArea geo = CircularTileAreas::get(3u);
    const u16 half = static_cast<u16>(CityBlockingMask::m_side / 2u);
    for (u16 k = 0; k < SMS_BEST_N; ++k) {
        i32 best_cost = 2147483647;
        u32 best_i = 0;
        bool has_best = false;
        for (u32 i = 0; i < cand.n; ++i) {
            const u16 px = cand.pts[i].x;
            const u16 py = cand.pts[i].y;
            if (px >= w || py >= h) {
                continue;
            }
            bool blocked = false;
            for (u16 j = 0; j < out.n; ++j) {
                const i32 dx = static_cast<i32>(px) - static_cast<i32>(out.pts[j].x);
                const i32 dy = static_cast<i32>(py) - static_cast<i32>(out.pts[j].y);
                if (gmap != nullptr) {
                    const u8* m = CityBlockingMask::preview(*gmap, out.pts[j].x, out.pts[j].y);
                    if (dx < -static_cast<i32>(half) || dy < -static_cast<i32>(half)
                        || dx > static_cast<i32>(half) || dy > static_cast<i32>(half)) {
                        continue;
                    }
                    const u32 mi = static_cast<u32>(dy + static_cast<i32>(half)) * static_cast<u32>(CityBlockingMask::m_side)
                        + static_cast<u32>(dx + static_cast<i32>(half));
                    if (m[mi] != 0) {
                        blocked = true;
                        break;
                    }
                } else {
                    for (u16 b = 0; b < geo.m_lim; ++b) {
                        if (static_cast<i32>(geo.m_brd[b][0]) == dx && static_cast<i32>(geo.m_brd[b][1]) == dy) {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked) {
                        break;
                    }
                }
            }
            if (blocked) {
                continue;
            }
            const i32 cost = pt_cost(w, h, cls, l2w, px, py, cap.x, cap.y);
            if (!has_best || cost < best_cost) {
                best_cost = cost;
                best_i = i;
                has_best = true;
            }
        }
        if (!has_best) {
            break;
        }
        out.pts[out.n].x = cand.pts[best_i].x;
        out.pts[out.n].y = cand.pts[best_i].y;
        out.n = static_cast<u16>(out.n + 1u);
    }
    return out;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
