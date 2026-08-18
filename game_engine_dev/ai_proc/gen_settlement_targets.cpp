//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_settlement_targets.h"

#include <cstring>
#include <vector>

#include "city_blocking_mask.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "settle_rules.h"

//================================================================================================================================
//=> - Local -
//================================================================================================================================

static const i8 k_dx4[4] = {0, 1, 0, -1};
static const i8 k_dy4[4] = {-1, 0, 1, 0};

static bool is_land (const GameArraySimple& map, u16 x, u16 y) {
    return !overlay_is_water_terr(map.get_terrain(x, y));
}

static bool can_place (const GameArraySimple& map, u16 x, u16 y) {
    if (!SettleRules::tile_ok(map, x, y)) {
        return false;
    }
    if (map.get_settler_blocked(x, y) != 0) {
        return false;
    }
    if (map.get_planned_city(x, y) != 0) {
        return false;
    }
    return true;
}

static void place (GameArraySimple& map, u16 x, u16 y, GenSettlementTargetsRslt* out, u32* bucket) {
    map.set_planned_city(x, y, 1u);
    CityBlockingMask::stamp(map, x, y);
    ++out->m_n;
    ++(*bucket);
}

static void clr_marks (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            map.set_settler_blocked(x, y, 0u);
            map.set_planned_city(x, y, 0u);
        }
    }
}

static void seed_starts (GameArraySimple& map, const SpgCoordPair* starts, u32 start_n, GenSettlementTargetsRslt* out) {
    for (u32 i = 0; i < start_n; ++i) {
        const u16 x = starts[i].x;
        const u16 y = starts[i].y;
        if (x >= map.width() || y >= map.height()) {
            continue;
        }
        if (!can_place(map, x, y)) {
            continue;
        }
        place(map, x, y, out, &out->m_seed_n);
    }
}

static u32 tix (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static void river_from_seed (GameArraySimple& map, u16 sx, u16 sy, std::vector<u8>& seen, GenSettlementTargetsRslt* out) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u32> q;
    q.reserve(1024);
    auto push = [&](u16 x, u16 y) {
        const u32 i = tix(w, x, y);
        if (seen[i] != 0u) {
            return;
        }
        seen[i] = 1u;
        q.push_back(i);
    };
    push(sx, sy);
    u32 head = 0;
    u16 riv_x = sx;
    u16 riv_y = sy;
    bool hit_riv = map.get_river(sx, sy) != 0;
    while (head < q.size() && !hit_riv) {
        const u32 i = q[head++];
        const u16 x = static_cast<u16>(i % w);
        const u16 y = static_cast<u16>(i / w);
        for (u32 k = 0; k < 4u; ++k) {
            const int nx = static_cast<int>(x) + k_dx4[k];
            const int ny = static_cast<int>(y) + k_dy4[k];
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(w) || ny >= static_cast<int>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!is_land(map, ux, uy)) {
                continue;
            }
            push(ux, uy);
            if (map.get_river(ux, uy) != 0) {
                riv_x = ux;
                riv_y = uy;
                hit_riv = true;
                break;
            }
        }
    }
    if (!hit_riv) {
        return;
    }
    std::vector<u8> rseen(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    std::vector<u32> rq;
    rq.push_back(tix(w, riv_x, riv_y));
    rseen[tix(w, riv_x, riv_y)] = 1u;
    u32 rh = 0;
    while (rh < rq.size()) {
        const u32 i = rq[rh++];
        const u16 x = static_cast<u16>(i % w);
        const u16 y = static_cast<u16>(i / w);
        if (can_place(map, x, y)) {
            place(map, x, y, out, &out->m_river_n);
        }
        for (u32 k = 0; k < 4u; ++k) {
            const int nx = static_cast<int>(x) + k_dx4[k];
            const int ny = static_cast<int>(y) + k_dy4[k];
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(w) || ny >= static_cast<int>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = tix(w, ux, uy);
            if (rseen[ni] != 0u) {
                continue;
            }
            if (map.get_river(ux, uy) == 0) {
                continue;
            }
            rseen[ni] = 1u;
            rq.push_back(ni);
        }
    }
}

static void river_phase (GameArraySimple& map, const SpgCoordPair* starts, u32 start_n, GenSettlementTargetsRslt* out) {
    const u32 tiles = map.tile_n();
    std::vector<u8> seen(tiles, 0u);
    for (u32 i = 0; i < start_n; ++i) {
        const u16 x = starts[i].x;
        const u16 y = starts[i].y;
        if (x >= map.width() || y >= map.height()) {
            continue;
        }
        std::fill(seen.begin(), seen.end(), 0u);
        river_from_seed(map, x, y, seen, out);
    }
}

static void pack_phase (GameArraySimple& map, GenSettlementTargetsRslt* out) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!can_place(map, x, y)) {
                continue;
            }
            place(map, x, y, out, &out->m_pack_n);
        }
    }
}

//================================================================================================================================
//=> - GenSettlementTargets -
//================================================================================================================================

bool GenSettlementTargets::generate (
    GameArraySimple& map,
    const SpgCoordPair* starts,
    u32 start_n,
    GenSettlementTargetsRslt* out)
{
    if (map.width() == 0 || map.height() == 0) {
        return false;
    }
    if (starts == nullptr && start_n != 0u) {
        return false;
    }
    GenSettlementTargetsRslt local = {};
    GenSettlementTargetsRslt* r = out != nullptr ? out : &local;
    *r = {};
    clr_marks(map);
    seed_starts(map, starts, start_n, r);
    river_phase(map, starts, start_n, r);
    pack_phase(map, r);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
