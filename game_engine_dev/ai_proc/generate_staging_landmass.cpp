//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_staging_landmass.h"

#include "game_map_defs.h"
#include "game_state.h"
#include "log_dbg.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_wtr_tile (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]
        || t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool land_ok (const GameState& s, u16 x, u16 y) {
    const u8 t = s.m_map.get_terrain(x, y);
    return t != TERR_NONE[0] && t != TERR_MOUNTAINS[0] && !is_wtr_tile(t);
}

static bool own_free_open (const GameState& s, u16 seat, u16 x, u16 y) {
    const u8 own = s.m_map.get_civ_owner(x, y);
    return own == U8_KEY_NULL || own == static_cast<u8>(seat);
}

//================================================================================================================================
//=> - GenerateStagingLandmass -
//================================================================================================================================

bool GenerateStagingLandmass::generate (const GameState& s, u16 seat, Whiteboard_4B& out, u32* out_n) {
    PROF_TH_WAR_ENTER(__func__);
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 n = s.m_map.tile_n();
    if (w == 0 || h == 0 || !out.ok() || out_n == nullptr) {
        LOG_WAR_OWN_FREE_COMP_FAIL::LOG(SC_U32(seat), SC_U32(s.m_current_turn), 0u);
        return false;
    }
    if (out.w() != w || out.h() != h) {
        LOG_WAR_OWN_FREE_COMP_FAIL::LOG(SC_U32(seat), SC_U32(s.m_current_turn), 0u);
        return false;
    }
    u32* tc = out.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        tc[i] = k_none;
    }
    Whiteboard_4B que("GenerateStagingLandmass", "comp_q", 0u);
    if (!que.ok()) {
        LOG_WAR_OWN_FREE_COMP_FAIL::LOG(SC_U32(seat), SC_U32(s.m_current_turn), 1u);
        return false;
    }
    u32* q = que.get_iter_ptr();
    u32 next_id = 0;
    static const i32 k_dx[4] = {-1, 1, 0, 0};
    static const i32 k_dy[4] = {0, 0, -1, 1};
    for (u32 i = 0; i < n; ++i) {
        if (tc[i] != k_none) {
            continue;
        }
        const u16 x0 = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y0 = static_cast<u16>(i / static_cast<u32>(w));
        if (!land_ok(s, x0, y0) || !own_free_open(s, seat, x0, y0)) {
            continue;
        }
        const u32 cid = next_id++;
        u32 qn = 0;
        tc[i] = cid;
        q[qn++] = i;
        while (qn > 0) {
            const u32 cur = q[--qn];
            const u16 x = static_cast<u16>(cur % static_cast<u32>(w));
            const u16 y = static_cast<u16>(cur / static_cast<u32>(w));
            for (u16 k = 0; k < 4u; ++k) {
                const i32 nx = static_cast<i32>(x) + k_dx[k];
                const i32 ny = static_cast<i32>(y) + k_dy[k];
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                const u32 ni = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
                if (tc[ni] != k_none) {
                    continue;
                }
                if (!land_ok(s, ux, uy) || !own_free_open(s, seat, ux, uy)) {
                    continue;
                }
                tc[ni] = cid;
                q[qn++] = ni;
            }
        }
    }
    if (next_id == 0u) {
        LOG_WAR_OWN_FREE_COMP_FAIL::LOG(SC_U32(seat), SC_U32(s.m_current_turn), 2u);
        return false;
    }
    *out_n = next_id;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
