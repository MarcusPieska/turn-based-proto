//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "army_general.h"

#include "booster_apply.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "job_count_array.h"
#include "log_dbg.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_domain_enum.h"
#include "unit_level_static_data.h"
#include "unit_level_static_key.h"
#include "unit_role_enum.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u8 job_seat (u16 city_idx, u16 job_id, void*) {
    if (!JobCountArray::ready()) {
        return 0u;
    }
    return JobCountArray::get(city_idx, job_id);
}

static bool land_role (const RuntimeStatics& st, u16 typ) {
    if (typ >= st.unit().get_item_count()) {
        return false;
    }
    const UnitStaticDataStruct& u = st.unit().get_item(UnitStaticDataKey::from_raw(typ));
    return u.domain == static_cast<u16>(UnitDomain::LAND)
        && u.role != static_cast<u16>(UnitRole::NONE);
}

static EffectCtx make_ctx (const GameState& st, u16 seat, u16 x, u16 y) {
    EffectCtx ctx = {};
    ctx.m_owner = seat;
    ctx.m_city_idx = U16_KEY_NULL;
    if (st.m_player_states != nullptr && seat < st.m_player_n) {
        const PlayerState& ps = st.m_player_states[seat];
        ctx.m_tech = ps.m_techs_researched;
        ctx.m_small_wonder_city = ps.m_small_wonder_city;
        if (st.m_statics != nullptr) {
            effect_ctx_bind_civ_traits(ctx, *st.m_statics, ps.m_civ_index);
        }
    }
    ctx.m_wonder_city = st.m_wonder_city;
    ctx.m_wonder_n = st.m_wonder_count;
    ctx.m_small_wonder_n = st.m_small_wonder_count;
    if (x != U16_KEY_NULL && y != U16_KEY_NULL
        && st.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        ctx.m_city_idx = st.m_map.get_add_idx(x, y);
        ctx.m_bld_bank = st.m_cities.get_bld_bank();
        ctx.m_job_seat_fn = job_seat;
        ctx.m_job_seat_ud = nullptr;
    }
    return ctx;
}

static u16 max_lv (const GameState& st, const EffectCtx& ctx) {
    if (st.m_statics == nullptr) {
        return 0u;
    }
    const DynBoosterRegister& reg = st.m_statics->dyn_booster();
    u16 lv = 0u;
    lv = apply_booster_u16(lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::LOCAL, ctx));
    lv = apply_booster_u16(lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CITY, ctx));
    lv = apply_booster_u16(lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::CIV, ctx));
    lv = apply_booster_u16(lv, reg.determine(ItemEffectBoosterType::UNIT_TRAINING_MAX, ItemEffectsScope::GLOBAL, ctx));
    const u16 n = st.m_statics->unit_level().get_item_count();
    const u16 hi = (n == 0u) ? 0u : static_cast<u16>(n - 1u);
    if (lv > hi) {
        lv = hi;
    }
    return lv;
}

//================================================================================================================================
//=> - ArmyGeneral -
//================================================================================================================================

ArmyGeneral::ArmyGeneral () :
    m_hd(U16_KEY_NULL),
    m_moved(0u),
    m_pad(0u) {
}

void ArmyGeneral::asgn (u16 hd) {
    m_hd = hd;
    m_moved = 1u;
}

void ArmyGeneral::unasgn () {
    m_hd = U16_KEY_NULL;
    m_moved = 0u;
}

u16 ArmyGeneral::grp () const {
    return static_cast<u16>(m_hd);
}

bool ArmyGeneral::moved () const {
    return m_moved != 0u;
}

void ArmyGeneral::set_moved (bool on) {
    m_moved = on ? 1u : 0u;
}

void ArmyGeneral::train (GameState& st) {
    const u16 hd = grp();
    if (hd == U16_KEY_NULL || st.m_statics == nullptr) {
        return;
    }
    UnitAddStruct* hu = st.m_units.get_unit_add(UnitAddKey::from_raw(hd));
    if (hu == nullptr) {
        return;
    }
    const u16 seat = static_cast<u16>(hu->m_player_idx);
    const EffectCtx ctx = make_ctx(st, seat, static_cast<u16>(hu->m_x), static_cast<u16>(hu->m_y));
    const u16 hi = max_lv(st, ctx);
    if (hi == 0u) {
        return;
    }
    u16 cur = hd;
    while (cur != U16_KEY_NULL) {
        UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 nxt = static_cast<u16>(u->m_next_unit_in_group);
        if (land_role(*st.m_statics, static_cast<u16>(u->m_unit_typ_idx))
            && u->m_level < static_cast<u8>(hi)) {
            u->m_level = static_cast<u8>(u->m_level + 1u);
            return;
        }
        cur = nxt;
    }
}

bool ArmyGeneral::full (const GameState& st) const {
    const u16 hd = grp();
    if (hd == U16_KEY_NULL || st.m_statics == nullptr) {
        return true;
    }
    const UnitAddStruct* hu = st.m_units.get_unit_add(UnitAddKey::from_raw(hd));
    if (hu == nullptr) {
        return true;
    }
    const u16 seat = static_cast<u16>(hu->m_player_idx);
    const EffectCtx ctx = make_ctx(st, seat, static_cast<u16>(hu->m_x), static_cast<u16>(hu->m_y));
    const u16 hi = max_lv(st, ctx);
    if (hi == 0u) {
        return true;
    }
    u16 cur = hd;
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 nxt = static_cast<u16>(u->m_next_unit_in_group);
        if (land_role(*st.m_statics, static_cast<u16>(u->m_unit_typ_idx))
            && u->m_level < static_cast<u8>(hi)) {
            return false;
        }
        cur = nxt;
    }
    return true;
}

void ArmyGeneral::boost_mvt_pts (GameState& st) {
    const u16 hd = grp();
    if (hd == U16_KEY_NULL || st.m_statics == nullptr) {
        return;
    }
    const u16 turn_mp = st.m_statics->config().get_mov_pt_per_turn();
    if (turn_mp == 0u) {
        return;
    }
    u16 cur = hd;
    while (cur != U16_KEY_NULL) {
        UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        u->m_mvt_points = static_cast<i16>(static_cast<i32>(u->m_mvt_points) + static_cast<i32>(turn_mp));
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = static_cast<u16>(u->m_next_unit_in_group);
    }
}

void ArmyGeneral::boost_mvt_lvl (GameState& st) {
    const u16 hd = grp();
    if (hd == U16_KEY_NULL || st.m_statics == nullptr) {
        return;
    }
    const UnitLevelStaticData& lvls = st.m_statics->unit_level();
    const u16 lvl_n = lvls.get_item_count();
    if (lvl_n == 0u) {
        return;
    }
    u32 n = 0u;
    u32 sum = 0u;
    u16 cur = hd;
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        n++;
        sum += static_cast<u32>(u->m_level);
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = static_cast<u16>(u->m_next_unit_in_group);
    }
    if (n == 0u) {
        return;
    }
    u16 avg = static_cast<u16>(sum / n);
    if (avg >= lvl_n) {
        avg = static_cast<u16>(lvl_n - 1u);
    }
    u32 bonus = 0u;
    for (u16 i = 0u; i <= avg; ++i) {
        bonus += lvls.get_item(UnitLevelStaticDataKey::from_raw(i)).mvt_boost;
    }
    const UnitAddStruct* hu = st.m_units.get_unit_add(UnitAddKey::from_raw(hd));
    const u16 seat = (hu != nullptr) ? static_cast<u16>(hu->m_player_idx) : 0u;
    LOG_ARMY_GEN_MVT_LVL::LOG(seat, avg, static_cast<u16>(bonus));
    if (bonus == 0u) {
        return;
    }
    cur = hd;
    while (cur != U16_KEY_NULL) {
        UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        u->m_mvt_points = static_cast<i16>(static_cast<i32>(u->m_mvt_points) + static_cast<i32>(bonus));
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = static_cast<u16>(u->m_next_unit_in_group);
    }
}

void ArmyGeneral::stamp_moved (GameState& st, u16 seat, u16 hd) {
    if (st.m_player_states == nullptr || seat >= st.m_player_n || hd == U16_KEY_NULL) {
        return;
    }
    PlayerState& ps = st.m_player_states[seat];
    for (u16 i = 0; i < static_cast<u16>(GENERAL_SLOTS); ++i) {
        ArmyGeneral& g = ps.m_generals[i];
        const u16 ghd = g.grp();
        if (ghd == U16_KEY_NULL) {
            continue;
        }
        u16 cur = ghd;
        while (cur != U16_KEY_NULL) {
            if (cur == hd) {
                g.set_moved(true);
                return;
            }
            const UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(cur));
            if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = static_cast<u16>(u->m_next_unit_in_group);
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
