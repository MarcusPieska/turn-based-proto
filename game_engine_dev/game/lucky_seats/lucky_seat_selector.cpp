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

static u16 rem_on (const LocGroup& g, const SpgPickCoords& starts, const LuckySeats& lucky) {
    u16 rem = 0u;
    for (u16 i = 0; i < g.m_n; ++i) {
        const u16 seat = seat_of(starts, g.m_pts[i]);
        if (seat != U16_KEY_NULL && !already(lucky, seat)) {
            ++rem;
        }
    }
    return rem;
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
    for (u16 gi = 0; gi < groups.m_n; ++gi) {
        if (out->m_n >= target) {
            break;
        }
        const LocGroup& g = groups.m_g[gi];
        if (g.m_n == 0u) {
            continue;
        }
        const u16 seat = seat_of(starts, g.m_pts[0]);
        if (seat == U16_KEY_NULL) {
            continue;
        }
        (void)add_seat(out, seat);
    }
    if (out->m_n < target) {
        const u16 quota = static_cast<u16>(target - out->m_n);
        DistPropEnt ents[ContSizeList::k_cap];
        u16 map_gi[ContSizeList::k_cap];
        u16 en = 0u;
        for (u16 gi = 0; gi < groups.m_n; ++gi) {
            const LocGroup& g = groups.m_g[gi];
            if (g.m_n == 0u) {
                continue;
            }
            const u16 rem = rem_on(g, starts, *out);
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
                    if (seat == U16_KEY_NULL || already(*out, seat)) {
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
