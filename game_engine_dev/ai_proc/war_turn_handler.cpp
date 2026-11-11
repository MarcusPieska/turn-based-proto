//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_turn_handler.h"

#include "army_general.h"
#include "army_upgrade_mng.h"
#include "booster_apply.h"
#include "army_reassembler.h"
#include "border_turn_handler.h"
#include "city_attack_manager.h"
#include "city.h"
#include "city_array.h"
#include "civ_relations.h"
#include "dyn_booster_register.h"
#include "effect_ctx.h"
#include "civ_static_key.h"
#include "game_array_simple.h"
#include "generate_access_mask.h"
#include "generate_distance_p2p.h"
#include "generate_exposure.h"
#include "gen_land_sector_network.h"
#include "gen_land_sectors.h"
#include "game_state.h"
#include "general_move_boost.h"
#include "game_map_defs.h"
#include "mvt_pts_boost_mng.h"
#include "expansion_state.h"
#include "land_sector_adj.h"
#include "land_sector_network.h"
#include "log_dbg.h"
#include "mock_muster_siege.h"
#include "negotiate_peace.h"
#include "runtime_statics.h"
#include "sect_select.h"
#include "sect_select_defensible.h"
#include "sect_select_walk.h"
#include "sector_support.h"
#include "target_ordering_flood.h"
#include "tile_entry_resolver.h"
#include "tile_transfer.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_group_management.h"
#include "unit_movement_mng.h"
#include "war_staging_selector.h"
#include "war_target_policy.h"
#include "unit_static_key.h"
#include "unit_turn_handler.h"
#include "unit_type_action_map.h"
#include "unit_utility_helper.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool filt_land_army_grp (GameState& s, UnitAddKey* head_io) {
    if (head_io == nullptr || !head_io->is_valid() || s.m_statics == nullptr) {
        return false;
    }
    static const u16 k_cap = 2048u;
    UnitAddKey keys[k_cap];
    u16 n = 0u;
    u16 x = U16_KEY_NULL;
    u16 y = U16_KEY_NULL;
    UnitAddKey cur = *head_io;
    while (cur.is_valid() && n < k_cap) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (n == 0u) {
            x = u->m_x;
            y = u->m_y;
        }
        keys[n++] = cur;
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    if (n == 0u || x == U16_KEY_NULL) {
        return false;
    }
    UnitAddStruct* hu = s.m_units.get_unit_add(*head_io);
    if (hu == nullptr) {
        return false;
    }
    UnitAddKey nxt = UnitAddKey::from_raw(hu->m_next_unit_in_group);
    hu->m_next_unit_in_group = U16_KEY_NULL;
    while (nxt.is_valid()) {
        UnitAddStruct* tu = s.m_units.get_unit_add(nxt);
        if (tu == nullptr) {
            return false;
        }
        const UnitAddKey fol = UnitAddKey::from_raw(tu->m_next_unit_in_group);
        tu->m_next_unit_in_group = U16_KEY_NULL;
        if (!UnitMovementMng::stack_append(s, nxt, x, y)) {
            return false;
        }
        nxt = fol;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    UnitAddKey keep[k_cap];
    u16 kn = 0u;
    for (u16 i = 0; i < n; ++i) {
        const UnitAddStruct* u = s.m_units.get_unit_add(keys[i]);
        if (u == nullptr || u->m_unit_typ_idx >= typ_n) {
            continue;
        }
        const u16 ut = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (!UnitUtilityHelper::is_type_for_land_army(ut)) {
            continue;
        }
        keep[kn++] = keys[i];
    }
    if (kn == 0u) {
        *head_io = UnitAddKey::None();
        return false;
    }
    return UnitMovementMng::form_group_chain(s, keep, kn, head_io);
}

static bool mob_hit (const WalkP2P& mob, u16 x, u16 y) {
    if (!mob.ok() || x >= mob.w() || y >= mob.h()) {
        return false;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(mob.w()) + static_cast<u32>(x);
    return mob.turn()[i] != GenerateDistanceP2P::k_turn_sent;
}

//================================================================================================================================
//=> - WarTurnHandler -
//================================================================================================================================

WarTurnHandler::WarTurnHandler (GameState& s, u16 seat) :
    m_st(s),
    m_seat(seat),
    m_mob(),
    m_grp_n(0),
    m_atk_n(0),
    m_flag_i(U16_KEY_NULL),
    m_tgt_n(0),
    m_tgt_i(0),
    m_sx(0),
    m_sy(0),
    m_tx(0),
    m_ty(0),
    m_re_i(U16_KEY_NULL),
    m_hold_sx(0),
    m_hold_sy(0),
    m_enemy(U8_KEY_NULL),
    m_sector(U16_KEY_NULL),
    m_assign_busy(false),
    m_ready(false),
    m_mob_ok(false) {
    for (u16 i = 0; i < k_atk_cap; ++i) {
        m_atk[i].m_hd = U16_KEY_NULL;
        m_atk[i].m_split = U16_KEY_NULL;
        m_atk[i].m_tx = 0u;
        m_atk[i].m_ty = 0u;
        m_atk[i].m_idx = i;
        m_atk[i].m_path_i = U16_KEY_NULL;
        m_atk[i].m_front = U16_KEY_NULL;
        m_atk[i].m_paths = nullptr;
        m_atk[i].m_phase = ArmyPhase::Idle;
        m_atk[i].m_stall = false;
        m_atk[i].m_br_tot = 0u;
        m_atk[i].m_br_last = 0u;
        m_atk[i].m_assault = WarAssault::Fail;
    }
}

WarTurnHandler::~WarTurnHandler () {
    for (u16 i = 0; i < k_atk_cap; ++i) {
        path_drop(i);
    }
}

void WarTurnHandler::path_drop (u16 army_i) {
    if (army_i >= k_atk_cap) {
        return;
    }
    if (m_atk[army_i].m_paths != nullptr && m_atk[army_i].m_path_i != U16_KEY_NULL) {
        m_atk[army_i].m_paths->release(m_atk[army_i].m_path_i);
    }
    m_atk[army_i].m_paths = nullptr;
    m_atk[army_i].m_path_i = U16_KEY_NULL;
}

bool WarTurnHandler::ok () const {
    return m_st.m_statics != nullptr
        && m_st.m_player_states != nullptr
        && m_seat < m_st.m_player_n
        && m_mob.ok();
}

u16 WarTurnHandler::grp_use_n () const {
    return (m_atk_n < k_grp_use) ? m_atk_n : k_grp_use;
}

static u16 wth_sec_at (const Whiteboard_2B* wb, u16 x, u16 y) {
    if (wb == nullptr || !wb->ok() || x >= wb->w() || y >= wb->h()) {
        return U16_KEY_NULL;
    }
    const u16 tag = wb->rd(x, y);
    return (tag == 0u) ? U16_KEY_NULL : static_cast<u16>(tag - 1u);
}

static u16 wth_own_seed_sec (u16 seat, u16 tgt) {
    if (tgt == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    if (SectorSupport::has_own_city(seat, tgt)) {
        return tgt;
    }
    const LandSectorAdj* adj = WarTurnHandler::sec_adj();
    if (adj == nullptr || !adj->ok()) {
        return U16_KEY_NULL;
    }
    for (u16 e = adj->hd(tgt); e != U16_KEY_NULL; e = adj->nx(e)) {
        const u16 n = adj->to(e);
        if (SectorSupport::has_own_city(seat, n)) {
            return n;
        }
    }
    return U16_KEY_NULL;
}

bool WarTurnHandler::set_goal_army (u16 army_i, u16 x2, u16 y2) {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || army_i >= k_grp_use || army_i >= k_atk_cap || m_atk[army_i].m_hd == U16_KEY_NULL) {
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    Whiteboard_1B acc("WarTurnHandler", "acc", 0u);
    if (!GenerateAccessMask::generate(m_st, m_seat, acc)) {
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    u16 ax = m_sx;
    u16 ay = m_sy;
    const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[army_i].m_hd));
    if (u != nullptr && u->m_x != U16_KEY_NULL) {
        ax = u->m_x;
        ay = u->m_y;
    }
    const Whiteboard_2B* wb = sec_wb();
    u16 to = m_sector;
    if (to == U16_KEY_NULL) {
        to = wth_sec_at(wb, x2, y2);
    }
    const u16 from = wth_own_seed_sec(m_seat, to);
    u16 secs[2];
    u16 sn = 0u;
    if (from != U16_KEY_NULL) {
        secs[sn++] = from;
    }
    if (to != U16_KEY_NULL && (sn == 0u || secs[0] != to)) {
        secs[sn++] = to;
    }
    path_drop(army_i);
    u16 slot = U16_KEY_NULL;
    SectorPathGradients* spg = SectorPathGradients::pool_acq(&slot);
    if (spg == nullptr || slot == U16_KEY_NULL) {
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    bool ok = false;
    if (sn == 1u) {
        ok = spg->lease(slot, secs[0])
            && spg->fill(slot, m_st, *m_st.m_statics, ax, ay, x2, y2, &acc)
            && spg->ok(slot);
    } else if (sn == 2u) {
        ok = spg->lease_multi(slot, secs, sn)
            && spg->fill(slot, m_st, *m_st.m_statics, ax, ay, x2, y2, &acc)
            && spg->ok(slot);
    }
    if (!ok) {
        spg->release(slot);
        ok = spg->pack_path(slot, m_st, *m_st.m_statics, ax, ay, x2, y2, &acc) && spg->ok(slot);
    }
    if (!ok) {
        spg->release(slot);
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    m_atk[army_i].m_path_i = slot;
    m_atk[army_i].m_paths = spg;
    m_atk[army_i].m_tx = x2;
    m_atk[army_i].m_ty = y2;
    return true;
}

bool WarTurnHandler::set_goal (u16 x1, u16 y1, u16 x2, u16 y2) {
    PROF_TH_WAR_ENTER(__func__);
    (void)x1;
    (void)y1;
    m_ready = false;
    if (!ok()) {
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    const u16 n = grp_use_n();
    if (n == 0u) {
        if (!set_goal_army(0u, x2, y2)) {
            return false;
        }
        m_ready = true;
        return true;
    }
    for (u16 i = 0; i < n; ++i) {
        if (m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        if (!set_goal_army(i, x2, y2)) {
            return false;
        }
    }
    m_ready = true;
    return true;
}

bool WarTurnHandler::make_muster_gradient (u16 x, u16 y) {
    PROF_TH_WAR_ENTER(__func__);
    m_mob_ok = false;
    m_grp_n = 0;
    if (!ok()) {
        LOG_WAR_MUSTER_GRAD_FAIL::LOG(SC_U32(m_seat), SC_U32(x), SC_U32(y), SC_U32(m_st.m_current_turn));
        return false;
    }
    Whiteboard_1B acc("WarTurnHandler", "muster_acc", 0u);
    if (!GenerateAccessMask::generate_own_free(m_st, m_seat, acc)) {
        LOG_WAR_MUSTER_GRAD_FAIL::LOG(SC_U32(m_seat), SC_U32(x), SC_U32(y), SC_U32(m_st.m_current_turn));
        return false;
    }
    u16 dmax = 0;
    if (!GenerateDistanceP2P::generate(m_st, *m_st.m_statics, x, y, m_mob, &acc, &dmax)) {
        LOG_WAR_MUSTER_GRAD_FAIL::LOG(SC_U32(m_seat), SC_U32(x), SC_U32(y), SC_U32(m_st.m_current_turn));
        return false;
    }
    m_sx = x;
    m_sy = y;
    m_mob_ok = true;
    return true;
}

u16 WarTurnHandler::do_total_muster () {
    PROF_TH_WAR_ENTER(__func__);
    m_grp_n = 0;
    if (!ok() || !m_mob_ok) {
        return 0;
    }
    muster_pull_city_armies();
    const u16 cn = m_st.m_cities.get_city_count();
    for (u16 i = 0; i < cn && m_grp_n < k_grp_cap; ++i) {
        const City* c = m_st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != m_seat) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (!mob_hit(m_mob, cx, cy)) {
            continue;
        }
        if (cx == m_sx && cy == m_sy) {
            continue;
        }
        UnitAddKey head = UnitAddKey::None();
        if (!UnitMovementMng::muster_leave_one_defense(m_st, cx, cy, m_seat, &head)) {
            continue;
        }
        if (!filt_land_army_grp(m_st, &head)) {
            continue;
        }
        UnitMovementMng::set_grp_campaign(m_st, head, 1u);
        m_grp[m_grp_n].m_hd = head.value();
        m_grp[m_grp_n].m_exp = GenerateExposure::k_none;
        m_grp_n++;
    }
    return m_grp_n;
}

bool WarTurnHandler::determine_exposure (u16 enemy) {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || m_grp_n == 0u) {
        LOG_WAR_EXPOSURE_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn));
        return false;
    }
    Whiteboard_1B board("WarTurnHandler", "exp", 0u);
    if (!GenerateExposure::generate(m_st, m_seat, enemy, board)) {
        LOG_WAR_EXPOSURE_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn));
        for (u16 i = 0; i < m_grp_n; ++i) {
            m_grp[i].m_exp = GenerateExposure::k_none;
        }
        return true;
    }
    for (u16 i = 0; i < m_grp_n; ++i) {
        const UnitAddKey key = UnitAddKey::from_raw(m_grp[i].m_hd);
        const UnitAddStruct* u = m_st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            m_grp[i].m_exp = GenerateExposure::k_none;
            continue;
        }
        m_grp[i].m_exp = board.rd(u->m_x, u->m_y);
    }
    return true;
}

void WarTurnHandler::refill_grp (u16 head_idx) {
    if (m_st.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = m_st.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(head_idx);
    while (cur.is_valid()) {
        UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_unit_typ_idx < typ_n) {
            MvtPtsBoostMng::fill(m_st, u);
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    GeneralMoveBoost::on_grp_refilled(m_st, UnitAddKey::from_raw(head_idx));
}

bool WarTurnHandler::army_in_own_city (u16 army_i) const {
    if (army_i >= k_atk_cap || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return false;
    }
    const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[army_i].m_hd));
    if (u == nullptr || u->m_x == U16_KEY_NULL) {
        return false;
    }
    const u16 cn = m_st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != m_seat) {
            continue;
        }
        if (c->get_x() == u->m_x && c->get_y() == u->m_y) {
            return true;
        }
    }
    return false;
}

void WarTurnHandler::muster_pull_city_armies () {
    PROF_TH_WAR_ENTER(__func__);
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (i == m_flag_i || m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        if (!army_in_own_city(i)) {
            continue;
        }
        const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[i].m_hd));
        if (u != nullptr && u->m_x == m_sx && u->m_y == m_sy) {
            (void)merge_into_flag(i);
            continue;
        }
        (void)park_army(i);
    }
}

bool WarTurnHandler::walk_muster () {
    PROF_TH_WAR_ENTER(__func__);
    bool any = false;
    if (!ok() || !m_mob_ok) {
        LOG_WAR_WALK_MUSTER_FAIL::LOG(SC_U32(m_seat), SC_U32(m_st.m_current_turn));
        return false;
    }
    for (u16 i = 0; i < m_grp_n; ++i) {
        if (m_grp[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        refill_grp(m_grp[i].m_hd);
        const UnitAddKey key = UnitAddKey::from_raw(m_grp[i].m_hd);
        UnitAddStruct* u = m_st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (u->m_x == m_sx && u->m_y == m_sy) {
            continue;
        }
        for (;;) {
            u = m_st.m_units.get_unit_add(key);
            if (u == nullptr || u->m_x == U16_KEY_NULL) {
                break;
            }
            if (u->m_x == m_sx && u->m_y == m_sy) {
                break;
            }
            const u16 ux = u->m_x;
            const u16 uy = u->m_y;
            const WalkP2P::StepRes step = m_mob.peek(m_st, ux, uy);
            if (!step.have) {
                LOG_WAR_MUSTER_PEEK_FAIL::LOG(
                    SC_U32(m_seat), SC_U32(key.value()), SC_U32(static_cast<u8>(step.reason)),
                    SC_U32(ux), SC_U32(uy), SC_U32(m_st.m_current_turn));
                break;
            }
            const CanStepFail csr = UnitMovementMng::can_step_reason(m_st, key, step.nx, step.ny, nullptr);
            if (csr == CanStepFail::Ok) {
                if (!UnitMovementMng::apply_step(m_st, key, step.nx, step.ny)) {
                    break;
                }
                ArmyGeneral::stamp_moved(m_st, m_seat, m_grp[i].m_hd);
                any = true;
                continue;
            }
            if (csr != CanStepFail::Entry) {
                LOG_WAR_MUSTER_CAN_STEP_FAIL::LOG(
                    SC_U32(m_seat), SC_U32(key.value()), SC_U32(static_cast<u8>(csr)),
                    SC_U32(ux), SC_U32(uy), SC_U32(step.nx), SC_U32(step.ny),
                    SC_U32(m_st.m_current_turn));
                break;
            }
            if (TileEntryResolver::resolve(key.value(), step.nx, step.ny) != TileEntryRes::BlockRemains) {
                if (UnitMovementMng::can_step(m_st, key, step.nx, step.ny, nullptr)
                    && UnitMovementMng::apply_step(m_st, key, step.nx, step.ny)) {
                    ArmyGeneral::stamp_moved(m_st, m_seat, m_grp[i].m_hd);
                    any = true;
                    continue;
                }
            }
            u16 ox[WalkP2P::k_opt_n];
            u16 oy[WalkP2P::k_opt_n];
            // TODO: peek_opts fallback slowed execution a lot; optimize heavily or remove once Entry resolver exists.
            m_mob.peek_opts(m_st, ux, uy, ox, oy);
            bool moved = false;
            CanStepFail last = csr;
            u16 lnx = step.nx;
            u16 lny = step.ny;
            for (u16 k = 0u; k < WalkP2P::k_opt_n; ++k) {
                if (ox[k] == U16_KEY_NULL) {
                    break;
                }
                if (ox[k] == step.nx && oy[k] == step.ny) {
                    continue;
                }
                const CanStepFail alt = UnitMovementMng::can_step_reason(m_st, key, ox[k], oy[k], nullptr);
                if (alt != CanStepFail::Ok) {
                    last = alt;
                    lnx = ox[k];
                    lny = oy[k];
                    continue;
                }
                if (!UnitMovementMng::apply_step(m_st, key, ox[k], oy[k])) {
                    last = CanStepFail::BadUnit;
                    lnx = ox[k];
                    lny = oy[k];
                    continue;
                }
                moved = true;
                break;
            }
            if (moved) {
                ArmyGeneral::stamp_moved(m_st, m_seat, m_grp[i].m_hd);
                any = true;
                continue;
            }
            LOG_WAR_MUSTER_CAN_STEP_FAIL::LOG(
                SC_U32(m_seat), SC_U32(key.value()), SC_U32(static_cast<u8>(last)),
                SC_U32(ux), SC_U32(uy), SC_U32(lnx), SC_U32(lny), SC_U32(m_st.m_current_turn));
            break;
        }
    }
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (i == m_flag_i || m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        if (m_atk[i].m_phase != ArmyPhase::Park && m_atk[i].m_phase != ArmyPhase::Rally) {
            continue;
        }
        if (army_handle(i) == ArmyTurnRslt::Busy) {
            any = true;
        }
    }
    return any;
}

bool WarTurnHandler::flatten_muster () {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || m_grp_n == 0u) {
        return false;
    }
    UnitAddKey heads[k_grp_cap];
    u16 hn = 0u;
    for (u16 i = 0; i < m_grp_n; ++i) {
        if (m_grp[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        const UnitAddKey key = UnitAddKey::from_raw(m_grp[i].m_hd);
        const UnitAddStruct* u = m_st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x != m_sx || u->m_y != m_sy) {
            continue;
        }
        heads[hn++] = key;
    }
    if (hn == 0u) {
        return false;
    }
    UnitAddKey flat = UnitAddKey::None();
    if (!UnitMovementMng::flatten_groups(m_st, heads, hn, &flat)) {
        return false;
    }
    m_grp[0].m_hd = flat.value();
    m_grp[0].m_exp = GenerateExposure::k_none;
    m_grp_n = 1u;
    return true;
}

bool WarTurnHandler::form_army () {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || !m_mob_ok) {
        return false;
    }
    UnitAddKey head = UnitAddKey::None();
    if (!UnitGroupManagement::campaign_form_land_army(m_st, m_sx, m_sy, m_seat, &head)) {
        return false;
    }
    UnitAddKey parts[k_grp_use];
    u16 part_n = 0u;
    if (!UnitGroupManagement::partition_land_armies_by_arty(m_st, head, parts, k_grp_use, &part_n)
        || part_n == 0u) {
        return false;
    }
    m_grp_n = 0;
    m_atk_n = 0u;
    m_flag_i = U16_KEY_NULL;
    u32 army_n = 0u;
    u32 size_sum = 0u;
    for (u16 i = 0; i < k_atk_cap; ++i) {
        path_drop(i);
        m_atk[i].m_hd = U16_KEY_NULL;
        m_atk[i].m_split = U16_KEY_NULL;
        m_atk[i].m_tx = m_tx;
        m_atk[i].m_ty = m_ty;
        m_atk[i].m_idx = i;
        m_atk[i].m_front = U16_KEY_NULL;
        m_atk[i].m_phase = ArmyPhase::Idle;
        m_atk[i].m_stall = false;
        m_atk[i].m_br_tot = 0u;
        m_atk[i].m_br_last = 0u;
        m_atk[i].m_assault = WarAssault::Fail;
    }
    for (u16 i = 0; i < part_n && i < k_grp_use; ++i) {
        if (!parts[i].is_valid()) {
            continue;
        }
        UnitMovementMng::set_grp_army(m_st, parts[i], 1u);
        UnitMovementMng::set_grp_campaign(m_st, parts[i], 1u);
        m_atk[i].m_hd = parts[i].value();
        if (m_flag_i == U16_KEY_NULL) {
            m_flag_i = i;
            LOG_WAR_FLAG_SET::LOG(SC_U32(m_seat), SC_U32(m_flag_i), SC_U32(m_st.m_current_turn));
        }
        m_atk_n = static_cast<u16>(i + 1u);
        UnitAddKey cur = parts[i];
        while (cur.is_valid()) {
            const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
            if (u == nullptr) {
                break;
            }
            ++army_n;
            size_sum += static_cast<u32>(u->m_unit_size);
            if (u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
        EVAL_ARMY_STATE_TO_WAR::EVAL(m_st, parts[i].value());
    }
    if (m_atk_n == 0u) {
        return false;
    }
    LOG_WAR_ARMY_SIZE::LOG(SC_U32(m_seat), SC_U32(army_n), SC_U32(size_sum), SC_U32(m_sx), SC_U32(m_sy), SC_U32(m_st.m_current_turn));
    LOG_WAR_ARMY_GROUPS::LOG(SC_U32(m_seat), SC_U32(m_atk_n), SC_U32(m_st.m_current_turn));
    return true;
}

bool WarTurnHandler::find_enemy_seed (u16 enemy, u16* ox, u16* oy) const {
    PROF_TH_WAR_ENTER(__func__);
    if (ox == nullptr || oy == nullptr) {
        LOG_WAR_FIND_ENEMY_SEED_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn));
        return false;
    }
    const u16 cn = m_st.m_cities.get_city_count();
    u32 best = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        const u8 t = m_st.m_map.get_terrain(x, y);
        if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
            continue;
        }
        if (t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0]) {
            continue;
        }
        if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
            continue;
        }
        const u32 adx = m_sx > x ? static_cast<u32>(m_sx - x) : static_cast<u32>(x - m_sx);
        const u32 ady = m_sy > y ? static_cast<u32>(m_sy - y) : static_cast<u32>(y - m_sy);
        const u32 d = adx + ady;
        if (!found || d < best) {
            best = d;
            bx = x;
            by = y;
            found = true;
        }
    }
    if (!found) {
        LOG_WAR_FIND_ENEMY_SEED_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn));
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

bool WarTurnHandler::ensure_war (u16 other) {
    if (other >= m_st.m_player_n || other == m_seat || m_st.m_player_states == nullptr) {
        return false;
    }
    const CivStaticDataKey a = CivStaticDataKey::from_raw(m_seat);
    const CivStaticDataKey b = CivStaticDataKey::from_raw(other);
    if (m_st.m_civ_relations.get(a, b) != CivRel::CIV_REL_WAR) {
        m_st.m_civ_relations.set(a, b, CivRel::CIV_REL_WAR);
    }
    m_st.m_player_states[m_seat].m_at_war = 1u;
    m_enemy = static_cast<u8>(other);
    return true;
}

bool WarTurnHandler::apply_plan (const WarTargetPlan& plan) {
    const u16 n = (plan.m_n < k_tgt_cap) ? plan.m_n : k_tgt_cap;
    for (u16 i = 0; i < n; ++i) {
        m_tgts[i] = plan.m_cities[i];
    }
    m_tgt_n = n;
    m_tgt_i = 0u;
    m_sector = plan.m_sector;
    if (plan.m_enemy < m_st.m_player_n) {
        m_enemy = static_cast<u8>(plan.m_enemy);
    } else {
        m_enemy = U8_KEY_NULL;
    }
    return m_tgt_n > 0u;
}

bool WarTurnHandler::refill_targets (u16 enemy) {
    PROF_TH_WAR_ENTER(__func__);
    (void)enemy;
    m_tgt_n = 0;
    m_tgt_i = 0;
    m_enemy = U8_KEY_NULL;
    if (m_sector != U16_KEY_NULL) {
        WarTargetPlan cur;
        cur.m_enemy = U16_KEY_NULL;
        cur.m_sector = m_sector;
        cur.m_n = 0u;
        if (war_target_flood_sector(m_st, m_seat, m_sector, &cur) && cur.m_n > 0u) {
            return apply_plan(cur);
        }
        LOG_WAR_SECT_DONE::LOG(SC_U32(m_seat), SC_U32(m_sector), SC_U32(m_st.m_current_turn));
        m_sector = U16_KEY_NULL;
    }
    WarTargetPlan plan;
    plan.m_enemy = U16_KEY_NULL;
    plan.m_sector = U16_KEY_NULL;
    plan.m_n = 0u;
    if (!war_target_pick(m_st, m_seat, m_sx, m_sy, &plan) || plan.m_n == 0u) {
        LOG_WAR_REFILL_TARGETS_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 1u);
        return false;
    }
    return apply_plan(plan);
}

bool WarTurnHandler::city_in_use (u16 x, u16 y, u16 skip_army) const {
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (i == skip_army || m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        if (m_atk[i].m_phase == ArmyPhase::Idle) {
            continue;
        }
        if (m_atk[i].m_tx == x && m_atk[i].m_ty == y) {
            return true;
        }
    }
    return false;
}

bool WarTurnHandler::next_enemy_city (u16 enemy, u16* ox, u16* oy) {
    PROF_TH_WAR_ENTER(__func__);
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    if (m_tgt_n == 0u || m_tgt_i >= m_tgt_n) {
        if (!refill_targets(enemy)) {
            return false;
        }
    }
    while (m_tgt_i < m_tgt_n) {
        const u16 cidx = m_tgts[m_tgt_i++];
        City* c = m_st.m_cities.get_city(cidx);
        if (c == nullptr) {
            continue;
        }
        const u16 own = c->get_owner();
        if (own == m_seat || own >= m_st.m_player_n) {
            continue;
        }
        if (m_st.m_player_states != nullptr
            && (m_st.m_player_states[own].m_is_active == 0u
                || m_st.m_player_states[own].m_lucky != 0u)) {
            continue;
        }
        *ox = c->get_x();
        *oy = c->get_y();
        return true;
    }
    return false;
}

bool WarTurnHandler::assign_army_city (u16 army_i, u16 enemy, u16* ox, u16* oy) {
    PROF_TH_WAR_ENTER(__func__);
    m_assign_busy = false;
    if (army_i >= grp_use_n() || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return false;
    }
    if (m_tgt_i >= m_tgt_n) {
        if (!refill_targets(enemy)) {
            return false;
        }
    }
    bool saw_busy = false;
    bool saw_path = false;
    const u16 budget = (m_tgt_n > 0u) ? m_tgt_n : k_tgt_cap;
    for (u16 tries = 0; tries < budget; ++tries) {
        u16 tx = 0u;
        u16 ty = 0u;
        if (!next_enemy_city(enemy, &tx, &ty)) {
            if (m_tgt_n == 0u) {
                return false;
            }
            if (!refill_targets(enemy)) {
                return false;
            }
            continue;
        }
        if (city_in_use(tx, ty, army_i)) {
            saw_busy = true;
            continue;
        }
        if (!set_goal_army(army_i, tx, ty)) {
            saw_path = true;
            continue;
        }
        const u8 own = m_st.m_map.get_civ_owner(tx, ty);
        if (own != U8_KEY_NULL) {
            (void)ensure_war(static_cast<u16>(own));
        }
        m_atk[army_i].m_phase = ArmyPhase::March;
        m_atk[army_i].m_assault = WarAssault::Fail;
        m_atk[army_i].m_stall = false;
        m_tx = tx;
        m_ty = ty;
        if (ox != nullptr) {
            *ox = tx;
        }
        if (oy != nullptr) {
            *oy = ty;
        }
        return true;
    }
    m_assign_busy = saw_busy && !saw_path;
    return false;
}

bool WarTurnHandler::park_army (u16 army_i) {
    PROF_TH_WAR_ENTER(__func__);
    if (army_i >= grp_use_n() || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return false;
    }
    const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[army_i].m_hd));
    if (u != nullptr && u->m_x == m_sx && u->m_y == m_sy) {
        m_atk[army_i].m_tx = m_sx;
        m_atk[army_i].m_ty = m_sy;
        m_atk[army_i].m_phase = ArmyPhase::Idle;
        m_atk[army_i].m_assault = WarAssault::Fail;
        m_atk[army_i].m_stall = false;
        LOG_WAR_PARK::LOG(SC_U32(m_seat), SC_U32(army_i), SC_U32(m_sx), SC_U32(m_sy), SC_U32(m_st.m_current_turn));
        return true;
    }
    if (!set_goal_army(army_i, m_sx, m_sy)) {
        return false;
    }
    m_atk[army_i].m_phase = ArmyPhase::Park;
    m_atk[army_i].m_assault = WarAssault::Fail;
    m_atk[army_i].m_stall = false;
    LOG_WAR_PARK::LOG(SC_U32(m_seat), SC_U32(army_i), SC_U32(m_sx), SC_U32(m_sy), SC_U32(m_st.m_current_turn));
    return true;
}

bool WarTurnHandler::flag_xy (u16* ox, u16* oy) const {
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    if (m_flag_i != U16_KEY_NULL && m_flag_i < k_atk_cap && m_atk[m_flag_i].m_hd != U16_KEY_NULL) {
        const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[m_flag_i].m_hd));
        if (u != nullptr && u->m_x != U16_KEY_NULL) {
            *ox = u->m_x;
            *oy = u->m_y;
            return true;
        }
    }
    *ox = m_sx;
    *oy = m_sy;
    return true;
}

bool WarTurnHandler::merge_into_flag (u16 army_i) {
    PROF_TH_WAR_ENTER(__func__);
    if (m_flag_i == U16_KEY_NULL || army_i == m_flag_i || army_i >= k_atk_cap) {
        return false;
    }
    if (m_atk[m_flag_i].m_hd == U16_KEY_NULL || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return false;
    }
    u16 fx = 0u;
    u16 fy = 0u;
    if (!flag_xy(&fx, &fy)) {
        return false;
    }
    const UnitAddStruct* ou = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[army_i].m_hd));
    if (ou == nullptr || ou->m_x != fx || ou->m_y != fy) {
        return rejoin_flag(army_i);
    }
    UnitAddKey heads[2];
    heads[0] = UnitAddKey::from_raw(m_atk[m_flag_i].m_hd);
    heads[1] = UnitAddKey::from_raw(m_atk[army_i].m_hd);
    UnitAddKey flat = UnitAddKey::None();
    if (!UnitMovementMng::flatten_groups(m_st, heads, 2u, &flat) || !flat.is_valid()) {
        return false;
    }
    UnitMovementMng::set_grp_army(m_st, flat, 1u);
    UnitMovementMng::set_grp_campaign(m_st, flat, 1u);
    m_atk[m_flag_i].m_hd = flat.value();
    m_atk[m_flag_i].m_split = U16_KEY_NULL;
    m_atk[army_i].m_hd = U16_KEY_NULL;
    m_atk[army_i].m_split = U16_KEY_NULL;
    m_atk[army_i].m_phase = ArmyPhase::Idle;
    m_atk[army_i].m_front = U16_KEY_NULL;
    LOG_WAR_MERGE_FLAG::LOG(
        SC_U32(m_seat), SC_U32(army_i), SC_U32(m_flag_i), SC_U32(m_st.m_current_turn));
    return true;
}

bool WarTurnHandler::rejoin_flag (u16 army_i) {
    PROF_TH_WAR_ENTER(__func__);
    if (army_i >= grp_use_n() || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return false;
    }
    if (m_flag_i == U16_KEY_NULL || army_i == m_flag_i || m_atk[m_flag_i].m_hd == U16_KEY_NULL) {
        return park_army(army_i);
    }
    u16 fx = 0u;
    u16 fy = 0u;
    if (!flag_xy(&fx, &fy)) {
        return park_army(army_i);
    }
    const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[army_i].m_hd));
    if (u != nullptr && u->m_x == fx && u->m_y == fy) {
        return merge_into_flag(army_i);
    }
    if (!set_goal_army(army_i, fx, fy)) {
        return park_army(army_i);
    }
    m_atk[army_i].m_phase = ArmyPhase::Rally;
    m_atk[army_i].m_assault = WarAssault::Fail;
    m_atk[army_i].m_stall = false;
    LOG_WAR_RALLY::LOG(SC_U32(m_seat), SC_U32(army_i), SC_U32(fx), SC_U32(fy), SC_U32(m_st.m_current_turn));
    return true;
}

bool WarTurnHandler::absorb_muster_to_flag () {
    PROF_TH_WAR_ENTER(__func__);
    if (m_flag_i == U16_KEY_NULL || m_flag_i >= k_atk_cap || m_atk[m_flag_i].m_hd == U16_KEY_NULL) {
        return false;
    }
    UnitAddKey heads[k_grp_cap];
    u16 hn = 0u;
    heads[hn++] = UnitAddKey::from_raw(m_atk[m_flag_i].m_hd);
    for (u16 i = 0; i < m_grp_n && hn < k_grp_cap; ++i) {
        if (m_grp[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        const UnitAddKey key = UnitAddKey::from_raw(m_grp[i].m_hd);
        const UnitAddStruct* u = m_st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x != m_sx || u->m_y != m_sy) {
            UnitMovementMng::set_grp_campaign(m_st, key, 0u);
            continue;
        }
        heads[hn++] = key;
    }
    if (hn <= 1u) {
        m_grp_n = 0u;
        return hn == 1u;
    }
    UnitAddKey flat = UnitAddKey::None();
    if (!UnitMovementMng::flatten_groups(m_st, heads, hn, &flat) || !flat.is_valid()) {
        m_grp_n = 0u;
        return false;
    }
    UnitMovementMng::set_grp_army(m_st, flat, 1u);
    UnitMovementMng::set_grp_campaign(m_st, flat, 1u);
    m_atk[m_flag_i].m_hd = flat.value();
    m_atk[m_flag_i].m_split = U16_KEY_NULL;
    m_grp_n = 0u;
    return true;
}

bool WarTurnHandler::queue_pending () const {
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        if (m_atk[i].m_phase == ArmyPhase::Park || m_atk[i].m_phase == ArmyPhase::Rally) {
            return true;
        }
    }
    return false;
}

bool WarTurnHandler::set_target_city (u16 enemy, u16* ox, u16* oy) {
    PROF_TH_WAR_ENTER(__func__);
    m_ready = false;
    if (!ok() || enemy >= m_st.m_player_n || enemy == m_seat) {
        LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 0u);
        return false;
    }
    bool any = false;
    u16 first_tx = 0u;
    u16 first_ty = 0u;
    if (m_flag_i != U16_KEY_NULL && m_flag_i < grp_use_n() && m_atk[m_flag_i].m_hd != U16_KEY_NULL) {
        u16 tx = 0u;
        u16 ty = 0u;
        if (assign_army_city(m_flag_i, enemy, &tx, &ty)) {
            first_tx = tx;
            first_ty = ty;
            any = true;
        } else {
            LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 2u);
            return false;
        }
    }
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (i == m_flag_i || m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        u16 tx = 0u;
        u16 ty = 0u;
        if (assign_army_city(i, enemy, &tx, &ty)) {
            if (!any) {
                first_tx = tx;
                first_ty = ty;
                any = true;
            }
            continue;
        }
        if (!rejoin_flag(i) && !any) {
            LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 2u);
            return false;
        }
    }
    if (!any) {
        LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 2u);
        return false;
    }
    m_ready = true;
    if (ox != nullptr) {
        *ox = first_tx;
    }
    if (oy != nullptr) {
        *oy = first_ty;
    }
    return true;
}

bool WarTurnHandler::retarget_idle (u16 enemy, u16* ox, u16* oy) {
    PROF_TH_WAR_ENTER(__func__);
    m_ready = false;
    if (!ok() || enemy >= m_st.m_player_n || enemy == m_seat) {
        LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 0u);
        return false;
    }
    bool any = false;
    bool got_enemy = false;
    u16 first_tx = 0u;
    u16 first_ty = 0u;
    auto drop = [this] (u16 i) {
        UnitMovementMng::set_grp_campaign(m_st, UnitAddKey::from_raw(m_atk[i].m_hd), 0u);
        UnitMovementMng::set_grp_army(m_st, UnitAddKey::from_raw(m_atk[i].m_hd), 0u);
        drop_army(i);
    };
    auto try_one = [&] (u16 i) {
        if (m_atk[i].m_hd == U16_KEY_NULL || m_atk[i].m_phase != ArmyPhase::Idle) {
            return;
        }
        u16 tx = 0u;
        u16 ty = 0u;
        if (assign_army_city(i, enemy, &tx, &ty)) {
            if (!any) {
                first_tx = tx;
                first_ty = ty;
            }
            any = true;
            got_enemy = true;
            return;
        }
        if (m_assign_busy) {
            any = true;
            return;
        }
        if (i != m_flag_i) {
            if (rejoin_flag(i)) {
                any = true;
                return;
            }
        }
        drop(i);
    };
    if (m_flag_i != U16_KEY_NULL && m_flag_i < grp_use_n()) {
        try_one(m_flag_i);
    }
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (i == m_flag_i) {
            continue;
        }
        try_one(i);
    }
    if (m_flag_i == U16_KEY_NULL) {
        for (u16 i = 0; i < grp_use_n(); ++i) {
            if (m_atk[i].m_hd != U16_KEY_NULL) {
                m_flag_i = i;
                break;
            }
        }
    }
    if (!got_enemy && queue_pending()) {
        m_ready = true;
        if (ox != nullptr) {
            *ox = m_tx;
        }
        if (oy != nullptr) {
            *oy = m_ty;
        }
        return true;
    }
    if (!got_enemy && m_assign_busy && any) {
        m_ready = true;
        if (ox != nullptr) {
            *ox = m_tx;
        }
        if (oy != nullptr) {
            *oy = m_ty;
        }
        return true;
    }
    if (!any || !got_enemy) {
        LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 2u);
        return false;
    }
    m_ready = true;
    if (ox != nullptr) {
        *ox = first_tx;
    }
    if (oy != nullptr) {
        *oy = first_ty;
    }
    return true;
}

ArmyTurnRslt WarTurnHandler::army_handle (u16 army_i) {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || army_i >= grp_use_n() || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return ArmyTurnRslt::Stop;
    }
    const bool rallying = (m_atk[army_i].m_phase == ArmyPhase::Rally);
    const bool parking = (m_atk[army_i].m_phase == ArmyPhase::Park);
    const ArmyTurnRslt r = ArmyTurnHandler::handle(m_st, m_seat, m_enemy, m_atk[army_i]);
    if ((rallying || parking) && r == ArmyTurnRslt::NeedTarget) {
        if (merge_into_flag(army_i)) {
            return ArmyTurnRslt::Busy;
        }
        if (rejoin_flag(army_i)) {
            return ArmyTurnRslt::Busy;
        }
    }
    if (m_atk[army_i].m_phase == ArmyPhase::Rejoin || r == ArmyTurnRslt::NeedTarget) {
        sync_from_army(army_i);
    }
    return r;
}

void WarTurnHandler::sync_from_army (u16 army_i) {
    if (army_i >= k_atk_cap) {
        return;
    }
    m_tx = m_atk[army_i].m_tx;
    m_ty = m_atk[army_i].m_ty;
    if (army_i != m_flag_i) {
        return;
    }
    if (m_sx == m_atk[army_i].m_tx && m_sy == m_atk[army_i].m_ty) {
        return;
    }
    m_sx = m_atk[army_i].m_tx;
    m_sy = m_atk[army_i].m_ty;
    LOG_WAR_MUSTER_PULL::LOG(SC_U32(m_seat), SC_U32(m_sx), SC_U32(m_sy), SC_U32(m_st.m_current_turn));
}

bool WarTurnHandler::stalled () const {
    return m_atk_n > 0u && m_atk[0].m_stall;
}

u32 WarTurnHandler::br_tot () const {
    return (m_atk_n > 0u) ? m_atk[0].m_br_tot : 0u;
}

u32 WarTurnHandler::br_last () const {
    return (m_atk_n > 0u) ? m_atk[0].m_br_last : 0u;
}

bool WarTurnHandler::pick_staging_city (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy) {
    return WarStagingSelector::pick(s, seat, enemy, ox, oy);
}

u16 WarTurnHandler::muster_n () const {
    return m_grp_n;
}

void WarTurnHandler::muster_units (u32* unit_n, u32* size_sum) const {
    u32 un = 0u;
    u32 sz = 0u;
    for (u16 i = 0; i < m_grp_n; ++i) {
        UnitAddKey cur = UnitAddKey::from_raw(m_grp[i].m_hd);
        while (cur.is_valid()) {
            const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
            if (u == nullptr) {
                break;
            }
            ++un;
            sz += static_cast<u32>(u->m_unit_size);
            if (u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
    }
    if (unit_n != nullptr) {
        *unit_n = un;
    }
    if (size_sum != nullptr) {
        *size_sum = sz;
    }
}

bool WarTurnHandler::is_exposed (u16 i) const {
    return i < m_grp_n && m_grp[i].m_exp < k_exp_lim;
}

WarArmy* WarTurnHandler::army (u16 i) {
    return (i < k_atk_cap) ? &m_atk[i] : nullptr;
}

const WarArmy* WarTurnHandler::army (u16 i) const {
    return (i < k_atk_cap) ? &m_atk[i] : nullptr;
}

u16 WarTurnHandler::atk_hd (u16 i) const {
    return (i < k_atk_cap) ? m_atk[i].m_hd : U16_KEY_NULL;
}

u16 WarTurnHandler::split_hd (u16 i) const {
    return (i < k_atk_cap) ? m_atk[i].m_split : U16_KEY_NULL;
}

void WarTurnHandler::clr_campaign () {
    m_tgt_n = 0u;
    m_tgt_i = 0u;
    m_sector = U16_KEY_NULL;
    m_assign_busy = false;
    for (u16 i = 0; i < m_grp_n; ++i) {
        if (m_grp[i].m_hd != U16_KEY_NULL) {
            EVAL_ARMY_STATE_PEACE::EVAL(m_st, m_grp[i].m_hd);
            UnitMovementMng::set_grp_campaign(m_st, UnitAddKey::from_raw(m_grp[i].m_hd), 0u);
        }
    }
    for (u16 i = 0; i < k_atk_cap; ++i) {
        if (m_atk[i].m_hd != U16_KEY_NULL) {
            EVAL_ARMY_STATE_PEACE::EVAL(m_st, m_atk[i].m_hd);
            UnitMovementMng::set_grp_campaign(m_st, UnitAddKey::from_raw(m_atk[i].m_hd), 0u);
        }
        if (m_atk[i].m_split != U16_KEY_NULL) {
            EVAL_ARMY_STATE_PEACE::EVAL(m_st, m_atk[i].m_split);
            UnitMovementMng::set_grp_campaign(m_st, UnitAddKey::from_raw(m_atk[i].m_split), 0u);
        }
        path_drop(i);
    }
}

void WarTurnHandler::clr_army_flags () {
    for (u16 i = 0; i < m_grp_n; ++i) {
        if (m_grp[i].m_hd != U16_KEY_NULL) {
            UnitMovementMng::set_grp_army(m_st, UnitAddKey::from_raw(m_grp[i].m_hd), 0u);
        }
    }
    for (u16 i = 0; i < k_atk_cap; ++i) {
        if (m_atk[i].m_hd != U16_KEY_NULL) {
            UnitMovementMng::set_grp_army(m_st, UnitAddKey::from_raw(m_atk[i].m_hd), 0u);
        }
        if (m_atk[i].m_split != U16_KEY_NULL) {
            UnitMovementMng::set_grp_army(m_st, UnitAddKey::from_raw(m_atk[i].m_split), 0u);
        }
    }
}

u32 WarTurnHandler::army_size (u16 army_i) const {
    if (army_i >= k_atk_cap || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return 0u;
    }
    u32 sz = 0u;
    UnitAddKey cur = UnitAddKey::from_raw(m_atk[army_i].m_hd);
    while (cur.is_valid()) {
        const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        sz += static_cast<u32>(u->m_unit_size);
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return sz;
}

u16 WarTurnHandler::weakest_army () const {
    u16 best = U16_KEY_NULL;
    u32 best_sz = 0xFFFFFFFFu;
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (m_atk[i].m_hd == U16_KEY_NULL || m_atk[i].m_front == U16_KEY_NULL) {
            continue;
        }
        const u32 sz = army_size(i);
        if (sz < best_sz) {
            best_sz = sz;
            best = i;
        }
    }
    return best;
}

bool WarTurnHandler::bind_standing (u16 i, u16 hd, u16 front) {
    if (i >= k_grp_use || hd == U16_KEY_NULL) {
        return false;
    }
    const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(hd));
    if (u == nullptr) {
        return false;
    }
    path_drop(i);
    m_atk[i].m_hd = hd;
    m_atk[i].m_split = U16_KEY_NULL;
    m_atk[i].m_front = front;
    m_atk[i].m_idx = i;
    m_atk[i].m_phase = ArmyPhase::Idle;
    m_atk[i].m_stall = false;
    m_atk[i].m_br_tot = 0u;
    m_atk[i].m_br_last = 0u;
    m_atk[i].m_assault = WarAssault::Fail;
    UnitMovementMng::set_grp_army(m_st, UnitAddKey::from_raw(hd), 1u);
    UnitMovementMng::set_grp_campaign(m_st, UnitAddKey::from_raw(hd), 1u);
    if (static_cast<u16>(i + 1u) > m_atk_n) {
        m_atk_n = static_cast<u16>(i + 1u);
    }
    if (m_flag_i == U16_KEY_NULL) {
        m_flag_i = i;
        LOG_WAR_FLAG_SET::LOG(SC_U32(m_seat), SC_U32(m_flag_i), SC_U32(m_st.m_current_turn));
    }
    return true;
}

u16 WarTurnHandler::flag_i () const {
    return m_flag_i;
}

void WarTurnHandler::drop_army (u16 army_i) {
    if (army_i >= k_atk_cap) {
        return;
    }
    path_drop(army_i);
    m_atk[army_i].m_hd = U16_KEY_NULL;
    m_atk[army_i].m_split = U16_KEY_NULL;
    m_atk[army_i].m_phase = ArmyPhase::Idle;
    m_atk[army_i].m_front = U16_KEY_NULL;
    if (m_flag_i != army_i) {
        return;
    }
    m_flag_i = U16_KEY_NULL;
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (m_atk[i].m_hd != U16_KEY_NULL) {
            m_flag_i = i;
            break;
        }
    }
}

bool WarTurnHandler::has_armies () const {
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (m_atk[i].m_hd != U16_KEY_NULL) {
            return true;
        }
    }
    return false;
}

bool WarTurnHandler::tick_reassemble (BorderFront* fronts, u16 n) {
    if (fronts == nullptr || n == 0u) {
        return false;
    }
    bool need = false;
    bool moved = false;
    for (u16 f = 0; f < n; ++f) {
        if (!BorderTurnHandler::slot_city(fronts[f].m_city)) {
            continue;
        }
        const City* c = m_st.m_cities.get_city(fronts[f].m_city);
        if (c == nullptr || c->get_owner() != m_seat) {
            continue;
        }
        u16 heads[k_grp_use];
        u16 hn = 0u;
        for (u16 i = 0; i < grp_use_n(); ++i) {
            if (m_atk[i].m_hd == U16_KEY_NULL || m_atk[i].m_front != f) {
                continue;
            }
            heads[hn++] = m_atk[i].m_hd;
        }
        if (hn == 0u) {
            continue;
        }
        ArmyReassembler re;
        if (!re.begin(m_st, m_seat, c->get_x(), c->get_y())) {
            continue;
        }
        if (re.at_goal(m_st, heads, hn)) {
            continue;
        }
        need = true;
        if (re.tick(m_st, m_seat, heads, hn)) {
            moved = true;
        }
    }
    return need && moved;
}

bool WarTurnHandler::try_peel_fronts (BorderFront* fronts, u16 n) {
    (void)fronts;
    (void)n;
    return false;
}

bool WarTurnHandler::start_reinforce (BorderFront* fronts, u16 n) {
    if (fronts == nullptr || n == 0u || m_re_i != U16_KEY_NULL) {
        return false;
    }
    const u16 wi = weakest_army();
    if (wi == U16_KEY_NULL) {
        return false;
    }
    const u16 fi = m_atk[wi].m_front;
    if (fi >= n || !BorderTurnHandler::slot_city(fronts[fi].m_city)) {
        return false;
    }
    const City* c = m_st.m_cities.get_city(fronts[fi].m_city);
    if (c == nullptr || c->get_owner() != m_seat) {
        return false;
    }
    m_hold_sx = m_sx;
    m_hold_sy = m_sy;
    if (!make_muster_gradient(c->get_x(), c->get_y())) {
        m_sx = m_hold_sx;
        m_sy = m_hold_sy;
        return false;
    }
    if (do_total_muster() == 0u) {
        m_sx = m_hold_sx;
        m_sy = m_hold_sy;
        m_mob_ok = false;
        return false;
    }
    m_re_i = wi;
    return true;
}

bool WarTurnHandler::finish_reinforce () {
    if (m_re_i == U16_KEY_NULL || m_re_i >= k_atk_cap || m_atk[m_re_i].m_hd == U16_KEY_NULL) {
        for (u16 i = 0; i < m_grp_n; ++i) {
            if (m_grp[i].m_hd != U16_KEY_NULL) {
                UnitMovementMng::set_grp_campaign(m_st, UnitAddKey::from_raw(m_grp[i].m_hd), 0u);
            }
        }
        m_re_i = U16_KEY_NULL;
        m_grp_n = 0u;
        m_sx = m_hold_sx;
        m_sy = m_hold_sy;
        return false;
    }
    UnitAddKey heads[k_grp_cap];
    u16 hn = 0u;
    heads[hn++] = UnitAddKey::from_raw(m_atk[m_re_i].m_hd);
    for (u16 i = 0; i < m_grp_n && hn < k_grp_cap; ++i) {
        if (m_grp[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        const UnitAddKey key = UnitAddKey::from_raw(m_grp[i].m_hd);
        const UnitAddStruct* u = m_st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x != m_sx || u->m_y != m_sy) {
            UnitMovementMng::set_grp_campaign(m_st, key, 0u);
            continue;
        }
        heads[hn++] = key;
    }
    if (hn > 1u) {
        UnitAddKey flat = UnitAddKey::None();
        if (UnitMovementMng::flatten_groups(m_st, heads, hn, &flat) && flat.is_valid()) {
            UnitMovementMng::set_grp_army(m_st, flat, 1u);
            UnitMovementMng::set_grp_campaign(m_st, flat, 1u);
            m_atk[m_re_i].m_hd = flat.value();
        }
    }
    m_grp_n = 0u;
    m_re_i = U16_KEY_NULL;
    m_sx = m_hold_sx;
    m_sy = m_hold_sy;
    return true;
}

bool WarTurnHandler::tick_reinforce (BorderFront* fronts, u16 n) {
    if (m_re_i == U16_KEY_NULL) {
        return start_reinforce(fronts, n);
    }
    if (walk_muster()) {
        return true;
    }
    (void)finish_reinforce();
    return false;
}

u16 WarTurnHandler::seat () const {
    return m_seat;
}

u16 WarTurnHandler::enemy () const {
    return m_enemy;
}

u16 WarTurnHandler::staging_x () const {
    return m_sx;
}

u16 WarTurnHandler::staging_y () const {
    return m_sy;
}

u16 WarTurnHandler::target_x () const {
    return m_tx;
}

u16 WarTurnHandler::target_y () const {
    return m_ty;
}

//================================================================================================================================
//= Eval prep =
//================================================================================================================================

#if defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(ENABLED_EVAL_ARMY_STATE_MUSTER_GROUP))
void prep_eval_muster_grps (const WarTurnHandler& camp) {
    u16 heads[WarTurnHandler::k_grp_cap];
    u16 n = 0u;
    for (u16 i = 0; i < camp.m_grp_n; ++i) {
        if (camp.m_grp[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        heads[n++] = camp.m_grp[i].m_hd;
    }
    EVAL_ARMY_STATE_MUSTER_GROUP::EVAL(camp.m_st, camp.m_sx, camp.m_sy, heads, n);
}
#else
void prep_eval_muster_grps (const WarTurnHandler& camp) {
    (void)camp;
}
#endif

//================================================================================================================================
//=> - Hot-path orchestration -
//================================================================================================================================

enum class WarPhase : u8 {
    Idle = 0,
    Muster = 1,
    Form = 2,
    Army = 3,
    Retarget = 4,
    Reassemble = 5,
    BuildUp = 6
};

struct WarSlot {
    WarTurnHandler* m_camp;
    MockMusterSiege* m_mock;
    u16 m_enemy;
    u16 m_front_i; // Index into g_fronts[seat]; U16_KEY_NULL if none
    WarPhase m_phase;
};

struct StandArmy {
    u16 m_hd;
    u16 m_front;
};

static WarSlot g_war[WarTurnHandler::k_seat_cap];
static BorderFront g_fronts[WarTurnHandler::k_seat_cap][WarTurnHandler::k_front_use];
static StandArmy g_stand[WarTurnHandler::k_seat_cap][WarTurnHandler::k_grp_use];
static GameState* g_war_st = nullptr;
static u16 g_war_n = 0u;
static GenLandSectors* g_gls = nullptr;
static LandSectorSeeds g_seeds = {};
static LandSectorNetwork* g_net = nullptr;
static LandSectorAdj* g_adj = nullptr;
static ExpansionState g_exp[WarTurnHandler::k_seat_cap];
static const u32 k_sec_rng = 43u;

static void fronts_idle_seat (u16 seat) {
    if (seat >= WarTurnHandler::k_seat_cap) {
        return;
    }
    for (u16 i = 0; i < WarTurnHandler::k_front_use; ++i) {
        g_fronts[seat][i].m_city = BorderTurnHandler::k_front_idle;
        g_fronts[seat][i].m_pl = U16_KEY_NULL;
    }
}

static void fronts_idle_all (u16 n) {
    const u16 lim = (n < WarTurnHandler::k_seat_cap) ? n : WarTurnHandler::k_seat_cap;
    for (u16 s = 0; s < lim; ++s) {
        fronts_idle_seat(s);
    }
}

static void stand_idle_seat (u16 seat) {
    if (seat >= WarTurnHandler::k_seat_cap) {
        return;
    }
    for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
        g_stand[seat][i].m_hd = U16_KEY_NULL;
        g_stand[seat][i].m_front = U16_KEY_NULL;
    }
}

static void stand_idle_all (u16 n) {
    const u16 lim = (n < WarTurnHandler::k_seat_cap) ? n : WarTurnHandler::k_seat_cap;
    for (u16 s = 0; s < lim; ++s) {
        stand_idle_seat(s);
    }
}

static void stand_save (u16 seat, WarTurnHandler* camp) {
    stand_idle_seat(seat);
    if (camp == nullptr || seat >= WarTurnHandler::k_seat_cap) {
        return;
    }
    for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
        const WarArmy* a = camp->army(i);
        if (a == nullptr || a->m_hd == U16_KEY_NULL) {
            continue;
        }
        g_stand[seat][i].m_hd = a->m_hd;
        g_stand[seat][i].m_front = a->m_front;
    }
}

static void stand_clear_front (GameState& st, u16 seat, u16 fi) {
    if (seat >= WarTurnHandler::k_seat_cap) {
        return;
    }
    for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
        if (g_stand[seat][i].m_front != fi || g_stand[seat][i].m_hd == U16_KEY_NULL) {
            continue;
        }
        UnitMovementMng::set_grp_army(st, UnitAddKey::from_raw(g_stand[seat][i].m_hd), 0u);
        g_stand[seat][i].m_hd = U16_KEY_NULL;
        g_stand[seat][i].m_front = U16_KEY_NULL;
    }
}

static u16 stand_load (WarTurnHandler& camp, u16 seat) {
    u16 n = 0u;
    if (seat >= WarTurnHandler::k_seat_cap) {
        return 0u;
    }
    for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
        const u16 hd = g_stand[seat][i].m_hd;
        if (hd == U16_KEY_NULL) {
            continue;
        }
        if (!camp.bind_standing(i, hd, g_stand[seat][i].m_front)) {
            g_stand[seat][i].m_hd = U16_KEY_NULL;
            g_stand[seat][i].m_front = U16_KEY_NULL;
            continue;
        }
        n = static_cast<u16>(n + 1u);
    }
    return n;
}

static u16 front_find_pl (u16 seat, u16 pl) {
    if (seat >= WarTurnHandler::k_seat_cap || pl == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    for (u16 i = 0; i < WarTurnHandler::k_front_use; ++i) {
        if (!BorderTurnHandler::slot_city(g_fronts[seat][i].m_city)) {
            continue;
        }
        if (g_fronts[seat][i].m_pl == pl) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 front_find_idle (u16 seat) {
    if (seat >= WarTurnHandler::k_seat_cap) {
        return U16_KEY_NULL;
    }
    for (u16 i = 0; i < WarTurnHandler::k_front_use; ++i) {
        if (BorderTurnHandler::slot_idle(g_fronts[seat][i].m_city)) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 city_at_xy (const GameState& st, u16 seat, u16 x, u16 y) {
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        if (c->get_x() == x && c->get_y() == y) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool front_ensure (GameState& st, u16 seat, u16 enemy, u16* out_fi) {
    if (out_fi == nullptr || seat >= WarTurnHandler::k_seat_cap) {
        return false;
    }
    u16 fi = front_find_pl(seat, enemy);
    if (fi != U16_KEY_NULL) {
        *out_fi = fi;
        return true;
    }
    fi = front_find_idle(seat);
    if (fi == U16_KEY_NULL) {
        return false;
    }
    g_fronts[seat][fi].m_city = BorderTurnHandler::k_front_need;
    g_fronts[seat][fi].m_pl = U16_KEY_NULL;
    BorderTurnHandler bth;
    if (!bth.begin(st.m_map, st.m_cities) || !bth.handle(seat, g_fronts[seat], WarTurnHandler::k_front_use)) {
        g_fronts[seat][fi].m_city = BorderTurnHandler::k_front_idle;
        g_fronts[seat][fi].m_pl = U16_KEY_NULL;
        return false;
    }
    const u16 matched = front_find_pl(seat, enemy);
    if (matched != U16_KEY_NULL) {
        *out_fi = matched;
        return true;
    }
    if (BorderTurnHandler::slot_city(g_fronts[seat][fi].m_city)) {
        g_fronts[seat][fi].m_pl = enemy;
        *out_fi = fi;
        return true;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    if (!WarTurnHandler::pick_staging_city(st, seat, enemy, &sx, &sy)) {
        g_fronts[seat][fi].m_city = BorderTurnHandler::k_front_idle;
        g_fronts[seat][fi].m_pl = U16_KEY_NULL;
        return false;
    }
    const u16 ci = city_at_xy(st, seat, sx, sy);
    if (ci == U16_KEY_NULL) {
        g_fronts[seat][fi].m_city = BorderTurnHandler::k_front_idle;
        g_fronts[seat][fi].m_pl = U16_KEY_NULL;
        return false;
    }
    g_fronts[seat][fi].m_city = ci;
    g_fronts[seat][fi].m_pl = enemy;
    *out_fi = fi;
    return true;
}

static bool front_staging_xy (const GameState& st, u16 seat, u16 fi, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr || seat >= WarTurnHandler::k_seat_cap || fi >= WarTurnHandler::k_front_use) {
        return false;
    }
    if (!BorderTurnHandler::slot_city(g_fronts[seat][fi].m_city)) {
        return false;
    }
    const City* c = st.m_cities.get_city(g_fronts[seat][fi].m_city);
    if (c == nullptr || c->get_owner() != seat) {
        return false;
    }
    *ox = c->get_x();
    *oy = c->get_y();
    return true;
}

static void front_clear (u16 seat, u16 fi) {
    if (seat >= WarTurnHandler::k_seat_cap || fi >= WarTurnHandler::k_front_use) {
        return;
    }
    g_fronts[seat][fi].m_city = BorderTurnHandler::k_front_idle;
    g_fronts[seat][fi].m_pl = U16_KEY_NULL;
}

static void war_sec_clr () {
    SectorPathGradients::pool_teardown();
    SectorSupport::clr();
    for (u16 i = 0; i < WarTurnHandler::k_seat_cap; ++i) {
        g_exp[i].clr();
    }
    GenLandSectors::free_seeds(&g_seeds);
    delete g_gls;
    g_gls = nullptr;
    delete g_net;
    g_net = nullptr;
    delete g_adj;
    g_adj = nullptr;
}

static bool war_exp_begin_seat (u16 seat, u32 rng) {
    if (g_adj == nullptr || !g_adj->ok() || seat >= WarTurnHandler::k_seat_cap) {
        return false;
    }
    switch (sect_select_get_kind()) {
    case SectSelectKind::Walk:
        return SectorSelect_Walk::begin(g_exp[seat], *g_adj, g_seeds.m_pts, g_seeds.m_n, rng);
    case SectSelectKind::Defensible:
    default:
        return SectorSelect_Defensible::begin(g_exp[seat], *g_adj, g_seeds.m_pts, g_seeds.m_n, rng);
    }
}

static bool war_sec_begin (GameState& st) {
    PROF_TH_WAR_ENTER(__func__);
    war_sec_clr();
    g_gls = new GenLandSectors();
    if (g_gls == nullptr || !g_gls->begin(st.m_map)) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 0u);
        war_sec_clr();
        return false;
    }
    if (!g_gls->gen_seeds(k_sec_rng, &g_seeds) || g_seeds.m_n == 0u) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 1u);
        war_sec_clr();
        return false;
    }
    g_net = new LandSectorNetwork();
    if (g_net == nullptr
        || !GenLandSectorNetwork::build(g_gls->sectors(), g_gls->sector_n(), st.m_map, g_net)
        || !g_net->ok()) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 2u);
        war_sec_clr();
        return false;
    }
    g_adj = new LandSectorAdj();
    if (g_adj == nullptr || !g_adj->build(*g_net, g_gls->sector_n())) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 3u);
        war_sec_clr();
        return false;
    }
    if (!SectorSupport::bind(&st, &g_gls->sectors(), &g_seeds, g_net)) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 4u);
        war_sec_clr();
        return false;
    }
    if (!SectorPathGradients::pool_setup(g_gls->sectors(), g_gls->sector_n())) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 4u);
        war_sec_clr();
        return false;
    }
    if (st.m_player_states == nullptr) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 5u);
        war_sec_clr();
        return false;
    }
    for (u16 s = 0; s < st.m_player_n && s < WarTurnHandler::k_seat_cap; ++s) {
        if (st.m_player_states[s].m_lucky == 0u) {
            continue;
        }
        const u32 rng = k_sec_rng + 0x9E3779B9u * static_cast<u32>(s + 1u);
        if (!war_exp_begin_seat(s, rng)) {
            LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 6u);
            war_sec_clr();
            return false;
        }
        LOG_WAR_SECT_BEGIN::LOG(SC_U32(s), SC_U32(g_exp[s].start()), SC_U32(st.m_current_turn));
    }
    war_target_set_kind(WarTargetKind::SectorFlood);
    return true;
}

ExpansionState* WarTurnHandler::sec_exp (u16 seat) {
    if (seat >= k_seat_cap || !g_exp[seat].ok()) {
        return nullptr;
    }
    return &g_exp[seat];
}

const LandSectorAdj* WarTurnHandler::sec_adj () {
    return (g_adj != nullptr && g_adj->ok()) ? g_adj : nullptr;
}

const LandSectorSeedPt* WarTurnHandler::sec_pts () {
    return (g_seeds.m_pts != nullptr && g_seeds.m_n > 0u) ? g_seeds.m_pts : nullptr;
}

const Whiteboard_2B* WarTurnHandler::sec_wb () {
    if (g_gls == nullptr || !g_gls->ok()) {
        return nullptr;
    }
    return &g_gls->sectors();
}

u16 WarTurnHandler::sec_n () {
    return (g_gls != nullptr && g_gls->ok()) ? g_gls->sector_n() : 0u;
}

static void war_slot_reset (WarSlot* s) {
    if (s == nullptr) {
        return;
    }
    if (s->m_camp != nullptr) {
        s->m_camp->clr_campaign();
    }
    delete s->m_camp;
    s->m_camp = nullptr;
    delete s->m_mock;
    s->m_mock = nullptr;
    s->m_enemy = U16_KEY_NULL;
    s->m_front_i = U16_KEY_NULL;
    s->m_phase = WarPhase::Idle;
}

static void war_finish (GameState& st, u16 seat, WarSlot* s, bool no_tgts) {
    if (s == nullptr) {
        return;
    }
    const u16 enemy = s->m_enemy;
    for (u16 i = 0; i < st.m_player_n; ++i) {
        if (i == seat) {
            continue;
        }
        if (st.m_civ_relations.get(CivStaticDataKey::from_raw(seat), CivStaticDataKey::from_raw(i))
            != CivRel::CIV_REL_WAR) {
            continue;
        }
        if (i == enemy) {
            (void)NegotiatePeace::deal(st, seat, i);
        } else {
            st.m_civ_relations.set(
                CivStaticDataKey::from_raw(seat),
                CivStaticDataKey::from_raw(i),
                CivRel::CIV_REL_PEACE);
        }
    }
    if (st.m_player_states != nullptr && seat < st.m_player_n) {
        st.m_player_states[seat].m_at_war = 0u;
    }
    if (no_tgts) {
        if (s->m_camp != nullptr) {
            for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
                const WarArmy* a = s->m_camp->army(i);
                if (a != nullptr && a->m_hd != U16_KEY_NULL) {
                    stand_clear_front(st, seat, a->m_front);
                    front_clear(seat, a->m_front);
                }
            }
            s->m_camp->clr_army_flags();
        }
        stand_clear_front(st, seat, s->m_front_i);
        front_clear(seat, s->m_front_i);
    } else {
        stand_save(seat, s->m_camp);
    }
    war_slot_reset(s);
}

static void war_peace_mock (GameState& st, u16 seat, WarSlot* s, u16 tx, u16 ty) {
    LOG_WAR_PEACE_MOCK::LOG(SC_U32(seat), SC_U32(tx), SC_U32(ty), SC_U32(st.m_current_turn));
    war_finish(st, seat, s, false);
}

static bool war_begin_buildup (GameState& st, u16 seat, WarSlot* s, u16 tx, u16 ty) {
    PROF_TH_WAR_ENTER(__func__);
    if (s == nullptr || s->m_camp == nullptr) {
        return false;
    }
    WarTurnHandler& camp = *s->m_camp;
    LOG_WAR_SIEGE_FAIL_BUILDUP::LOG(
        SC_U32(seat), SC_U32(s->m_enemy), SC_U32(tx), SC_U32(ty), SC_U32(st.m_current_turn));
    const u16 sx = camp.staging_x();
    const u16 sy = camp.staging_y();
    if (!camp.make_muster_gradient(sx, sy)) {
        return false;
    }
    (void)camp.do_total_muster();
    s->m_phase = WarPhase::BuildUp;
    return true;
}

static bool war_try_mocks (GameState& st, u16 seat, WarSlot* s, WarTurnHandler& camp) {
    PROF_TH_WAR_ENTER(__func__);
    if (s == nullptr || s->m_mock == nullptr || !s->m_mock->ok()) {
        war_peace_mock(st, seat, s, camp.target_x(), camp.target_y());
        return false;
    }
    if (camp.prep_mocks(*s->m_mock)) {
        return true;
    }
    u16 tx = camp.target_x();
    u16 ty = camp.target_y();
    if (camp.flag_i() != U16_KEY_NULL) {
        WarArmy* fl = camp.army(camp.flag_i());
        if (fl != nullptr && fl->m_hd != U16_KEY_NULL) {
            tx = fl->m_tx;
            ty = fl->m_ty;
        }
    }
    if (!war_begin_buildup(st, seat, s, tx, ty)) {
        war_peace_mock(st, seat, s, tx, ty);
    }
    return false;
}

bool WarTurnHandler::prep_mocks (MockMusterSiege& mock) {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || !mock.ok()) {
        return false;
    }
    static const u16 k_min_barrage = 10u;
    if (mock.barrage_n(m_st) > k_min_barrage) {
        if (m_flag_i == U16_KEY_NULL) {
            return has_armies();
        }
        return m_flag_i < k_atk_cap && m_atk[m_flag_i].m_hd != U16_KEY_NULL;
    }
    bool flag_ok = (m_flag_i == U16_KEY_NULL);
    const u16 enemy = static_cast<u16>(m_enemy);
    for (u16 i = 0; i < grp_use_n(); ++i) {
        if (m_atk[i].m_hd == U16_KEY_NULL) {
            continue;
        }
        MockSiegeRslt r = {};
        if (mock.siege(m_st, m_atk[i].m_tx, m_atk[i].m_ty, &r) && r.m_taken) {
            if (i == m_flag_i) {
                flag_ok = true;
            }
            continue;
        }
        bool got = false;
        const u16 budget = (m_tgt_n > 0u) ? m_tgt_n : k_tgt_cap;
        for (u16 t = 0; t < budget; ++t) {
            u16 tx = 0u;
            u16 ty = 0u;
            if (!assign_army_city(i, enemy, &tx, &ty)) {
                break;
            }
            r = {};
            if (mock.siege(m_st, tx, ty, &r) && r.m_taken) {
                got = true;
                if (i == m_flag_i) {
                    flag_ok = true;
                }
                break;
            }
        }
        if (got) {
            continue;
        }
        if (i == m_flag_i) {
            continue;
        }
        (void)rejoin_flag(i);
    }
    return flag_ok;
}

static bool war_start_camp (GameState& st, u16 seat, u16 enemy, WarSlot* s) {
    PROF_TH_WAR_ENTER(__func__);
    EVAL_COUNT_ARTILLERY::EVAL(st, seat);
    if (s == nullptr || seat >= st.m_player_n || enemy >= st.m_player_n || seat == enemy) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 0u);
        return false;
    }
    u16 fi = U16_KEY_NULL;
    if (!front_ensure(st, seat, enemy, &fi)) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 1u);
        return false;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    if (!front_staging_xy(st, seat, fi, &sx, &sy)) {
        front_clear(seat, fi);
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 1u);
        return false;
    }
    WarTurnHandler* camp = new WarTurnHandler(st, seat);
    if (camp == nullptr || !camp->ok()) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 2u);
        delete camp;
        return false;
    }
    const u16 stood = stand_load(*camp, seat);
    WarPhase phase = WarPhase::Muster;
    if (stood > 0u) {
        phase = WarPhase::Reassemble;
    } else {
        if (!camp->make_muster_gradient(sx, sy)) {
            LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 3u);
            delete camp;
            return false;
        }
        (void)camp->do_total_muster();
        if (!camp->determine_exposure(enemy)) {
            LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 4u);
            delete camp;
            return false;
        }
    }
    MockMusterSiege* mock = new MockMusterSiege();
    if (mock == nullptr || !mock->collect(st, seat)) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 5u);
        delete mock;
        delete camp;
        return false;
    }
    delete s->m_camp;
    delete s->m_mock;
    s->m_camp = camp;
    s->m_mock = mock;
    s->m_enemy = enemy;
    s->m_front_i = fi;
    s->m_phase = phase;
    u32 unit_n = 0u;
    u32 size_sum = 0u;
    camp->muster_units(&unit_n, &size_sum);
    LOG_WAR_MUSTER::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(sx), SC_U32(sy), SC_U32(unit_n), SC_U32(size_sum), SC_U32(camp->muster_n()), SC_U32(st.m_current_turn));
    return true;
}

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

static void bind_generals (GameState& st, u16 seat, WarSlot* s) {
    if (st.m_player_states == nullptr || seat >= st.m_player_n) {
        return;
    }
    PlayerState& ps = st.m_player_states[seat];
    for (u16 i = 0; i < static_cast<u16>(GENERAL_SLOTS); ++i) {
        ps.m_generals[i].unasgn();
    }
    if (s == nullptr || s->m_camp == nullptr) {
        return;
    }
    WarTurnHandler& camp = *s->m_camp;
    const u16 lim = gen_lim(st, seat);
    u16 slot = 0u;
    for (u16 i = 0; i < WarTurnHandler::k_grp_use && slot < lim; ++i) {
        const u16 hd = camp.atk_hd(i);
        if (hd == U16_KEY_NULL) {
            continue;
        }
        ps.m_generals[slot].asgn(hd);
        ++slot;
    }
}

static void war_stop (GameState& st, u16 seat, WarSlot* s, bool no_tgts) {
    PROF_TH_WAR_ENTER(__func__);
    war_finish(st, seat, s, no_tgts);
}

static void war_advance (GameState& st, u16 seat, WarSlot* s) {
    PROF_TH_WAR_ENTER(__func__);
    if (s == nullptr || s->m_camp == nullptr || s->m_enemy == U16_KEY_NULL) {
        return;
    }
    WarTurnHandler& camp = *s->m_camp;
    switch (s->m_phase) {
    case WarPhase::Reassemble: {
        if (camp.tick_reassemble(g_fronts[seat], WarTurnHandler::k_front_use)) {
            break;
        }
        if (!camp.has_armies()) {
            war_stop(st, seat, s, false);
            break;
        }
        u16 tx = 0u;
        u16 ty = 0u;
        if (!camp.set_target_city(s->m_enemy, &tx, &ty)) {
            war_stop(st, seat, s, true);
            break;
        }
        if (camp.enemy() != U8_KEY_NULL) {
            s->m_enemy = camp.enemy();
        }
        if (!war_try_mocks(st, seat, s, camp)) {
            break;
        }
        s->m_phase = WarPhase::Army;
        break;
    }
    case WarPhase::Muster:
        if (!camp.walk_muster()) {
            prep_eval_muster_grps(camp);
            (void)camp.flatten_muster();
            s->m_phase = WarPhase::Form;
        }
        break;
    case WarPhase::Form:
        if (!camp.form_army()) {
            LOG_WAR_FORM_ARMY_FAIL::LOG(SC_U32(seat), SC_U32(s->m_enemy), SC_U32(st.m_current_turn));
            war_stop(st, seat, s, false);
            break;
        }
        for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
            WarArmy* slot = camp.army(i);
            if (slot == nullptr || slot->m_hd == U16_KEY_NULL) {
                continue;
            }
            slot->m_front = s->m_front_i;
            (void)ArmyUpgradeMng::upgrade_all(st, seat, slot->m_hd, camp.staging_x(), camp.staging_y());
        }
        {
            u16 tx = 0u;
            u16 ty = 0u;
            if (!camp.set_target_city(s->m_enemy, &tx, &ty)) {
                war_stop(st, seat, s, true);
                break;
            }
            if (camp.enemy() != U8_KEY_NULL) {
                s->m_enemy = camp.enemy();
            }
            if (!war_try_mocks(st, seat, s, camp)) {
                break;
            }
        }
        s->m_phase = WarPhase::Army;
        break;
    case WarPhase::BuildUp: {
        if (camp.walk_muster()) {
            break;
        }
        prep_eval_muster_grps(camp);
        (void)camp.flatten_muster();
        (void)camp.absorb_muster_to_flag();
        for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
            WarArmy* slot = camp.army(i);
            if (slot == nullptr || slot->m_hd == U16_KEY_NULL) {
                continue;
            }
            (void)ArmyUpgradeMng::upgrade_all(st, seat, slot->m_hd, camp.staging_x(), camp.staging_y());
        }
        if (s->m_mock == nullptr || !s->m_mock->collect(st, seat)) {
            war_stop(st, seat, s, false);
            break;
        }
        if (!war_try_mocks(st, seat, s, camp)) {
            break;
        }
        s->m_phase = WarPhase::Army;
        break;
    }
    case WarPhase::Army: {
        (void)camp.tick_reinforce(g_fronts[seat], WarTurnHandler::k_front_use);
        bool busy = false;
        bool need = false;
        u16 live = 0u;
        for (u16 i = 0; i < WarTurnHandler::k_grp_use; ++i) {
            WarArmy* slot = camp.army(i);
            if (slot == nullptr || slot->m_hd == U16_KEY_NULL) {
                continue;
            }
            ++live;
            if (slot->m_phase == ArmyPhase::Idle) {
                need = true;
                continue;
            }
            const ArmyTurnRslt ar = camp.army_handle(i);
            if (ar == ArmyTurnRslt::Busy) {
                busy = true;
                continue;
            }
            if (ar == ArmyTurnRslt::NeedTarget) {
                need = true;
                continue;
            }
            UnitMovementMng::set_grp_campaign(st, UnitAddKey::from_raw(slot->m_hd), 0u);
            UnitMovementMng::set_grp_army(st, UnitAddKey::from_raw(slot->m_hd), 0u);
            camp.drop_army(i);
            --live;
        }
        if (busy) {
            break;
        }
        if (need) {
            s->m_phase = WarPhase::Retarget;
            break;
        }
        if (live == 0u) {
            war_stop(st, seat, s, false);
        }
        break;
    }
    case WarPhase::Retarget: {
        u16 tx = 0u;
        u16 ty = 0u;
        if (!camp.retarget_idle(s->m_enemy, &tx, &ty)) {
            LOG_WAR_RETARGET_FAIL::LOG(SC_U32(seat), SC_U32(s->m_enemy), SC_U32(st.m_current_turn));
            war_stop(st, seat, s, true);
            break;
        }
        if (camp.enemy() != U8_KEY_NULL) {
            s->m_enemy = camp.enemy();
        }
        if (!war_try_mocks(st, seat, s, camp)) {
            break;
        }
        s->m_phase = WarPhase::Army;
        break;
    }
    case WarPhase::Idle:
    default:
        break;
    }
}

bool WarTurnHandler::begin (GameState& state) {
    PROF_TH_WAR_ENTER(__func__);
    clear();
    if (state.m_player_n == 0u || state.m_player_n > k_seat_cap) {
        LOG_WAR_BEGIN_FAIL::LOG(SC_U32(state.m_player_n), SC_U32(state.m_current_turn));
        return false;
    }
    g_war_st = &state;
    g_war_n = state.m_player_n;
    fronts_idle_all(g_war_n);
    stand_idle_all(g_war_n);
    for (u16 i = 0u; i < g_war_n; ++i) {
        g_war[i].m_camp = nullptr;
        g_war[i].m_mock = nullptr;
        g_war[i].m_enemy = U16_KEY_NULL;
        g_war[i].m_front_i = U16_KEY_NULL;
        g_war[i].m_phase = WarPhase::Idle;
    }
    (void)war_sec_begin(state);
    return true;
}

void WarTurnHandler::clear () {
    for (u16 i = 0u; i < g_war_n && i < k_seat_cap; ++i) {
        war_slot_reset(&g_war[i]);
    }
    fronts_idle_all(g_war_n);
    stand_idle_all(g_war_n);
    g_war_st = nullptr;
    g_war_n = 0u;
    war_sec_clr();
}

bool WarTurnHandler::pick_enemy (GameState& state, u16 seat, u16* out_enemy) {
    PROF_TH_WAR_ENTER(__func__);
    if (out_enemy == nullptr || g_war_st != &state || seat >= g_war_n || state.m_player_states == nullptr) {
        LOG_WAR_PICK_ENEMY_FAIL::LOG(SC_U32(seat), SC_U32(state.m_current_turn), 0u);
        return false;
    }
    u16 fx = 0u;
    u16 fy = 0u;
    bool have = false;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        fx = c->get_x();
        fy = c->get_y();
        have = true;
        break;
    }
    if (!have) {
        LOG_WAR_PICK_ENEMY_FAIL::LOG(SC_U32(seat), SC_U32(state.m_current_turn), 1u);
        return false;
    }
    WarTargetPlan plan;
    plan.m_enemy = U16_KEY_NULL;
    plan.m_n = 0u;
    if (!war_target_pick(state, seat, fx, fy, &plan)) {
        LOG_WAR_PICK_ENEMY_FAIL::LOG(SC_U32(seat), SC_U32(state.m_current_turn), 1u);
        return false;
    }
    if (plan.m_enemy >= state.m_player_n || plan.m_enemy == seat) {
        LOG_WAR_PICK_ENEMY_FAIL::LOG(SC_U32(seat), SC_U32(state.m_current_turn), 2u);
        return false;
    }
    if (state.m_player_states[plan.m_enemy].m_lucky != 0u
        || state.m_player_states[plan.m_enemy].m_is_active == 0u) {
        LOG_WAR_PICK_ENEMY_FAIL::LOG(SC_U32(seat), SC_U32(state.m_current_turn), 3u);
        return false;
    }
    *out_enemy = plan.m_enemy;
    return true;
}

bool WarTurnHandler::engage (GameState& state, u16 seat, u16 enemy) {
    PROF_TH_WAR_ENTER(__func__);
    if (g_war_st != &state || seat >= g_war_n || enemy >= g_war_n || seat == enemy) {
        LOG_WAR_ENGAGE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(state.m_current_turn), 0u);
        return false;
    }
    if (state.m_player_states == nullptr) {
        LOG_WAR_ENGAGE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(state.m_current_turn), 1u);
        return false;
    }
    war_slot_reset(&g_war[seat]);
    if (!war_start_camp(state, seat, enemy, &g_war[seat])) {
        LOG_WAR_ENGAGE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(state.m_current_turn), 2u);
        return false;
    }
    return true;
}

bool WarTurnHandler::is_engaged (u16 seat) {
    return seat < g_war_n && g_war[seat].m_enemy != U16_KEY_NULL;
}

const SectorPathGradients* WarTurnHandler::path_grads (u16 pool_i) {
    return SectorPathGradients::pool_bank(pool_i);
}

void WarTurnHandler::handle (GameState& state) {
    PROF_TH_WAR_ENTER(__func__);
    if (g_war_st != &state || state.m_player_states == nullptr) {
        return;
    }
    for (u16 seat = 0u; seat < g_war_n; ++seat) {
        PlayerState& ps = state.m_player_states[seat];
        if (ps.m_is_active == 0u || ps.m_ai_controlled == 0u) {
            continue;
        }
        WarSlot& s = g_war[seat];
        if (s.m_enemy == U16_KEY_NULL) {
            continue;
        }
        if (s.m_camp == nullptr) {
            if (!war_start_camp(state, seat, s.m_enemy, &s)) {
                LOG_WAR_HANDLE_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(s.m_enemy), SC_U32(state.m_current_turn));
                war_stop(state, seat, &s, false);
                continue;
            }
        }
        bind_generals(state, seat, &s);
        war_advance(state, seat, &s);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
