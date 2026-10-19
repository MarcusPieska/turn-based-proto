//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "army_general_turn_handler.h"

#include "army_general.h"
#include "booster_apply.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "game_state.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u16 gen_lim (GameState& st, u16 seat) {
    u16 lim = static_cast<u16>(GENERAL_SLOTS);
    if (st.m_statics == nullptr || st.m_player_states == nullptr || seat >= st.m_player_n) {
        return lim;
    }
    const PlayerState& ps = st.m_player_states[seat];
    EffectCtx ctx = {};
    ctx.m_owner = seat;
    ctx.m_city_idx = U16_KEY_NULL;
    ctx.m_tech = ps.m_techs_researched;
    ctx.m_wonder_city = st.m_wonder_city;
    ctx.m_wonder_n = st.m_wonder_count;
    ctx.m_small_wonder_city = ps.m_small_wonder_city;
    ctx.m_small_wonder_n = st.m_small_wonder_count;
    effect_ctx_bind_civ_traits(ctx, *st.m_statics, ps.m_civ_index);
    const DynBoosterRegister& reg = st.m_statics->dyn_booster();
    u16 n = 0u;
    n = apply_booster_u16(n, reg.determine(ItemEffectBoosterType::ARMY_GENERAL_LIMIT, ItemEffectsScope::LOCAL, ctx));
    n = apply_booster_u16(n, reg.determine(ItemEffectBoosterType::ARMY_GENERAL_LIMIT, ItemEffectsScope::CITY, ctx));
    n = apply_booster_u16(n, reg.determine(ItemEffectBoosterType::ARMY_GENERAL_LIMIT, ItemEffectsScope::CIV, ctx));
    n = apply_booster_u16(n, reg.determine(ItemEffectBoosterType::ARMY_GENERAL_LIMIT, ItemEffectsScope::GLOBAL, ctx));
    if (n > 0u && n < lim) {
        lim = n;
    }
    return lim;
}

//================================================================================================================================
//=> - ArmyGeneralTurnHandler -
//================================================================================================================================

void ArmyGeneralTurnHandler::handle (GameState& st) {
    if (st.m_player_states == nullptr) {
        return;
    }
    for (u16 seat = 0; seat < st.m_player_n; ++seat) {
        PlayerState& ps = st.m_player_states[seat];
        const u16 lim = gen_lim(st, seat);
        for (u16 i = 0; i < lim; ++i) {
            ArmyGeneral& g = ps.m_generals[i];
            if (g.grp() == U16_KEY_NULL) {
                continue;
            }
            if (!g.moved()) {
                g.train(st);
            }
            g.set_moved(false);
            g.boost_mvt_pts(st);
            g.boost_mvt_lvl(st);
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
