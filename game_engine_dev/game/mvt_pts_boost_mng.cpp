//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "mvt_pts_boost_mng.h"

#include "booster_apply.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "game_state.h"
#include "item_effects.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_domain_enum.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

#include <cstring>

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

u16* MvtPtsBoostMng::m_land = nullptr;
u16* MvtPtsBoostMng::m_sea = nullptr;
u16 MvtPtsBoostMng::m_n = 0u;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u16 seat_boost (const GameState& s, u16 seat, ItemEffectBoosterType tp) {
    if (s.m_statics == nullptr || s.m_player_states == nullptr || seat >= s.m_player_n) {
        return 0u;
    }
    const PlayerState& ps = s.m_player_states[seat];
    EffectCtx ctx = {};
    ctx.m_owner = seat;
    ctx.m_city_idx = U16_KEY_NULL;
    ctx.m_tech = ps.m_techs_researched;
    ctx.m_wonder_city = s.m_wonder_city;
    ctx.m_wonder_n = s.m_wonder_count;
    ctx.m_small_wonder_city = ps.m_small_wonder_city;
    ctx.m_small_wonder_n = s.m_small_wonder_count;
    const BoosterRegisterResult r = s.m_statics->dyn_booster().determine(tp, ItemEffectsScope::CIV, ctx);
    return apply_booster_u16(0u, r);
}

static u16 domain_boost (u16 seat, u16 domain) {
    if (domain == static_cast<u16>(UnitDomain::SEA)) {
        return MvtPtsBoostMng::sea(seat);
    }
    if (domain == static_cast<u16>(UnitDomain::LAND)) {
        return MvtPtsBoostMng::land(seat);
    }
    return 0u;
}

//================================================================================================================================
//=> - MvtPtsBoostMng -
//================================================================================================================================

bool MvtPtsBoostMng::setup (u16 player_n) {
    clear();
    if (player_n == 0u) {
        return false;
    }
    m_land = new u16[player_n];
    m_sea = new u16[player_n];
    std::memset(m_land, 0, sizeof(u16) * player_n);
    std::memset(m_sea, 0, sizeof(u16) * player_n);
    m_n = player_n;
    return true;
}

void MvtPtsBoostMng::clear () {
    delete[] m_land;
    delete[] m_sea;
    m_land = nullptr;
    m_sea = nullptr;
    m_n = 0u;
}

void MvtPtsBoostMng::refresh (GameState& s) {
    if (m_land == nullptr || m_sea == nullptr || m_n == 0u) {
        return;
    }
    const u16 n = (s.m_player_n < m_n) ? s.m_player_n : m_n;
    for (u16 p = 0; p < n; ++p) {
        m_land[p] = seat_boost(s, p, ItemEffectBoosterType::LAND_MVT_PTS);
        m_sea[p] = seat_boost(s, p, ItemEffectBoosterType::SEA_MVT_PTS);
    }
    for (u16 p = n; p < m_n; ++p) {
        m_land[p] = 0u;
        m_sea[p] = 0u;
    }
}

u16 MvtPtsBoostMng::land (u16 seat) {
    if (m_land == nullptr || seat >= m_n) {
        return 0u;
    }
    return m_land[seat];
}

u16 MvtPtsBoostMng::sea (u16 seat) {
    if (m_sea == nullptr || seat >= m_n) {
        return 0u;
    }
    return m_sea[seat];
}

i32 MvtPtsBoostMng::budget (const GameState& s, const UnitAddStruct& u) {
    if (s.m_statics == nullptr) {
        return 0;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    if (u.m_unit_typ_idx >= typ_n) {
        return 0;
    }
    const UnitStaticDataStruct& ud = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u.m_unit_typ_idx));
    const u16 base = s.m_statics->config().get_mov_pt_per_turn();
    const u16 boost = domain_boost(static_cast<u16>(u.m_player_idx), ud.domain);
    return static_cast<i32>(ud.mvt_pts) * static_cast<i32>(base) + static_cast<i32>(boost);
}

void MvtPtsBoostMng::fill (GameState& s, UnitAddStruct* u) {
    if (u == nullptr) {
        return;
    }
    i32 v = budget(s, *u);
    if (v < 0) {
        v = 0;
    }
    if (v > 32767) {
        v = 32767;
    }
    u->m_mvt_points = static_cast<i16>(v);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
