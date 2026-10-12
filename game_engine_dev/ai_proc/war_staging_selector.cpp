//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_staging_selector.h"

#include "city.h"
#include "game_state.h"
#include "generate_staging_landmass.h"
#include "log_dbg.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - WarStagingSelector -
//================================================================================================================================

bool WarStagingSelector::pick (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy) {
    PROF_TH_WAR_ENTER(__func__);
    if (ox == nullptr || oy == nullptr || s.m_player_states == nullptr) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 0u);
        return false;
    }
    if (seat >= s.m_player_n || enemy >= s.m_player_n || seat == enemy) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 0u);
        return false;
    }
    const u16 cn = s.m_cities.get_city_count();
    if (cn == 0u) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 1u);
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 tn = s.m_map.tile_n();
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 2u);
        return false;
    }
    Whiteboard_4B tile_comp("WarStagingSelector", "tile_comp", 0u);
    if (!tile_comp.ok()) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 2u);
        return false;
    }
    u32 comp_n = 0;
    if (!GenerateStagingLandmass::generate(s, seat, tile_comp, &comp_n)) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 3u);
        return false;
    }
    const u32* tc = tile_comp.get_iter_ptr();
    if (comp_n > tn) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 2u);
        return false;
    }
    Whiteboard_4B city_n_wb("WarStagingSelector", "city_n", 0u);
    if (!city_n_wb.ok()) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 2u);
        return false;
    }
    u32* city_n = city_n_wb.get_iter_ptr();
    for (u32 i = 0; i < comp_n; ++i) {
        city_n[i] = 0;
    }
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u32 ti = static_cast<u32>(c->get_y()) * static_cast<u32>(w) + static_cast<u32>(c->get_x());
        if (ti >= tn) {
            continue;
        }
        const u32 cid = tc[ti];
        if (cid < comp_n) {
            city_n[cid]++;
        }
    }
    u32 best_comp = GenerateStagingLandmass::k_none;
    u32 best_cn = 0;
    for (u32 i = 0; i < comp_n; ++i) {
        if (city_n[i] > best_cn) {
            best_cn = city_n[i];
            best_comp = i;
        }
    }
    if (best_comp == GenerateStagingLandmass::k_none || best_cn == 0u) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 4u);
        return false;
    }
    u32 best_sc = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u16 sx = c->get_x();
        const u16 sy = c->get_y();
        const u32 ti = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
        if (ti >= tn || tc[ti] != best_comp) {
            continue;
        }
        u32 min_d = 0xFFFFFFFFu;
        u64 esx = 0;
        u64 esy = 0;
        u32 en = 0;
        for (u16 j = 0; j < cn; ++j) {
            const City* e = s.m_cities.get_city(j);
            if (e == nullptr || e->get_owner() != enemy) {
                continue;
            }
            const u16 ex = e->get_x();
            const u16 ey = e->get_y();
            esx += ex;
            esy += ey;
            en++;
            const u32 adx = sx > ex ? static_cast<u32>(sx - ex) : static_cast<u32>(ex - sx);
            const u32 ady = sy > ey ? static_cast<u32>(sy - ey) : static_cast<u32>(ey - sy);
            const u32 d = adx + ady;
            if (d < min_d) {
                min_d = d;
            }
        }
        if (en == 0u || min_d == 0xFFFFFFFFu) {
            continue;
        }
        const u16 ecx = static_cast<u16>(esx / en);
        const u16 ecy = static_cast<u16>(esy / en);
        const u32 cdx = sx > ecx ? static_cast<u32>(sx - ecx) : static_cast<u32>(ecx - sx);
        const u32 cdy = sy > ecy ? static_cast<u32>(sy - ecy) : static_cast<u32>(ecy - sy);
        const u32 sc = min_d * 64u + cdx + cdy;
        if (!found || sc < best_sc) {
            best_sc = sc;
            bx = sx;
            by = sy;
            found = true;
        }
    }
    if (!found) {
        LOG_WAR_STAGING_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(s.m_current_turn), 5u);
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
