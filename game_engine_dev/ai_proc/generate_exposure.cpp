//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_exposure.h"

#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool on_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && x < static_cast<i32>(w) && y < static_cast<i32>(h);
}

static bool is_wtr (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]
        || t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool is_mtn (u8 t) {
    return t == TERR_MOUNTAINS[0];
}

static bool can_pass (const GameState& s, u16 x, u16 y) {
    const u8 t = s.m_map.get_terrain(x, y);
    return t != TERR_NONE[0] && !is_mtn(t) && !is_wtr(t);
}

static bool touches_enemy (const GameState& s, u16 w, u16 h, u16 x, u16 y, u8 enemy) {
    for (u16 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!on_map(w, h, nx, ny)) {
            continue;
        }
        if (s.m_map.get_civ_owner(static_cast<u16>(nx), static_cast<u16>(ny)) == enemy) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - GenerateExposure -
//================================================================================================================================

bool GenerateExposure::generate (const GameState& s, u16 seat, u16 enemy, Whiteboard_1B& out) {
    if (!out.ok() || s.m_player_states == nullptr) {
        return false;
    }
    if (seat >= s.m_player_n || enemy >= s.m_player_n || seat == enemy) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    if (out.w() != w || out.h() != h) {
        return false;
    }
    const u8 self = static_cast<u8>(seat);
    const u8 en = static_cast<u8>(enemy);
    const u32 n = s.m_map.tile_n();
    u8* dist = out.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_none;
    }
    u32* q = new u32[n];
    if (q == nullptr) {
        return false;
    }
    u32 qn = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (s.m_map.get_civ_owner(x, y) != self || !can_pass(s, x, y)) {
                continue;
            }
            if (!touches_enemy(s, w, h, x, y, en)) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            dist[i] = 0u;
            q[qn++] = i;
        }
    }
    u32 head = 0;
    while (head < qn) {
        const u32 i = q[head++];
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u8 d = dist[i];
        if (d == k_none || d == 254u) {
            continue;
        }
        const u8 nd = static_cast<u8>(d + 1u);
        for (u16 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(x) + k_dx4[k];
            const i32 ny = static_cast<i32>(y) + k_dy4[k];
            if (!on_map(w, h, nx, ny)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (s.m_map.get_civ_owner(ux, uy) != self || !can_pass(s, ux, uy)) {
                continue;
            }
            const u32 ni = tidx(w, ux, uy);
            if (dist[ni] != k_none) {
                continue;
            }
            dist[ni] = nd;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
