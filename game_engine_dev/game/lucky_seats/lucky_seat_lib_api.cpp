//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_seat_api.h"

#include "game_array_simple.h"
#include "lucky_resource_booster.h"
#include "lucky_seat_selector.h"
#include "runtime_statics.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static LuckySeatRslt fail_rslt () {
    LuckySeatRslt r = {};
    r.m_ok = false;
    return r;
}

//================================================================================================================================
//=> - lucky_seat_run -
//================================================================================================================================

LuckySeatRslt lucky_seat_run (LuckySeatReq* req) {
    if (req == nullptr || req->m_map == nullptr || req->m_statics == nullptr) {
        return fail_rslt();
    }
    if (req->m_starts == nullptr || req->m_start_n == 0u) {
        return fail_rslt();
    }
    if (req->m_lucky_seats == nullptr || req->m_lucky_cap == 0u) {
        return fail_rslt();
    }
    if (req->m_do_select == 0u && req->m_do_boost == 0u) {
        return fail_rslt();
    }
    if (!TileYields::setup(*req->m_statics)) {
        return fail_rslt();
    }
    LuckySeatRslt out = {};
    out.m_ok = false;
    out.m_local_n = 0u;
    out.m_river_n = 0u;
    if (req->m_do_select != 0u) {
        SpgPickCoords starts = {};
        if (req->m_start_n > SPG_MAX_PICK_PTS) {
            return fail_rslt();
        }
        starts.n = req->m_start_n;
        for (u16 i = 0u; i < req->m_start_n; ++i) {
            starts.pts[i] = req->m_starts[i];
        }
        LuckySeats lucky = {};
        if (!LuckySeatSelector::select(*req->m_map, starts, &lucky)) {
            return fail_rslt();
        }
        if (lucky.m_n > req->m_lucky_cap) {
            return fail_rslt();
        }
        for (u16 i = 0u; i < lucky.m_n; ++i) {
            req->m_lucky_seats[i] = lucky.m_seat[i];
        }
        req->m_lucky_n = lucky.m_n;
    }
    if (req->m_do_boost != 0u) {
        if (req->m_lucky_n > req->m_lucky_cap || req->m_lucky_n > SPG_MAX_PICK_PTS) {
            return fail_rslt();
        }
        SpgCoordPair pts[SPG_MAX_PICK_PTS];
        for (u16 i = 0u; i < req->m_lucky_n; ++i) {
            const u16 s = req->m_lucky_seats[i];
            if (s >= req->m_start_n) {
                return fail_rslt();
            }
            pts[i] = req->m_starts[s];
        }
        u32 local_n = 0u;
        u32 river_n = 0u;
        if (!LuckyResourceBooster::boost_local(*req->m_map, pts, req->m_lucky_n, &local_n)) {
            return fail_rslt();
        }
        if (!LuckyResourceBooster::boost_river(*req->m_map, *req->m_statics, pts, req->m_lucky_n, &river_n)) {
            return fail_rslt();
        }
        out.m_local_n = local_n;
        out.m_river_n = river_n;
    }
    out.m_ok = true;
    return out;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
