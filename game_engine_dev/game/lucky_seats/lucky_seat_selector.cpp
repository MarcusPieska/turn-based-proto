//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_seat_selector.h"

#include <vector>

#include "continent_size_indexer.h"
#include "distribute_proportional.h"
#include "game_array_simple.h"
#include "land_potential.h"
#include "location_grouper.h"
#include "select_on_continent.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Tunables -
//================================================================================================================================

#define LUCKY_BLOCK_N 5

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 seat_of (const SpgPickCoords& starts, SpgCoordPair pt) {
    for (u32 i = 0; i < starts.n; ++i) {
        if (starts.pts[i].x == pt.x && starts.pts[i].y == pt.y) {
            return static_cast<u16>(i);
        }
    }
    return U16_KEY_NULL;
}

static bool already (const LuckySeats& lucky, u16 seat) {
    for (u16 i = 0; i < lucky.m_n; ++i) {
        if (lucky.m_seat[i] == seat) {
            return true;
        }
    }
    return false;
}

static bool is_blk (const u8* blk, u16 seat) {
    return blk != nullptr && seat < SPG_MAX_PICK_PTS && blk[seat] != 0u;
}

static bool add_seat (LuckySeats* out, u16 seat) {
    if (out == nullptr || seat == U16_KEY_NULL || already(*out, seat)) {
        return false;
    }
    if (out->m_n >= SPG_MAX_PICK_PTS) {
        return false;
    }
    out->m_seat[out->m_n] = seat;
    out->m_n = static_cast<u16>(out->m_n + 1u);
    return true;
}

static void block_near (const SpgPickCoords& starts, u16 seat, u8* blk) {
    const u16 n = static_cast<u16>(starts.n);
    if (blk == nullptr || seat >= n || LUCKY_BLOCK_N == 0) {
        return;
    }
    u32 dist[SPG_MAX_PICK_PTS];
    u16 idx[SPG_MAX_PICK_PTS];
    u16 en = 0u;
    const i32 ax = static_cast<i32>(starts.pts[seat].x);
    const i32 ay = static_cast<i32>(starts.pts[seat].y);
    for (u16 i = 0; i < n; ++i) {
        if (i == seat) {
            continue;
        }
        const i32 dx = static_cast<i32>(starts.pts[i].x) - ax;
        const i32 dy = static_cast<i32>(starts.pts[i].y) - ay;
        dist[en] = static_cast<u32>(dx * dx + dy * dy);
        idx[en] = i;
        ++en;
    }
    const u16 kn = (static_cast<u16>(LUCKY_BLOCK_N) < en) ? static_cast<u16>(LUCKY_BLOCK_N) : en;
    for (u16 k = 0; k < kn; ++k) {
        u16 best = k;
        for (u16 j = static_cast<u16>(k + 1u); j < en; ++j) {
            if (dist[j] < dist[best]) {
                best = j;
            }
        }
        const u32 td = dist[k];
        const u16 ti = idx[k];
        dist[k] = dist[best];
        idx[k] = idx[best];
        dist[best] = td;
        idx[best] = ti;
        blk[idx[k]] = 1u;
    }
}

static u16 rem_on (const LocGroup& g, const SpgPickCoords& starts, const LuckySeats& lucky, const u8* blk) {
    u16 rem = 0u;
    for (u16 i = 0; i < g.m_n; ++i) {
        const u16 seat = seat_of(starts, g.m_pts[i]);
        if (seat != U16_KEY_NULL && !already(lucky, seat) && !is_blk(blk, seat)) {
            ++rem;
        }
    }
    return rem;
}

static void mark_eligible (const ContSizeList& clist, u8* elig) {
    for (u16 i = 0; i < ContSizeList::k_cap; ++i) {
        elig[i] = 0u;
    }
    if (clist.m_n == 0u) {
        return;
    }
    const u32 largest = clist.m_e[0].m_tiles;
    const u32 min_tiles = (largest * static_cast<u32>(LuckySeatSelector::k_min_pct)) / 100u;
    for (u16 i = 0; i < clist.m_n; ++i) {
        if (i < LuckySeatSelector::k_top || clist.m_e[i].m_tiles >= min_tiles) {
            elig[i] = 1u;
        }
    }
}

//================================================================================================================================
//=> - LuckySeatSelector -
//================================================================================================================================

bool LuckySeatSelector::select (const GameArraySimple& map, const SpgPickCoords& starts, LuckySeats* out) {
    if (out == nullptr) {
        return false;
    }
    out->m_n = 0u;
    if (starts.n == 0u) {
        return true;
    }
    if (starts.n > SPG_MAX_PICK_PTS) {
        return false;
    }
    const u16 n = static_cast<u16>(starts.n);
    const u16 target = static_cast<u16>((static_cast<u32>(n) * static_cast<u32>(k_pct)) / 100u);
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u) {
        return false;
    }
    std::vector<i32> scores(n, 0);
    if (!LandPotential::score(map, starts.pts, n, scores.data())) {
        return false;
    }
    const u32 tn = map.tile_n();
    std::vector<u8> terr(tn);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            terr[i] = map.get_terrain(x, y);
        }
    }
    WhiteboardMng::init(w, h);
    Whiteboard_4B wb_rgb("LuckySeatSelector", "rgb", 0u);
    Whiteboard_2B wb_idx("LuckySeatSelector", "idx", 0u);
    if (!wb_rgb.ok() || !wb_idx.ok()) {
        WhiteboardMng::terminate();
        return false;
    }
    ContSizeList clist = {};
    if (!ContinentSizeIndexer::index(terr.data(), w, h, &clist, wb_rgb, wb_idx)) {
        WhiteboardMng::terminate();
        return false;
    }
    LocGroups groups;
    if (!LocationGrouper::group(starts.pts, scores.data(), n, wb_idx.get_iter_ptr(), w, h, clist.m_n, &groups)) {
        WhiteboardMng::terminate();
        return false;
    }
    u8 elig[ContSizeList::k_cap];
    mark_eligible(clist, elig);
    u8 blk[SPG_MAX_PICK_PTS] = {};
    for (u16 gi = 0; gi < groups.m_n; ++gi) {
        if (out->m_n >= target) {
            break;
        }
        if (gi >= ContSizeList::k_cap || elig[gi] == 0u) {
            continue;
        }
        const LocGroup& g = groups.m_g[gi];
        for (u16 i = 0; i < g.m_n; ++i) {
            const u16 seat = seat_of(starts, g.m_pts[i]);
            if (seat == U16_KEY_NULL || already(*out, seat) || is_blk(blk, seat)) {
                continue;
            }
            if (add_seat(out, seat)) {
                block_near(starts, seat, blk);
            }
            break;
        }
    }
    if (out->m_n < target) {
        const u16 quota = static_cast<u16>(target - out->m_n);
        DistPropEnt ents[ContSizeList::k_cap];
        u16 map_gi[ContSizeList::k_cap];
        u16 en = 0u;
        for (u16 gi = 0; gi < groups.m_n; ++gi) {
            if (gi >= ContSizeList::k_cap || elig[gi] == 0u) {
                continue;
            }
            const LocGroup& g = groups.m_g[gi];
            if (g.m_n == 0u) {
                continue;
            }
            const u16 rem = rem_on(g, starts, *out, blk);
            ents[en].m_tiles = clist.m_e[gi].m_tiles;
            ents[en].m_rem = rem;
            map_gi[en] = gi;
            ++en;
        }
        i32 counts[ContSizeList::k_cap];
        for (u16 i = 0; i < ContSizeList::k_cap; ++i) {
            counts[i] = 0;
        }
        if (!DistributeProportional::run(ents, en, quota, counts)) {
            WhiteboardMng::terminate();
            return false;
        }
        for (u16 ei = 0; ei < en; ++ei) {
            const u16 gi = map_gi[ei];
            const LocGroup& g = groups.m_g[gi];
            i32 need = counts[ei];
            while (need > 0 && out->m_n < target) {
                SpgCoordPair cands[SPG_MAX_PICK_PTS];
                i32 cscores[SPG_MAX_PICK_PTS];
                u16 cand_n = 0u;
                for (u16 i = 0; i < g.m_n; ++i) {
                    const u16 seat = seat_of(starts, g.m_pts[i]);
                    if (seat == U16_KEY_NULL || already(*out, seat) || is_blk(blk, seat)) {
                        continue;
                    }
                    cands[cand_n] = g.m_pts[i];
                    cscores[cand_n] = g.m_sc[i];
                    ++cand_n;
                }
                if (cand_n == 0u) {
                    break;
                }
                SpgCoordPair sels[SPG_MAX_PICK_PTS];
                u16 sel_n = 0u;
                for (u16 i = 0; i < g.m_n; ++i) {
                    const u16 seat = seat_of(starts, g.m_pts[i]);
                    if (seat == U16_KEY_NULL || !already(*out, seat)) {
                        continue;
                    }
                    sels[sel_n] = g.m_pts[i];
                    ++sel_n;
                }
                u16 pick_i = 0u;
                if (!SelectOnContinent::pick(cands, cscores, cand_n, sels, sel_n, &pick_i)) {
                    WhiteboardMng::terminate();
                    return false;
                }
                const u16 seat = seat_of(starts, cands[pick_i]);
                if (!add_seat(out, seat)) {
                    break;
                }
                block_near(starts, seat, blk);
                --need;
            }
        }
    }
    WhiteboardMng::terminate();
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
