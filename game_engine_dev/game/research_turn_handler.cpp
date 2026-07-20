//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "research_turn_handler.h"
#include "assert_log.h"
#include "bit_array.h"
#include "game_state.h"
#include "general_assessor.h"
#include "linear_tech.h"
#include "runtime_statics.h"
#include "tech_static_data.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void clr_owned (BitArrayCL& available, const BitArrayCL& owned) {
    const u32 n = available.get_count();
    for (u32 i = 0; i < n; ++i) {
        if (owned.get_bit(i) != 0) {
            available.clear_bit(i);
        }
    }
}

static void pick_target (PlayerState& ps, const RuntimeStatics& st) {
    if (ps.m_current_research_target_idx != U16_KEY_NULL) {
        return;
    }
    const u16 tech_n = st.tech().get_item_count();
    GAME_EXPECT(tech_n != 0, "ResearchTurnHandler pick_target tech count");
    if (tech_n == 0) {
        return;
    }
    if (ps.m_techs_researched == nullptr) {
        ps.m_techs_researched = new BitArrayCL(tech_n);
    }
    GAME_EXPECT(ps.m_techs_researched != nullptr, "ResearchTurnHandler pick_target techs null");
    if (ps.m_techs_researched == nullptr) {
        return;
    }
    BitArrayCL resource(st.resource().get_item_count());
    BitArrayCL building(st.building().get_item_count());
    BitArrayCL city_flag(st.city_flag().get_item_count());
    BitArrayCL civ(st.civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    AssessorCtx ctx = {};
    ctx.m_tech = ps.m_techs_researched;
    ctx.m_civ = &civ;
    ctx.m_city_idx = 0;
    ctx.m_resource_bank = nullptr;
    ctx.m_building_bank = nullptr;
    ctx.m_city_flag_bank = nullptr;
    ctx.m_resource = &resource;
    ctx.m_building = &building;
    ctx.m_city_flag = &city_flag;
    BitArrayCL available(tech_n);
    const TechStaticDataStruct* items = &st.tech().get_item(TechStaticDataKey::from_raw(0));
    GeneralAssessor::assess_tech(&available, tech_n, items, ctx);
    clr_owned(available, *ps.m_techs_researched);
    u16 pick = U16_KEY_NULL;
    if (!LinearTech::pick(available, &pick)) {
        return;
    }
    ps.m_current_research_target_idx = pick;
}

static void bank_commerce (PlayerState& ps) {
    u16 perc = ps.m_research_spending_perc;
    if (perc > 100u) {
        perc = 100u;
    }
    const u32 from = ps.m_commerce_from_turn;
    const u32 to_research = (from * static_cast<u32>(perc)) / 100u;
    const u32 to_commerce = from - to_research;
    ps.m_research = ps.m_research + to_research;
    ps.m_commerce = ps.m_commerce + to_commerce;
    ps.m_commerce_from_turn = 0;
}

static void finish_ready (PlayerState& ps, const RuntimeStatics& st) {
    GAME_EXPECT(ps.m_techs_researched != nullptr, "ResearchTurnHandler finish_ready techs null");
    if (ps.m_techs_researched == nullptr) {
        return;
    }
    const u16 tech_n = st.tech().get_item_count();
    while (true) {
        if (ps.m_current_research_target_idx == U16_KEY_NULL) {
            pick_target(ps, st);
        }
        const u16 tgt = ps.m_current_research_target_idx;
        if (tgt == U16_KEY_NULL || tgt >= tech_n) {
            return;
        }
        const u32 cost = st.tech().get_item(TechStaticDataKey::from_raw(tgt)).cost;
        if (ps.m_research < cost) {
            return;
        }
        ps.m_research = ps.m_research - cost;
        ps.m_techs_researched->set_bit(tgt);
        ps.m_current_research_target_idx = U16_KEY_NULL;
    }
}

//================================================================================================================================
//=> - ResearchTurnHandler -
//================================================================================================================================

void ResearchTurnHandler::begin (GameState& state) {
    GAME_EXPECT(state.m_player_states != nullptr, "ResearchTurnHandler begin got nullptr player states");
    GAME_EXPECT(state.m_statics != nullptr, "ResearchTurnHandler begin got nullptr statics");
    if (state.m_player_states == nullptr || state.m_statics == nullptr) {
        return;
    }
    for (u16 p = 0; p < state.m_player_n; ++p) {
        pick_target(state.m_player_states[p], *state.m_statics);
    }
}

void ResearchTurnHandler::handle (GameState& state, u16 player) {
    GAME_EXPECT(state.m_player_states != nullptr, "ResearchTurnHandler handle got nullptr player states");
    GAME_EXPECT(state.m_statics != nullptr, "ResearchTurnHandler handle got nullptr statics");
    GAME_EXPECT(player < state.m_player_n, "ResearchTurnHandler handle player out of bounds");
    if (state.m_player_states == nullptr || state.m_statics == nullptr || player >= state.m_player_n) {
        return;
    }
    PlayerState& ps = state.m_player_states[player];
    bank_commerce(ps);
    finish_ready(ps, *state.m_statics);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
