//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "job_target_manager.h"

#include "assert_log.h"
#include "city.h"
#include "city_tile_manager.h"
#include "circular_tile_areas.h"
#include "dyn_job_slot_register.h"
#include "dyn_job_yield_register.h"
#include "effect_ctx.h"
#include "game_state.h"
#include "job_target_commerce_then_preference.h"
#include "job_target_preference_only.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_reach_r = 4;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static EffectCtx make_ctx (GameState& state, u16 city_idx, u16 owner) {
    EffectCtx ctx = {};
    ctx.m_owner = owner;
    ctx.m_city_idx = city_idx;
    if (state.m_player_states != nullptr && owner < state.m_player_n) {
        const PlayerState& ps = state.m_player_states[owner];
        ctx.m_tech = ps.m_techs_researched;
        ctx.m_small_wonder_city = ps.m_small_wonder_city;
        if (state.m_statics != nullptr) {
            effect_ctx_bind_civ_traits(ctx, *state.m_statics, ps.m_civ_index);
        }
    }
    ctx.m_bld_bank = state.m_cities.get_bld_bank();
    ctx.m_wonder_city = state.m_wonder_city;
    ctx.m_wonder_n = state.m_wonder_count;
    ctx.m_small_wonder_n = state.m_small_wonder_count;
    return ctx;
}

static u16 leftover_pops (GameState& state, City* city, u16 city_idx) {
    const CircArea area = CityTileManager::work_area();
    const u32 worked = state.m_map.count_worked(city->get_x(), city->get_y(), city_idx, area.m_brd, area.m_lim, k_reach_r);
    const u16 pop = city->get_current_population();
    if (worked >= pop) {
        return 0;
    }
    return static_cast<u16>(pop - worked);
}

//================================================================================================================================
//=> - JobTargetManager -
//================================================================================================================================

DynJobYieldPack JobTargetManager::fill (GameState& state, u16 city_idx, u16 trait_idx) {
    const DynJobYieldPack empty = {};
    City* city = state.m_cities.get_city(city_idx);
    GAME_EXPECT_RET(city != nullptr, empty, "JobTargetManager city");
    GAME_EXPECT_RET(state.m_statics != nullptr, empty, "JobTargetManager statics");
    GAME_EXPECT_RET(state.m_player_states != nullptr, empty, "JobTargetManager players");
    const u16 owner = city->get_owner();
    GAME_EXPECT_RET(owner < state.m_player_n, empty, "JobTargetManager owner");

    RuntimeStatics& st = *const_cast<RuntimeStatics*>(state.m_statics);
    DynJobYieldRegister& yld = st.dyn_job_yield();
    const u16 left = leftover_pops(state, city, city_idx);
    if (left == 0) {
        yld.reset_remain();
        return empty;
    }

    const DynJobSlotRegister& slots = st.dyn_job_slot();
    const EffectCtx ctx = make_ctx(state, city_idx, owner);
    if (state.m_player_states[owner].m_lucky != 0u) {
        return JobTarget_CommerceThenPreference::fill(left, trait_idx, yld, slots, ctx);
    }
    return JobTarget_PreferenceOnly::fill(left, trait_idx, yld, slots, ctx);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
