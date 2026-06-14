//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <map>
#include <utility>
#include <vector>

#include "generate_watershed_mtn_lines.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static u16* alloc_ov (u16 w, u16 h) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* ov = new u16[n];
    if (ov == nullptr) {
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        ov[i] = 0;
    }
    return ov;
}

static u16 basin_lo (u16 a, u16 b) {
    return (a < b) ? a : b;
}

static u16 basin_hi (u16 a, u16 b) {
    return (a < b) ? b : a;
}

static u32 mouth_dist_sq (u16 a, u16 b, const u16* mx, const u16* my, const u8* has, u16 cap) {
    if (a == 0 || b == 0 || a >= cap || b >= cap || has[a] == 0 || has[b] == 0) {
        return 0;
    }
    const i32 dx = static_cast<i32>(mx[a]) - static_cast<i32>(mx[b]);
    const i32 dy = static_cast<i32>(my[a]) - static_cast<i32>(my[b]);
    return static_cast<u32>(dx * dx + dy * dy);
}

static u16 max_basin_idx (const RiverNetworkResult* network) {
    u16 mx = 0;
    for (u16 bi = 0; bi < network->basin_n; ++bi) {
        if (network->basins[bi].idx > mx) {
            mx = network->basins[bi].idx;
        }
    }
    return mx;
}

static bool build_mouth_tbl (
    const RiverNetworkResult* network,
    u16 cap,
    u16* mx,
    u16* my,
    u8* has) 
{
    for (u16 i = 0; i <= cap; ++i) {
        has[i] = 0;
    }
    for (u16 bi = 0; bi < network->basin_n; ++bi) {
        const RiverBasinEntry& e = network->basins[bi];
        if (e.idx == 0 || e.idx > cap) {
            continue;
        }
        has[e.idx] = 1;
        mx[e.idx] = e.mouth_x;
        my[e.idx] = e.mouth_y;
    }
    return true;
}

static bool is_plains (u8 cls) {
    return cls == TERR_PLAINS[0];
}

static bool is_hills (u8 cls) {
    return cls == TERR_HILLS[0];
}

static void count_side_terr (u8 cls, u16* pln, u16* hil) {
    if (is_plains(cls)) {
        (*pln)++;
    } else if (is_hills(cls)) {
        (*hil)++;
    }
}

static u16 pick_nb (
    u16 a,
    u16 w,
    u16 h,
    u32 ti,
    const u16* bov,
    const u16* mx,
    const u16* my,
    const u8* has,
    u16 cap) 
{
    const u16 px = static_cast<u16>(ti % static_cast<u32>(w));
    const u16 py = static_cast<u16>(ti / static_cast<u32>(w));
    u16 best = 0;
    u32 best_d = 0;
    if (px > 0) {
        const u16 b = bov[ti - 1u];
        if (b != a && b != static_cast<u16>(RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    if (px + 1u < w) {
        const u16 b = bov[ti + 1u];
        if (b != a && b != static_cast<u16>(RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    if (py > 0) {
        const u16 b = bov[ti - static_cast<u32>(w)];
        if (b != a && b != static_cast<u16>(RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    if (py + 1u < h) {
        const u16 b = bov[ti + static_cast<u32>(w)];
        if (b != a && b != static_cast<u16>(RIVER_BASIN_NONE)) {
            const u32 d = mouth_dist_sq(a, b, mx, my, has, cap);
            if (d > best_d) {
                best_d = d;
                best = b;
            }
        }
    }
    return best;
}

//================================================================================================================================
//=> - Generate_WatershedMtnLines -
//================================================================================================================================

WatershedMtnLinesResult* Generate_WatershedMtnLines::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverNetworkResult* network) 
{
    if (terrain == nullptr || w == 0 || h == 0 || network == nullptr || network->overlay == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16* bov = network->overlay;
    const u16 cap = max_basin_idx(network);
    if (cap == 0) {
        WatershedMtnLinesResult* out = new WatershedMtnLinesResult();
        if (out == nullptr) {
            return nullptr;
        }
        out->w = w;
        out->h = h;
        out->seg_n = 0;
        out->border_tile_n = 0;
        out->segs = nullptr;
        out->overlay = alloc_ov(w, h);
        if (out->overlay == nullptr) {
            delete out;
            return nullptr;
        }
        return out;
    }
    std::vector<u16> mx(static_cast<size_t>(cap) + 1u, 0);
    std::vector<u16> my(static_cast<size_t>(cap) + 1u, 0);
    std::vector<u8> has(static_cast<size_t>(cap) + 1u, 0);
    build_mouth_tbl(network, cap, mx.data(), my.data(), has.data());
    std::map<std::pair<u16, u16>, std::vector<u32>> pair_tiles;
    for (u32 ti = 0; ti < n; ++ti) {
        const u16 a = bov[ti];
        if (a == static_cast<u16>(RIVER_BASIN_NONE)) {
            continue;
        }
        const u16 nb = pick_nb(a, w, h, ti, bov, mx.data(), my.data(), has.data(), cap);
        if (nb == 0) {
            continue;
        }
        const u16 lo = basin_lo(a, nb);
        const u16 hi = basin_hi(a, nb);
        pair_tiles[std::make_pair(lo, hi)].push_back(ti);
    }
    if (pair_tiles.empty()) {
        WatershedMtnLinesResult* out = new WatershedMtnLinesResult();
        if (out == nullptr) {
            return nullptr;
        }
        out->w = w;
        out->h = h;
        out->seg_n = 0;
        out->border_tile_n = 0;
        out->segs = nullptr;
        out->overlay = alloc_ov(w, h);
        if (out->overlay == nullptr) {
            delete out;
            return nullptr;
        }
        return out;
    }
    std::vector<WatershedBorderSeg> recs;
    recs.reserve(pair_tiles.size());
    u16 next_ov = 1;
    for (const auto& kv : pair_tiles) {
        const u16 lo = kv.first.first;
        const u16 hi = kv.first.second;
        WatershedBorderSeg s = {};
        s.basin_a = lo;
        s.basin_b = hi;
        s.mouth_ax = mx[lo];
        s.mouth_ay = my[lo];
        s.mouth_bx = mx[hi];
        s.mouth_by = my[hi];
        s.mouth_dist = mouth_dist_sq(lo, hi, mx.data(), my.data(), has.data(), cap);
        s.ov_idx = next_ov++;
        s.tile_n = 0;
        s.a_plains = 0;
        s.a_hills = 0;
        s.b_plains = 0;
        s.b_hills = 0;
        for (u32 ti : kv.second) {
            s.tile_n++;
            const u16 bid = bov[ti];
            const u8 t = terrain[ti];
            if (bid == lo) {
                count_side_terr(t, &s.a_plains, &s.a_hills);
            } else if (bid == hi) {
                count_side_terr(t, &s.b_plains, &s.b_hills);
            }
        }
        recs.push_back(s);
    }
    std::sort(recs.begin(), recs.end(), [](const WatershedBorderSeg& a, const WatershedBorderSeg& b) {
        if (a.mouth_dist != b.mouth_dist) {
            return a.mouth_dist < b.mouth_dist;
        }
        if (a.basin_a != b.basin_a) {
            return a.basin_a < b.basin_a;
        }
        return a.basin_b < b.basin_b;
    });
    WatershedMtnLinesResult* out = new WatershedMtnLinesResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->seg_n = static_cast<u16>(recs.size());
    out->border_tile_n = 0;
    out->segs = new WatershedBorderSeg[out->seg_n];
    out->overlay = alloc_ov(w, h);
    if (out->segs == nullptr || out->overlay == nullptr) {
        delete[] out->segs;
        delete[] out->overlay;
        delete out;
        return nullptr;
    }
    for (u16 si = 0; si < out->seg_n; ++si) {
        out->segs[si] = recs[si];
        out->border_tile_n += out->segs[si].tile_n;
    }
    for (const auto& kv : pair_tiles) {
        const u16 lo = kv.first.first;
        const u16 hi = kv.first.second;
        u16 oid = 0;
        for (u16 si = 0; si < out->seg_n; ++si) {
            if (out->segs[si].basin_a == lo && out->segs[si].basin_b == hi) {
                oid = out->segs[si].ov_idx;
                break;
            }
        }
        if (oid == 0) {
            continue;
        }
        for (u32 ti : kv.second) {
            out->overlay[ti] = oid;
        }
    }
    return out;
}

void Generate_WatershedMtnLines::free_result (WatershedMtnLinesResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->segs;
    delete[] res->overlay;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
