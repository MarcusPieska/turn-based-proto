//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_turn_handler.h"

#include "army_upgrade_mng.h"
#include "city_attack_manager.h"
#include "city.h"
#include "civ_relations.h"
#include "civ_static_key.h"
#include "game_array_simple.h"
#include "generate_access_mask.h"
#include "generate_distance_p2p.h"
#include "generate_exposure.h"
#include "gen_land_sector_network.h"
#include "gen_land_sectors.h"
#include "game_state.h"
#include "game_map_defs.h"
#include "land_sector_network.h"
#include "log_dbg.h"
#include "mock_muster_siege.h"
#include "negotiate_peace.h"
#include "runtime_statics.h"
#include "sector_support.h"
#include "target_ordering_flood.h"
#include "target_sector_defensible.h"
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
    m_paths(),
    m_mob(),
    m_grp_n(0),
    m_atk_n(0),
    m_tgt_n(0),
    m_tgt_i(0),
    m_sx(0),
    m_sy(0),
    m_tx(0),
    m_ty(0),
    m_enemy(U8_KEY_NULL),
    m_ready(false),
    m_mob_ok(false) {
    for (u16 i = 0; i < k_atk_cap; ++i) {
        m_atk[i].m_hd = U16_KEY_NULL;
        m_atk[i].m_split = U16_KEY_NULL;
        m_atk[i].m_tx = 0u;
        m_atk[i].m_ty = 0u;
        m_atk[i].m_idx = i;
        m_atk[i].m_path_i = i;
        m_atk[i].m_paths = &m_paths;
        m_atk[i].m_phase = ArmyPhase::Idle;
        m_atk[i].m_stall = false;
        m_atk[i].m_br_tot = 0u;
        m_atk[i].m_br_last = 0u;
        m_atk[i].m_assault = WarAssault::Fail;
    }
}

WarTurnHandler::~WarTurnHandler () {
}

bool WarTurnHandler::ok () const {
    return m_st.m_statics != nullptr
        && m_st.m_player_states != nullptr
        && m_seat < m_st.m_player_n
        && m_mob.ok();
}

bool WarTurnHandler::set_goal (u16 x1, u16 y1, u16 x2, u16 y2) {
    PROF_TH_WAR_ENTER(__func__);
    m_ready = false;
    if (!ok()) {
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    Whiteboard_1B acc("WarTurnHandler", "acc", 0u);
    if (!GenerateAccessMask::generate(m_st, m_seat, acc)) {
        LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
        return false;
    }
    const u16 n = (m_atk_n > 0u) ? m_atk_n : 1u;
    for (u16 i = 0; i < n && i < k_atk_cap; ++i) {
        u16 ax = x1;
        u16 ay = y1;
        if (m_atk[i].m_hd != U16_KEY_NULL) {
            const UnitAddStruct* u = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[i].m_hd));
            if (u != nullptr && u->m_x != U16_KEY_NULL) {
                ax = u->m_x;
                ay = u->m_y;
            }
        }
        u16 dmax = 0;
        if (!GenerateDistanceP2P::generate(m_st, *m_st.m_statics, ax, ay, x2, y2, m_paths.at(i), &acc, &dmax)) {
            LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
            return false;
        }
        if (!m_paths.ok(i)) {
            LOG_WAR_SET_GOAL_FAIL::LOG(SC_U32(m_seat), SC_U32(x2), SC_U32(y2), SC_U32(m_st.m_current_turn));
            return false;
        }
        m_atk[i].m_path_i = i;
        m_atk[i].m_paths = &m_paths;
        m_atk[i].m_tx = x2;
        m_atk[i].m_ty = y2;
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
        return false;
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
            const u16 pts = m_st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
            const u16 turn_mp = m_st.m_statics->config().get_mov_pt_per_turn();
            u->m_mvt_points = static_cast<i16>(pts * turn_mp);
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
}

bool WarTurnHandler::walk_muster () {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || !m_mob_ok || m_grp_n == 0u) {
        LOG_WAR_WALK_MUSTER_FAIL::LOG(SC_U32(m_seat), SC_U32(m_st.m_current_turn));
        return false;
    }
    bool any = false;
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
                any = true;
                continue;
            }
            LOG_WAR_MUSTER_CAN_STEP_FAIL::LOG(
                SC_U32(m_seat), SC_U32(key.value()), SC_U32(static_cast<u8>(last)),
                SC_U32(ux), SC_U32(uy), SC_U32(lnx), SC_U32(lny), SC_U32(m_st.m_current_turn));
            break;
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
    UnitAddKey parts[k_atk_cap];
    u16 part_n = 0u;
    if (!UnitGroupManagement::partition_land_armies_by_arty(m_st, head, parts, k_atk_cap, &part_n)
        || part_n == 0u) {
        return false;
    }
    m_grp_n = 0;
    m_atk_n = 0u;
    u32 army_n = 0u;
    u32 size_sum = 0u;
    for (u16 i = 0; i < k_atk_cap; ++i) {
        m_atk[i].m_hd = U16_KEY_NULL;
        m_atk[i].m_split = U16_KEY_NULL;
        m_atk[i].m_tx = m_tx;
        m_atk[i].m_ty = m_ty;
        m_atk[i].m_idx = i;
        m_atk[i].m_path_i = i;
        m_atk[i].m_paths = &m_paths;
        m_atk[i].m_phase = ArmyPhase::Idle;
        m_atk[i].m_stall = false;
        m_atk[i].m_br_tot = 0u;
        m_atk[i].m_br_last = 0u;
        m_atk[i].m_assault = WarAssault::Fail;
    }
    for (u16 i = 0; i < part_n && i < k_atk_cap; ++i) {
        if (!parts[i].is_valid()) {
            continue;
        }
        UnitMovementMng::set_grp_campaign(m_st, parts[i], 1u);
        m_atk[i].m_hd = parts[i].value();
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

bool WarTurnHandler::refill_targets (u16 enemy) {
    PROF_TH_WAR_ENTER(__func__);
    m_tgt_n = 0;
    m_tgt_i = 0;
    m_enemy = U8_KEY_NULL;
    WarTargetPlan plan;
    plan.m_enemy = enemy;
    plan.m_n = 0u;
    if (!war_target_pick(m_st, m_seat, m_sx, m_sy, &plan) || plan.m_n == 0u || plan.m_enemy != enemy) {
        LOG_WAR_REFILL_TARGETS_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 1u);
        return false;
    }
    const u16 n = (plan.m_n < k_tgt_cap) ? plan.m_n : k_tgt_cap;
    for (u16 i = 0; i < n; ++i) {
        m_tgts[i] = plan.m_cities[i];
    }
    m_tgt_n = n;
    m_enemy = static_cast<u8>(enemy);
    return true;
}

bool WarTurnHandler::set_target_city (u16 enemy, u16* ox, u16* oy) {
    PROF_TH_WAR_ENTER(__func__);
    m_ready = false;
    if (!ok() || enemy >= m_st.m_player_n || enemy == m_seat) {
        LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 0u);
        return false;
    }
    for (u8 pass = 0; pass < 2u; ++pass) {
        if (pass > 0u || m_enemy != static_cast<u8>(enemy) || m_tgt_i >= m_tgt_n) {
            if (!refill_targets(enemy)) {
                LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 1u);
                return false;
            }
        }
        while (m_tgt_i < m_tgt_n) {
            const u16 cidx = m_tgts[m_tgt_i++];
            City* c = m_st.m_cities.get_city(cidx);
            if (c == nullptr || c->get_owner() != enemy) {
                continue;
            }
            const u16 tx = c->get_x();
            const u16 ty = c->get_y();
            const u16 self_civ = m_st.m_player_states[m_seat].m_civ_index;
            const u16 enemy_civ = m_st.m_player_states[enemy].m_civ_index;
            m_st.m_civ_relations.set(
                CivStaticDataKey::from_raw(self_civ),
                CivStaticDataKey::from_raw(enemy_civ),
                CivRel::CIV_REL_WAR);
            if (!set_goal(m_sx, m_sy, tx, ty)) {
                continue;
            }
            m_tx = tx;
            m_ty = ty;
            for (u16 i = 0; i < m_atk_n && i < k_atk_cap; ++i) {
                m_atk[i].m_tx = tx;
                m_atk[i].m_ty = ty;
                m_atk[i].m_path_i = i;
                m_atk[i].m_paths = &m_paths;
                m_atk[i].m_phase = ArmyPhase::March;
                m_atk[i].m_assault = WarAssault::Fail;
                m_atk[i].m_stall = false;
            }
            if (ox != nullptr) {
                *ox = tx;
            }
            if (oy != nullptr) {
                *oy = ty;
            }
            return true;
        }
    }
    LOG_WAR_SET_TARGET_FAIL::LOG(SC_U32(m_seat), SC_U32(enemy), SC_U32(m_st.m_current_turn), 2u);
    return false;
}

ArmyTurnRslt WarTurnHandler::army_handle (u16 army_i) {
    PROF_TH_WAR_ENTER(__func__);
    if (!ok() || army_i >= k_atk_cap || m_atk[army_i].m_hd == U16_KEY_NULL) {
        return ArmyTurnRslt::Stop;
    }
    const ArmyTurnRslt r = ArmyTurnHandler::handle(m_st, m_seat, m_enemy, m_atk[army_i]);
    if (m_atk[army_i].m_phase == ArmyPhase::Rejoin || r == ArmyTurnRslt::NeedTarget) {
        sync_from_army(army_i);
    }
    return r;
}

void WarTurnHandler::sync_from_army (u16 army_i) {
    if (army_i >= k_atk_cap) {
        return;
    }
    m_sx = m_atk[army_i].m_tx;
    m_sy = m_atk[army_i].m_ty;
    m_tx = m_atk[army_i].m_tx;
    m_ty = m_atk[army_i].m_ty;
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
    }
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
    Retarget = 4
};

struct WarSlot {
    WarTurnHandler* m_camp;
    MockMusterSiege* m_mock;
    u16 m_enemy;
    WarPhase m_phase;
};

static WarSlot g_war[WarTurnHandler::k_seat_cap];
static GameState* g_war_st = nullptr;
static u16 g_war_n = 0u;
static GenLandSectors* g_gls = nullptr;
static LandSectorSeeds g_seeds = {};
static LandSectorNetwork* g_net = nullptr;
static const u32 k_sec_rng = 43u;

static void war_sec_clr () {
    SectorSupport::clr();
    GenLandSectors::free_seeds(&g_seeds);
    delete g_gls;
    g_gls = nullptr;
    delete g_net;
    g_net = nullptr;
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
    if (!SectorSupport::bind(&st, &g_gls->sectors(), &g_seeds, g_net)) {
        LOG_WAR_SEC_BEGIN_FAIL::LOG(SC_U32(st.m_current_turn), 3u);
        war_sec_clr();
        return false;
    }
    return true;
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
    s->m_phase = WarPhase::Idle;
}

static void war_peace_mock (GameState& st, u16 seat, WarSlot* s, u16 tx, u16 ty) {
    LOG_WAR_PEACE_MOCK::LOG(SC_U32(seat), SC_U32(tx), SC_U32(ty), SC_U32(st.m_current_turn));
    const u16 enemy = (s != nullptr) ? s->m_enemy : U16_KEY_NULL;
    (void)NegotiatePeace::deal(st, seat, enemy);
    war_slot_reset(s);
}

static bool war_mock_next (GameState& st, u16 seat, WarSlot* s, u16 tx, u16 ty) {
    PROF_TH_WAR_ENTER(__func__);
    if (s == nullptr || s->m_mock == nullptr || !s->m_mock->ok()) {
        war_peace_mock(st, seat, s, tx, ty);
        return false;
    }
    MockSiegeRslt r = {};
    if (!s->m_mock->siege(st, tx, ty, &r) || !r.m_taken) {
        war_peace_mock(st, seat, s, tx, ty);
        return false;
    }
    return true;
}

static bool war_start_camp (GameState& st, u16 seat, u16 enemy, WarSlot* s) {
    PROF_TH_WAR_ENTER(__func__);
    if (s == nullptr || seat >= st.m_player_n || enemy >= st.m_player_n || seat == enemy) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 0u);
        return false;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    if (!WarTurnHandler::pick_staging_city(st, seat, enemy, &sx, &sy)) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 1u);
        return false;
    }
    WarTurnHandler* camp = new WarTurnHandler(st, seat);
    if (camp == nullptr || !camp->ok()) {
        LOG_WAR_START_CAMP_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(st.m_current_turn), 2u);
        delete camp;
        return false;
    }
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
    s->m_phase = WarPhase::Muster;
    u32 unit_n = 0u;
    u32 size_sum = 0u;
    camp->muster_units(&unit_n, &size_sum);
    LOG_WAR_MUSTER::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(sx), SC_U32(sy), SC_U32(unit_n), SC_U32(size_sum), SC_U32(camp->muster_n()), SC_U32(st.m_current_turn));
    return true;
}

static void war_stop (GameState& st, u16 seat, WarSlot* s) {
    PROF_TH_WAR_ENTER(__func__);
    const u16 enemy = (s != nullptr) ? s->m_enemy : U16_KEY_NULL;
    (void)NegotiatePeace::deal(st, seat, enemy);
    war_slot_reset(s);
}

static void war_advance (GameState& st, u16 seat, WarSlot* s) {
    PROF_TH_WAR_ENTER(__func__);
    if (s == nullptr || s->m_camp == nullptr || s->m_enemy == U16_KEY_NULL) {
        return;
    }
    WarTurnHandler& camp = *s->m_camp;
    switch (s->m_phase) {
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
            war_stop(st, seat, s);
            break;
        }
        (void)ArmyUpgradeMng::upgrade_all(st, seat, camp.atk_hd(0u), camp.staging_x(), camp.staging_y());
        {
            u16 tx = 0u;
            u16 ty = 0u;
            if (!camp.set_target_city(s->m_enemy, &tx, &ty)) {
                war_stop(st, seat, s);
                break;
            }
            if (!war_mock_next(st, seat, s, tx, ty)) {
                break;
            }
        }
        s->m_phase = WarPhase::Army;
        break;
    case WarPhase::Army: {
        const ArmyTurnRslt ar = camp.army_handle(0u);
        if (ar == ArmyTurnRslt::Busy) {
            break;
        }
        if (ar == ArmyTurnRslt::NeedTarget) {
            s->m_phase = WarPhase::Retarget;
            break;
        }
        war_stop(st, seat, s);
        break;
    }
    case WarPhase::Retarget: {
        u16 tx = 0u;
        u16 ty = 0u;
        if (!camp.set_target_city(s->m_enemy, &tx, &ty)) {
            LOG_WAR_RETARGET_FAIL::LOG(SC_U32(seat), SC_U32(s->m_enemy), SC_U32(st.m_current_turn));
            war_stop(st, seat, s);
            break;
        }
        if (!war_mock_next(st, seat, s, tx, ty)) {
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
    for (u16 i = 0u; i < g_war_n; ++i) {
        g_war[i].m_camp = nullptr;
        g_war[i].m_mock = nullptr;
        g_war[i].m_enemy = U16_KEY_NULL;
        g_war[i].m_phase = WarPhase::Idle;
    }
    (void)war_sec_begin(state);
    return true;
}

void WarTurnHandler::clear () {
    for (u16 i = 0u; i < g_war_n && i < k_seat_cap; ++i) {
        war_slot_reset(&g_war[i]);
    }
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
                war_stop(state, seat, &s);
                continue;
            }
        }
        war_advance(state, seat, &s);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
