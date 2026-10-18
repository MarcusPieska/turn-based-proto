//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_turn_handler.h"

#include "assert_log.h"
#include "booster_apply.h"
#include "city.h"
#include "city_tile_manager.h"
#include "city_turn_handler_agricultural.h"
#include "city_turn_handler_commercial.h"
#include "city_turn_handler_core.h"
#include "city_turn_handler_ctx.h"
#include "city_turn_handler_default.h"
#include "city_turn_handler_expansionist.h"
#include "city_turn_handler_industrious.h"
#include "city_turn_handler_militaristic.h"
#include "city_turn_handler_religious.h"
#include "city_turn_handler_scientific.h"
#include "city_tracer.h"
#include "circular_tile_areas.h"
#include "civ_static_key.h"
#include "civ_trait_enum.h"
#include "dyn_booster_register.h"
#include "dyn_job_yield_register.h"
#include "effect_ctx.h"
#include "game_state.h"
#include "item_effects.h"
#include "job_assigner.h"
#include "job_count_array.h"
#include "job_effect_realizer.h"
#include "player_ledger.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_reach_r = 4;

static u16 clamp_u32 (u32 v) {
    return v > 65535u ? 65535u : static_cast<u16>(v);
}

static u16 clamp_i32 (i32 v) {
    if (v <= 0) {
        return 0;
    }
    if (v > 65535) {
        return 65535u;
    }
    return static_cast<u16>(v);
}

static u16 sum_yld (u32 tile, i32 job) {
    const u32 j = static_cast<u32>(clamp_i32(job));
    const u32 sum = tile + j;
    return clamp_u32(sum);
}

static u8 turn_job_seat_get (u16 city_idx, u16 job_id, void* ud) {
    (void)ud;
    if (!JobCountArray::ready()) {
        return 0u;
    }
    return JobCountArray::get(city_idx, job_id);
}

static EffectCtx make_job_ctx (GameState& state, u16 city_idx, u16 owner) {
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
    ctx.m_job_seat_fn = turn_job_seat_get;
    ctx.m_job_seat_ud = nullptr;
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

static u16 seated_n (u16 city_idx) {
    if (!JobCountArray::ready()) {
        return 0;
    }
    u16 n = 0;
    const u16 jn = JobCountArray::job_n();
    for (u16 j = 0; j < jn; ++j) {
        n = SC_U16(n + JobCountArray::get(city_idx, j));
    }
    return n;
}

static void ensure_jobs (GameState& state, City* city, u16 city_idx, u16 trait_idx) {
    const u16 left = leftover_pops(state, city, city_idx);
    if (left == 0 || seated_n(city_idx) > 0) {
        return;
    }
    const EffectCtx jctx = make_job_ctx(state, city_idx, city->get_owner());
    JobAssigner::fill(city_idx, trait_idx, left, jctx, *state.m_statics);
}

static void refill_jobs (GameState& state, City* city, u16 city_idx, u16 trait_idx) {
    const u16 left = leftover_pops(state, city, city_idx);
    const EffectCtx jctx = make_job_ctx(state, city_idx, city->get_owner());
    JobAssigner::fill(city_idx, trait_idx, left, jctx, *state.m_statics);
}

static u16 resolve_train_daily (GameState& state, u16 city_idx, u16 owner) {
    const EffectCtx ctx = make_job_ctx(state, city_idx, owner);
    const DynBoosterRegister& reg = state.m_statics->dyn_booster();
    u16 daily = 0u;
    daily = apply_booster_u16(daily, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_DAILY, ItemEffectsScope::LOCAL, ctx));
    daily = apply_booster_u16(daily, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_DAILY, ItemEffectsScope::CITY, ctx));
    daily = apply_booster_u16(daily, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_DAILY, ItemEffectsScope::CIV, ctx));
    daily = apply_booster_u16(daily, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_DAILY, ItemEffectsScope::GLOBAL, ctx));
    return daily;
}

//================================================================================================================================
//=> - CityTurnHandler -
//================================================================================================================================

void CityTurnHandler::handle (GameState& state, u16 city_idx) {
    City* city = state.m_cities.get_city(city_idx);
    GAME_EXPECT(city != nullptr, "CityTurnHandler got nullptr city");
    city->restore_defense();

    GAME_EXPECT(state.m_player_states != nullptr, "CityTurnHandler got nullptr player states");
    GAME_EXPECT(state.m_statics != nullptr, "CityTurnHandler got nullptr statics");
    const u16 player = city->get_owner();
    GAME_EXPECT(player < state.m_player_n, "CityTurnHandler player out of bounds");
    PlayerState& ps = state.m_player_states[player];

    CivTrait trait = CivTrait::Agricultural;
    if (ps.m_civ_index < state.m_statics->civ().get_item_count()) {
        const CivStaticDataStruct& civ = state.m_statics->civ().get_item(CivStaticDataKey::from_raw(ps.m_civ_index));
        trait = static_cast<CivTrait>(civ.traits.indices[0]);
    }
    const u16 trait_idx = static_cast<u16>(trait);

    const TotalTileYield yld = CityTileManager::gather_yields(player, city_idx);
    ensure_jobs(state, city, city_idx, trait_idx);
    const DynJobYieldPack job = JobEffectRealizer::realize(city_idx, *state.m_statics);
    const u16 food = sum_yld(yld.m_food, job.m_food);
    const u16 production = sum_yld(yld.m_production, job.m_production);
    const u16 commerce = sum_yld(yld.m_commerce, job.m_commerce);
    const u16 culture = clamp_i32(job.m_culture);
    const u16 science = clamp_i32(job.m_science);
    const u16 sanitation_boost = city->get_city_sanitation_boost(city_idx);
    const i16 net_sanitation = city->get_city_net_sanitation(sanitation_boost);

    LOG_CITY_BEGIN((city_idx, player, state.m_current_turn));

    city->add_commerce(city_idx, commerce);
    if (science > 0) {
        GAME_EXPECT(PlayerLedger::add_research(player, science), "CityTurnHandler job science");
    }
    city->add_culture(city_idx, culture);
    city->add_production(city_idx, production);
    city->finish_if_ready(city_idx);
    city->add_training(city_idx, resolve_train_daily(state, city_idx, player));
    i16 pop_change = city->add_food(city_idx, food, net_sanitation);

    CityTurnHandlerCtx ctx;
    ctx.m_state = &state;
    ctx.m_city = city;
    ctx.m_city_idx = city_idx;
    ctx.m_player = player;
    ctx.m_sanitation_boost = sanitation_boost;
    ctx.m_net_sanitation = net_sanitation;
    ctx.m_pop_change = pop_change;

    city->refresh_unit_support(city_idx);
    CityTurnHandler_Core::try_assess_imps(state, city_idx, city);
    CityTurnHandler_Core::try_pick_worker(state, city_idx, city);
    CityTurnHandler_Core::try_pick_land_unit(state, city_idx, city);
    switch (trait) {
    case CivTrait::Agricultural:
        CityTurnHandler_Agricultural::handle(ctx);
        break;
    case CivTrait::Industrious:
        CityTurnHandler_Industrious::handle(ctx);
        break;
    case CivTrait::Expansionist:
        CityTurnHandler_Expansionist::handle(ctx);
        break;
    case CivTrait::Religious:
        CityTurnHandler_Religious::handle(ctx);
        break;
    case CivTrait::Militaristic:
        CityTurnHandler_Militaristic::handle(ctx);
        break;
    case CivTrait::Scientific:
        CityTurnHandler_Scientific::handle(ctx);
        break;
    case CivTrait::Commercial:
        CityTurnHandler_Commercial::handle(ctx);
        break;
    default:
        CityTurnHandler_Default::handle(ctx);
        break;
    }

    if (pop_change != 0) {
        refill_jobs(state, city, city_idx, trait_idx);
    }

    LOG_CITY_COMMIT(());

    city->count_unit_build_support(&ps);
    ps.m_this_turn_city_count = static_cast<u16>(ps.m_this_turn_city_count + 1u);
    ps.m_this_turn_population_count = ps.m_this_turn_population_count + static_cast<u32>(city->get_current_population());
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
