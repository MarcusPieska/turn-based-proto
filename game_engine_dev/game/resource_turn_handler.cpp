//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "resource_turn_handler.h"

#include "assert_log.h"
#include "city_array.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "general_assessor.h"
#include "resource_assessor.h"
#include "resource_effector.h"
#include "resource_ledger.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

ResourceTurnHandler::ResCoord* ResourceTurnHandler::m_coord = nullptr;
u32 ResourceTurnHandler::m_coord_n = 0;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

u16 ResourceTurnHandler::extract_amt (const ResourceExtractCtx& ctx) {
    if (ctx.m_statics == nullptr) {
        return 0;
    }
    AssessorCtx actx = {};
    actx.m_tech = ctx.m_tech;
    actx.m_map = ctx.m_map;
    actx.m_x = ctx.m_x;
    actx.m_y = ctx.m_y;
    actx.m_city_idx = ctx.m_city_idx;
    actx.m_building_bank = ctx.m_bld_bank;
    actx.m_toggle_city_bank = ctx.m_flag_bank;
    if (!ResourceAssessor::can_extract(ctx.m_res_idx, actx, *ctx.m_statics)) {
        return 0;
    }
    return ResourceEffector::yield(ctx.m_res_idx, 1u, ctx);
}

//================================================================================================================================
//=> - ResourceTurnHandler -
//================================================================================================================================

bool ResourceTurnHandler::setup (GameState& state) {
    clear();
    if (state.m_statics == nullptr) {
        return false;
    }
    if (!ResourceEffector::setup(*state.m_statics)) {
        return false;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    u32 n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (state.m_map.get_res(x, y) != U16_KEY_NULL) {
                ++n;
            }
        }
    }
    if (n == 0) {
        return true;
    }
    m_coord = new ResCoord[n];
    if (m_coord == nullptr) {
        return false;
    }
    u32 wri = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 res = state.m_map.get_res(x, y);
            if (res == U16_KEY_NULL) {
                continue;
            }
            m_coord[wri].m_x = x;
            m_coord[wri].m_y = y;
            m_coord[wri].m_res = res;
            ++wri;
        }
    }
    m_coord_n = wri;
    return true;
}

void ResourceTurnHandler::clear () {
    delete[] m_coord;
    m_coord = nullptr;
    m_coord_n = 0;
    ResourceEffector::clear();
}

void ResourceTurnHandler::handle (GameState& state) {
    GAME_EXPECT(state.m_player_states != nullptr, "ResourceTurnHandler null player states");
    GAME_EXPECT(state.m_statics != nullptr, "ResourceTurnHandler null statics");
    GeneralBitBank* flags = state.m_cities.get_flag_bank();
    GeneralBitBank* blds = state.m_cities.get_bld_bank();
    for (u32 i = 0; i < m_coord_n; ++i) {
        const ResCoord& c = m_coord[i];
        const u8 owner = state.m_map.get_civ_owner(c.m_x, c.m_y);
        if (owner == U8_KEY_NULL || owner >= state.m_player_n) {
            continue;
        }
        PlayerState& ps = state.m_player_states[owner];
        ResourceExtractCtx ctx = {};
        ctx.m_owner = owner;
        ctx.m_x = c.m_x;
        ctx.m_y = c.m_y;
        ctx.m_res_idx = c.m_res;
        ctx.m_city_idx = U16_KEY_NULL;
        ctx.m_tile = state.m_map.tile(c.m_x, c.m_y);
        ctx.m_tech = ps.m_techs_researched;
        ctx.m_flag_bank = flags;
        ctx.m_bld_bank = blds;
        ctx.m_wonder_city = state.m_wonder_city;
        ctx.m_wonder_n = state.m_wonder_count;
        ctx.m_small_wonder_city = ps.m_small_wonder_city;
        ctx.m_small_wonder_n = state.m_small_wonder_count;
        ctx.m_statics = state.m_statics;
        ctx.m_map = &state.m_map;
        const u16 amt = extract_amt(ctx);
        if (amt == 0) {
            continue;
        }
        ps.m_res_ledger.add(c.m_res, amt);
    }
}

u32 ResourceTurnHandler::coord_n () {
    return m_coord_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
