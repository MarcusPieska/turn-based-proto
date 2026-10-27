//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civic_adopt_helper.h"

#include <cstring>

#include "assert_log.h"
#include "bit_array.h"
#include "booster_apply.h"
#include "civ_static_data.h"
#include "civ_static_key.h"
#include "civ_trait_enum.h"
#include "civic_static_data.h"
#include "civic_static_key.h"
#include "civic_trait_orderings.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "game_state.h"
#include "general_assessor.h"
#include "item_effects.h"
#include "log_dbg.h"
#include "runtime_statics.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 count_bits (const BitArrayCL& ba) {
    u16 n = 0;
    const u32 c = ba.get_count();
    for (u32 i = 0; i < c; ++i) {
        if (ba.get_bit(i) != 0) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static u16 civic_lim (GameState& st, u16 seat) {
    GAME_EXPECT(st.m_statics != nullptr, "CivicAdoptHelper lim statics");
    GAME_EXPECT(st.m_player_states != nullptr, "CivicAdoptHelper lim seats");
    GAME_EXPECT(seat < st.m_player_n, "CivicAdoptHelper lim seat");
    const PlayerState& ps = st.m_player_states[seat];
    u16 lim = st.m_statics->config().get_static_civic_limit();
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
    lim = apply_booster_u16(lim, reg.determine(ItemEffectBoosterType::CIVIC_LIMIT, ItemEffectsScope::LOCAL, ctx));
    lim = apply_booster_u16(lim, reg.determine(ItemEffectBoosterType::CIVIC_LIMIT, ItemEffectsScope::CITY, ctx));
    lim = apply_booster_u16(lim, reg.determine(ItemEffectBoosterType::CIVIC_LIMIT, ItemEffectsScope::CIV, ctx));
    lim = apply_booster_u16(lim, reg.determine(ItemEffectBoosterType::CIVIC_LIMIT, ItemEffectsScope::GLOBAL, ctx));
    return lim;
}

static void fill_available (BitArrayCL& available, const PlayerState& ps, const RuntimeStatics& st) {
    const u16 civic_n = st.civic().get_item_count();
    GAME_EXPECT(available.get_count() == civic_n, "CivicAdoptHelper avail size");
    BitArrayCL resource(st.resource().get_item_count());
    BitArrayCL building(st.building().get_item_count());
    BitArrayCL toggle_city(st.toggle_city().get_item_count());
    BitArrayCL civ(st.civ().get_item_count());
    for (u32 i = 0; i < resource.get_count(); ++i) {
        resource.set_bit(i);
    }
    GAME_EXPECT(ps.m_civ_index < civ.get_count(), "CivicAdoptHelper civ index");
    civ.set_bit(ps.m_civ_index);
    BitArrayCL civ_trait(st.civ_trait().get_item_count());
    const CivStaticDataStruct& civ_row = st.civ().get_item(CivStaticDataKey::from_raw(ps.m_civ_index));
    for (u32 t = 0; t < MAX_CIV_TRAIT_COUNT; ++t) {
        const u16 tix = civ_row.traits.indices[t];
        if (tix != U16_KEY_NULL && tix < civ_trait.get_count()) {
            civ_trait.set_bit(tix);
        }
    }
    AssessorCtx ctx = {};
    ctx.m_tech = ps.m_techs_researched;
    ctx.m_civ = &civ;
    ctx.m_city_idx = 0;
    ctx.m_resource_bank = nullptr;
    ctx.m_building_bank = nullptr;
    ctx.m_toggle_city_bank = nullptr;
    ctx.m_resource = &resource;
    ctx.m_building = &building;
    ctx.m_toggle_city = &toggle_city;
    ctx.m_civ_trait = &civ_trait;

    u8 type_taken[256];
    std::memset(type_taken, 0, sizeof(type_taken));
    if (ps.m_civics_adopted != nullptr) {
        for (u16 i = 0; i < civic_n; ++i) {
            if (ps.m_civics_adopted->get_bit(i) == 0) {
                continue;
            }
            const u16 tp = st.civic().get_item(CivicStaticDataKey::from_raw(i)).type;
            if (tp < 256u) {
                type_taken[tp] = 1;
            }
        }
    }

    for (u16 i = 0; i < civic_n; ++i) {
        available.clear_bit(i);
        if (ps.m_civics_adopted != nullptr && ps.m_civics_adopted->get_bit(i) != 0) {
            continue;
        }
        const CivicStaticDataStruct& item = st.civic().get_item(CivicStaticDataKey::from_raw(i));
        if (item.type < 256u && type_taken[item.type] != 0) {
            continue;
        }
        if (!GeneralAssessor::chk(item.reqs, ctx)) {
            continue;
        }
        available.set_bit(i);
    }
}

//================================================================================================================================
//=> - CivicAdoptHelper -
//================================================================================================================================

void CivicAdoptHelper::fill (GameState& st, u16 seat) {
    if (st.m_statics == nullptr || st.m_player_states == nullptr || seat >= st.m_player_n) {
        return;
    }
    if (!CivicTraitOrderings::ready()) {
        return;
    }
    PlayerState& ps = st.m_player_states[seat];
    const RuntimeStatics& rst = *st.m_statics;
    const u16 civic_n = rst.civic().get_item_count();
    if (civic_n == 0 || civic_n != CivicTraitOrderings::civic_n()) {
        return;
    }
    if (ps.m_civics_adopted == nullptr) {
        ps.m_civics_adopted = new BitArrayCL(civic_n);
    }
    GAME_EXPECT(ps.m_civics_adopted != nullptr, "CivicAdoptHelper adopted null");
    GAME_EXPECT(ps.m_civ_index < rst.civ().get_item_count(), "CivicAdoptHelper civ row");
    const CivStaticDataStruct& civ_row = rst.civ().get_item(CivStaticDataKey::from_raw(ps.m_civ_index));
    const CivTrait trait = static_cast<CivTrait>(civ_row.traits.indices[0]);
    BitArrayCL available(civic_n);
    while (true) {
        const u16 lim = civic_lim(st, seat);
        const u16 owned = count_bits(*ps.m_civics_adopted);
        if (owned >= lim) {
            return;
        }
        fill_available(available, ps, rst);
        const u16 pick = CivicTraitOrderings::pick(available, static_cast<u16>(trait));
        if (pick == U16_KEY_NULL) {
            return;
        }
        ps.m_civics_adopted->set_bit(pick);
        LOG_PLAYER_CIVIC_ADOPT::LOG(seat, lim, pick);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
