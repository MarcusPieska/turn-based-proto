//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "muster_dispatch.h"

#include "army_general.h"
#include "city.h"
#include "city_array.h"
#include "game_state.h"
#include "generate_distance_p2p.h"
#include "general_move_boost.h"
#include "log_dbg/log_war_muster_can_step_diag.h"
#include "log_dbg/log_war_muster_can_step_fail.h"
#include "log_dbg/log_war_muster_peek_fail.h"
#include "log_dbg/log_war_muster_pull.h"
#include "log_dbg/log_war_muster_refill_mp.h"
#include "mvt_pts_boost_mng.h"
#include "runtime_statics.h"
#include "tile_entry_resolver.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_movement_mng.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u16 k_mpull_arr_front = 4u;

static bool pack_acc (
    SectorPathGradients* spg,
    u16 slot,
    GameState& st,
    u16 seat,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u8 acc) {
    if (spg == nullptr || st.m_statics == nullptr) {
        return false;
    }
    if (spg->pack_path(slot, st, *st.m_statics, sx, sy, dx, dy, seat, acc) && spg->ok(slot)) {
        return true;
    }
    spg->release(slot);
    return false;
}

static bool pack_fallbacks (
    SectorPathGradients* spg,
    u16 slot,
    GameState& st,
    u16 seat,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy) {
    static const u8 k_acc[3] = {
        GenerateDistanceP2P::k_acc_own_free,
        GenerateDistanceP2P::k_acc_non_peace,
        GenerateDistanceP2P::k_acc_none
    };
    for (u16 i = 0u; i < 3u; ++i) {
        if (pack_acc(spg, slot, st, seat, sx, sy, dx, dy, k_acc[i])) {
            return true;
        }
    }
    return false;
}

static bool cap_xy (GameState& st, u16 seat, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr || st.m_player_states == nullptr || seat >= st.m_player_n) {
        return false;
    }
    const u16 ci = st.m_player_states[seat].m_muster_city;
    if (ci == U16_KEY_NULL) {
        return false;
    }
    const City* c = st.m_cities.get_city(ci);
    if (c == nullptr || c->get_owner() != seat) {
        return false;
    }
    *ox = c->get_x();
    *oy = c->get_y();
    return true;
}

static void refill_hd (GameState& st, u16 seat, u16 head_idx) {
    if (st.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = st.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(head_idx);
    while (cur.is_valid()) {
        UnitAddStruct* u = st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_unit_typ_idx < typ_n) {
            const int before = static_cast<int>(u->m_mvt_points);
            const int budget = static_cast<int>(MvtPtsBoostMng::budget(st, *u));
            MvtPtsBoostMng::fill(st, u);
            LOG_WAR_MUSTER_REFILL_MP::LOG(
                SC_U32(seat), SC_U32(cur.value()), before, budget,
                static_cast<int>(u->m_mvt_points), SC_U32(st.m_current_turn));
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    GeneralMoveBoost::on_grp_refilled(st, UnitAddKey::from_raw(head_idx));
}

//================================================================================================================================
//=> - MusterDispatch -
//================================================================================================================================

MusterDispatch::MusterDispatch () :
    m_feed(nullptr),
    m_feed_i(U16_KEY_NULL),
    m_hd(U16_KEY_NULL),
    m_dx(0u),
    m_dy(0u),
    m_front(0u),
    m_ok(false) {
}

MusterDispatch::~MusterDispatch () {
    drop();
}

void MusterDispatch::clr () {
    drop();
    m_hd = U16_KEY_NULL;
    m_dx = 0u;
    m_dy = 0u;
    m_front = 0u;
}

void MusterDispatch::drop () {
    if (m_feed != nullptr && m_feed_i != U16_KEY_NULL) {
        m_feed->release(m_feed_i);
    }
    m_feed = nullptr;
    m_feed_i = U16_KEY_NULL;
    m_ok = false;
}

bool MusterDispatch::busy () const {
    return m_ok && m_hd != U16_KEY_NULL;
}

bool MusterDispatch::ok () const {
    return m_ok;
}

bool MusterDispatch::set_dest (GameState& st, u16 seat, u16 dx, u16 dy) {
    drop();
    u16 cx = 0u;
    u16 cy = 0u;
    if (!cap_xy(st, seat, &cx, &cy)) {
        return false;
    }
    u16 slot = U16_KEY_NULL;
    SectorPathGradients* spg = SectorPathGradients::pool_acq(&slot);
    if (spg == nullptr || slot == U16_KEY_NULL) {
        return false;
    }
    if (!pack_fallbacks(spg, slot, st, seat, cx, cy, dx, dy)) {
        spg->release(slot);
        return false;
    }
    m_feed = spg;
    m_feed_i = slot;
    m_dx = dx;
    m_dy = dy;
    m_ok = true;
    return true;
}

bool MusterDispatch::take_hd (UnitAddKey hd) {
    if (!hd.is_valid()) {
        return false;
    }
    m_hd = hd.value();
    return true;
}

UnitAddKey MusterDispatch::hd () const {
    return (m_hd == U16_KEY_NULL) ? UnitAddKey::None() : UnitAddKey::from_raw(m_hd);
}

SectorPathGradients::StepRes MusterDispatch::peek (GameState& st, u16 x, u16 y) const {
    SectorPathGradients::StepRes r = {};
    r.have = false;
    if (!m_ok || m_feed == nullptr || m_feed_i == U16_KEY_NULL) {
        return r;
    }
    return m_feed->peek(m_feed_i, st, x, y);
}

bool MusterDispatch::walk_tick (GameState& st, u16 seat) {
    if (!m_ok || m_feed == nullptr || m_feed_i == U16_KEY_NULL || m_hd == U16_KEY_NULL) {
        drop();
        return false;
    }
    const UnitAddKey key = UnitAddKey::from_raw(m_hd);
    UnitMovementMng::set_grp_campaign(st, key, 1u);
    refill_hd(st, seat, m_hd);
    UnitAddStruct* u = st.m_units.get_unit_add(key);
    if (u == nullptr || u->m_x == U16_KEY_NULL) {
        m_hd = U16_KEY_NULL;
        drop();
        return false;
    }
    if (u->m_x == m_dx && u->m_y == m_dy) {
        LOG_WAR_MUSTER_PULL::LOG(
            SC_U32(seat), SC_U32(m_dx), SC_U32(m_dy), k_mpull_arr_front, SC_U32(st.m_current_turn));
        drop();
        return false;
    }
    for (;;) {
        u = st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            break;
        }
        if (u->m_x == m_dx && u->m_y == m_dy) {
            break;
        }
        const u16 ux = u->m_x;
        const u16 uy = u->m_y;
        const SectorPathGradients::StepRes step = m_feed->peek(m_feed_i, st, ux, uy);
        if (!step.have) {
            LOG_WAR_MUSTER_PEEK_FAIL::LOG(
                SC_U32(seat), SC_U32(key.value()), SC_U32(static_cast<u8>(step.reason)),
                SC_U32(ux), SC_U32(uy), SC_U32(st.m_current_turn));
            break;
        }
        i16 step_cost = 0;
        const CanStepFail csr = UnitMovementMng::can_step_reason(st, key, step.nx, step.ny, &step_cost);
        if (csr == CanStepFail::Ok) {
            if (!UnitMovementMng::apply_step(st, key, step.nx, step.ny)) {
                break;
            }
            ArmyGeneral::stamp_moved(st, seat, m_hd);
            continue;
        }
        if (csr != CanStepFail::Entry) {
            const i16 mp_now = (u != nullptr) ? u->m_mvt_points : static_cast<i16>(0);
            const i16 gmin = UnitMovementMng::grp_min_mvt(st, key);
            const i16 cost_now = UnitMovementMng::tile_cost(st, ux, uy, step.nx, step.ny);
            const u16 nxt = (u != nullptr) ? static_cast<u16>(u->m_next_unit_in_group) : U16_KEY_NULL;
            LOG_WAR_MUSTER_CAN_STEP_FAIL::LOG(
                SC_U32(seat), SC_U32(key.value()), SC_U32(static_cast<u8>(csr)),
                SC_U32(ux), SC_U32(uy), SC_U32(step.nx), SC_U32(step.ny),
                SC_U32(st.m_current_turn));
            LOG_WAR_MUSTER_CAN_STEP_DIAG::LOG(
                SC_U32(seat), SC_U32(key.value()), SC_U32(static_cast<u8>(csr)),
                static_cast<int>(mp_now), static_cast<int>(gmin), static_cast<int>(cost_now),
                SC_U32(nxt), SC_U32(ux), SC_U32(uy), SC_U32(step.nx), SC_U32(step.ny),
                SC_U32(st.m_current_turn));
            (void)step_cost;
            break;
        }
        if (TileEntryResolver::resolve(key.value(), step.nx, step.ny) != TileEntryRes::BlockRemains) {
            if (UnitMovementMng::can_step(st, key, step.nx, step.ny, nullptr)
                && UnitMovementMng::apply_step(st, key, step.nx, step.ny)) {
                ArmyGeneral::stamp_moved(st, seat, m_hd);
                continue;
            }
        }
        LOG_WAR_MUSTER_CAN_STEP_FAIL::LOG(
            SC_U32(seat), SC_U32(key.value()), SC_U32(static_cast<u8>(csr)),
            SC_U32(ux), SC_U32(uy), SC_U32(step.nx), SC_U32(step.ny),
            SC_U32(st.m_current_turn));
        break;
    }
    return true;
}

u16 MusterDispatch::dest_x () const {
    return m_dx;
}

u16 MusterDispatch::dest_y () const {
    return m_dy;
}

u16 MusterDispatch::front () const {
    return m_front;
}

void MusterDispatch::set_front (u16 fi) {
    m_front = (fi < k_front_cap) ? fi : 0u;
}

SectorPathGradients* MusterDispatch::feed () {
    return m_feed;
}

u16 MusterDispatch::feed_i () const {
    return m_feed_i;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
