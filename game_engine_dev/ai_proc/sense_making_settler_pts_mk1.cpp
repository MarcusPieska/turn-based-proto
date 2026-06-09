//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sense_making_settler_pts.h"

#include "../simple_map_gen/generator_constants.h"

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

static bool tile_ok (u8 cls, u8 own, u16 player) {
    return is_settle_land(cls) && is_own_ok(own, player);
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

static void expand_nbr (
    u16 w,
    u16 h,
    const u8* cls,
    const u8* own,
    u16 player,
    u16* dist,
    u32* q,
    u32* qt,
    u32 i,
    u16 cur,
    u16 max_dist) 
{
    if (cur >= max_dist) {
        return;
    }
    const u16 nxt = static_cast<u16>(cur + 1u);
    const u16 py = static_cast<u16>(i / static_cast<u32>(w));
    const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
    if (px > 0) {
        const u32 j = i - 1u;
        if (tile_ok(cls[j], own[j], player) && dist[j] == k_u16_inf) {
            dist[j] = nxt;
            q[*qt] = j;
            *qt = *qt + 1u;
        }
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
        const u32 j = i + 1u;
        if (tile_ok(cls[j], own[j], player) && dist[j] == k_u16_inf) {
            dist[j] = nxt;
            q[*qt] = j;
            *qt = *qt + 1u;
        }
    }
    if (py > 0) {
        const u32 j = i - static_cast<u32>(w);
        if (tile_ok(cls[j], own[j], player) && dist[j] == k_u16_inf) {
            dist[j] = nxt;
            q[*qt] = j;
            *qt = *qt + 1u;
        }
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
        const u32 j = i + static_cast<u32>(w);
        if (tile_ok(cls[j], own[j], player) && dist[j] == k_u16_inf) {
            dist[j] = nxt;
            q[*qt] = j;
            *qt = *qt + 1u;
        }
    }
}

//================================================================================================================================
//=> - SenseMakingSettlerPt -
//================================================================================================================================

SmSettlerPtResult SenseMakingSettlerPt::select (
    const MapTerrainData& map,
    const MapTerrainData& own,
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
    if (min_dist > max_dist || src == nullptr || src_n == 0) {
        return out;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* cls = map.data();
    const u8* own_rows = own.data();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    u32* q = new u32[n];
    u32 qh = 0;
    u32 qt = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_u16_inf;
    }
    for (u32 s = 0; s < src_n; ++s) {
        const u16 px = src[s].x;
        const u16 py = src[s].y;
        if (px >= w || py >= h) {
            continue;
        }
        const u32 i = tile_idx(w, px, py);
        if (!tile_ok(cls[i], own_rows[i], player)) {
            continue;
        }
        if (dist[i] != k_u16_inf) {
            continue;
        }
        dist[i] = 0;
        q[qt++] = i;
    }
    while (qh < qt) {
        const u32 i = q[qh++];
        const u16 cur = dist[i];
        if (cur > max_dist) {
            break;
        }
        if (cur >= min_dist) {
            const u16 py = static_cast<u16>(i / static_cast<u32>(w));
            const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
            push_pt(&out, px, py);
        }
        expand_nbr(w, h, cls, own_rows, player, dist, q, &qt, i, cur, max_dist);
    }
    delete[] dist;
    delete[] q;
    return out;
}

SmSettlerPt SenseMakingSettlerPt::pick (
    MapTerrainData& map,
    const SmSettlerPtResult& cand,
    const SpgCoordPair& cap,
    const MapArrayDistance& l2w) 
{
    SmSettlerPt out = {};
    out.x = U16_KEY_NULL;
    out.y = U16_KEY_NULL;
    if (map.data() == nullptr || map.width() == 0 || map.height() == 0) {
        return out;
    }
    if (cand.n == 0) {
        return out;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u8* cls = map.data();
    i32 best_cost = 2147483647;
    u32 best_i = 0;
    bool has_best = false;
    for (u32 i = 0; i < cand.n; ++i) {
        const u16 px = cand.pts[i].x;
        const u16 py = cand.pts[i].y;
        if (px >= w || py >= h) {
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
        return out;
    }
    out.x = cand.pts[best_i].x;
    out.y = cand.pts[best_i].y;
    return out;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

