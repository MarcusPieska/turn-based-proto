//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "army_turn_handler.h"

#include "city.h"
#include "city_attack_manager.h"
#include "game_state.h"
#include "general_move_boost.h"
#include "log_dbg.h"
#include "mvt_pts_boost_mng.h"
#include "runtime_statics.h"
#include "tile_transfer.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_turn_handler.h"
#include "unit_type_action_map.h"
#include "path_gradients.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void ArmyTurnHandler::refill_grp (GameState& s, u16 head_idx) {
    if (s.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(head_idx);
    while (cur.is_valid()) {
        UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_unit_typ_idx < typ_n) {
            MvtPtsBoostMng::fill(s, u);
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    GeneralMoveBoost::on_grp_refilled(s, UnitAddKey::from_raw(head_idx));
}

void ArmyTurnHandler::claim_city (GameState& s, u16 seat, u16 x, u16 y) {
    PROF_TH_WAR_ENTER(__func__);
    const u16 cn = s.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_x() != x || c->get_y() != y) {
            continue;
        }
        const u8 from = static_cast<u8>(c->get_owner());
        c->set_owner(seat);
        s.m_map.set_civ_owner(x, y, static_cast<u8>(seat));
        TileTransfer::apply(s, i, from, static_cast<u8>(seat), nullptr);
        LOG_WAR_CITY_CAPTURE::LOG(SC_U32(seat), SC_U32(from), SC_U32(x), SC_U32(y), SC_U32(s.m_current_turn));
        return;
    }
    LOG_WAR_CLAIM_CITY_MISS::LOG(SC_U32(seat), SC_U32(x), SC_U32(y), SC_U32(s.m_current_turn));
}

//================================================================================================================================
//=> - ArmyTurnHandler -
//================================================================================================================================

bool ArmyTurnHandler::walk (GameState& s, WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    if (army.m_hd == U16_KEY_NULL || army.m_paths == nullptr || !army.m_paths->ok(army.m_path_i)) {
        return false;
    }
    refill_grp(s, army.m_hd);
    const UnitAddKey key = UnitAddKey::from_raw(army.m_hd);
    UnitAddStruct* u = s.m_units.get_unit_add(key);
    if (u == nullptr || u->m_x == U16_KEY_NULL) {
        return false;
    }
    if (u->m_x == army.m_tx && u->m_y == army.m_ty) {
        return false;
    }
    bool any = false;
    for (;;) {
        u = s.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            break;
        }
        if (u->m_x == army.m_tx && u->m_y == army.m_ty) {
            break;
        }
        const PathGradients::StepRes step = army.m_paths->peek(army.m_path_i, s, u->m_x, u->m_y);
        if (!step.have) {
            break;
        }
        if (!UnitMovementMng::can_step(s, key, step.nx, step.ny, nullptr)) {
            break;
        }
        if (!UnitMovementMng::apply_step(s, key, step.nx, step.ny)) {
            break;
        }
        any = true;
    }
    return any;
}

WarAssault ArmyTurnHandler::assault (GameState& s, u16 seat, WarArmy& army, u16 city_x, u16 city_y) {
    PROF_TH_WAR_ENTER(__func__);
    army.m_stall = false;
    army.m_br_tot = 0u;
    army.m_br_last = 0u;
    army.m_assault = WarAssault::Fail;
    if (army.m_hd == U16_KEY_NULL) {
        LOG_WAR_ASSAULT_CITY_FAIL::LOG(SC_U32(seat), SC_U32(city_x), SC_U32(city_y), SC_U32(army.m_idx), SC_U32(s.m_current_turn));
        return army.m_assault;
    }
    UnitAddKey head = UnitAddKey::from_raw(army.m_hd);
    const CityAttackResult ar = CityAttackManager::assault(s, &head, city_x, city_y);
    army.m_br_tot = ar.m_br_tot;
    army.m_br_last = ar.m_br_last;
    const UnitAddKey stay = ar.m_stay;
    const UnitAddKey occupy = ar.m_occupy;
    if (ar.m_r == CityAssault::Stall) {
        if (stay.is_valid()) {
            army.m_hd = stay.value();
        } else if (head.is_valid()) {
            army.m_hd = head.value();
        }
        army.m_stall = true;
        army.m_assault = WarAssault::Stall;
        return army.m_assault;
    }
    if (ar.m_r != CityAssault::Ok) {
        if (stay.is_valid()) {
            army.m_hd = stay.value();
        }
        if (occupy.is_valid()) {
            army.m_hd = occupy.value();
            army.m_split = stay.is_valid() ? stay.value() : U16_KEY_NULL;
        } else if (head.is_valid()) {
            army.m_hd = head.value();
        }
        LOG_WAR_ASSAULT_CITY_FAIL::LOG(SC_U32(seat), SC_U32(city_x), SC_U32(city_y), SC_U32(army.m_idx), SC_U32(s.m_current_turn));
        army.m_assault = WarAssault::Fail;
        return army.m_assault;
    }
    if (occupy.is_valid()) {
        army.m_hd = occupy.value();
        army.m_split = stay.is_valid() ? stay.value() : U16_KEY_NULL;
    } else {
        army.m_hd = stay.is_valid() ? stay.value() : U16_KEY_NULL;
        army.m_split = U16_KEY_NULL;
    }
    claim_city(s, seat, city_x, city_y);
    army.m_assault = WarAssault::Ok;
    return army.m_assault;
}

bool ArmyTurnHandler::rejoin_move (GameState& s, u16 seat, u16 enemy, WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    const u16 null_xy = U16_KEY_NULL;
    if (army.m_split == U16_KEY_NULL) {
        return true;
    }
    if (army.m_hd == U16_KEY_NULL) {
        LOG_WAR_REJOIN_MOVE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx),
            SC_U32(s.m_current_turn), 1u, SC_U32(null_xy), SC_U32(null_xy), SC_U32(null_xy), SC_U32(null_xy));
        return false;
    }
    const UnitAddStruct* occ = s.m_units.get_unit_add(UnitAddKey::from_raw(army.m_hd));
    if (occ == nullptr || occ->m_x == U16_KEY_NULL) {
        LOG_WAR_REJOIN_MOVE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx),
            SC_U32(s.m_current_turn), 2u, SC_U32(null_xy), SC_U32(null_xy), SC_U32(null_xy), SC_U32(null_xy));
        return false;
    }
    const u16 dx = occ->m_x;
    const u16 dy = occ->m_y;
    refill_grp(s, army.m_split);
    const UnitAddKey key = UnitAddKey::from_raw(army.m_split);
    UnitAddStruct* u = s.m_units.get_unit_add(key);
    if (u == nullptr || u->m_x == U16_KEY_NULL) {
        LOG_WAR_REJOIN_MOVE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx),
            SC_U32(s.m_current_turn), 3u, SC_U32(null_xy), SC_U32(null_xy), SC_U32(dx), SC_U32(dy));
        return false;
    }
    const u16 sx = u->m_x;
    const u16 sy = u->m_y;
    if (sx == dx && sy == dy) {
        return true;
    }
    const CanStepFail step_r = UnitMovementMng::can_step_reason(s, key, dx, dy, nullptr);
    if (step_r != CanStepFail::Ok) {
        LOG_WAR_REJOIN_MOVE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx),
            SC_U32(s.m_current_turn), static_cast<unsigned>(40u + static_cast<u8>(step_r)),
            SC_U32(sx), SC_U32(sy), SC_U32(dx), SC_U32(dy));
        return false;
    }
    if (!UnitMovementMng::apply_step(s, key, dx, dy)) {
        LOG_WAR_REJOIN_MOVE_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx),
            SC_U32(s.m_current_turn), 5u, SC_U32(sx), SC_U32(sy), SC_U32(dx), SC_U32(dy));
        return false;
    }
    return true;
}

bool ArmyTurnHandler::rejoin_link (GameState& s, WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    if (army.m_split == U16_KEY_NULL) {
        return true;
    }
    if (army.m_hd == U16_KEY_NULL) {
        return false;
    }
    const UnitAddKey occ = UnitAddKey::from_raw(army.m_hd);
    const UnitAddKey sty = UnitAddKey::from_raw(army.m_split);
    const UnitAddStruct* ou = s.m_units.get_unit_add(occ);
    UnitAddStruct* su = s.m_units.get_unit_add(sty);
    if (ou == nullptr || su == nullptr || ou->m_x == U16_KEY_NULL || su->m_x == U16_KEY_NULL) {
        return false;
    }
    if (ou->m_x != su->m_x || ou->m_y != su->m_y) {
        return false;
    }
    const u16 x = ou->m_x;
    const u16 y = ou->m_y;
    static const u16 k_cap = 2048u;
    UnitAddKey keys[k_cap];
    u16 n = 0u;
    UnitAddKey cur = occ;
    while (cur.is_valid() && n < k_cap) {
        keys[n++] = cur;
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    cur = sty;
    while (cur.is_valid() && n < k_cap) {
        keys[n++] = cur;
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr || u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    if (n == 0u) {
        return false;
    }
    UnitAddStruct* hu = s.m_units.get_unit_add(occ);
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
    su = s.m_units.get_unit_add(sty);
    if (su == nullptr) {
        return false;
    }
    nxt = UnitAddKey::from_raw(su->m_next_unit_in_group);
    su->m_next_unit_in_group = U16_KEY_NULL;
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
    UnitAddKey merged = UnitAddKey::None();
    if (!UnitMovementMng::form_group_chain(s, keys, n, &merged)) {
        return false;
    }
    army.m_hd = merged.value();
    army.m_split = U16_KEY_NULL;
    return true;
}

u16 ArmyTurnHandler::rest_heal (GameState& s, WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    if (army.m_hd == U16_KEY_NULL || s.m_statics == nullptr) {
        return 0u;
    }
    u8 worst = UNIT_HEALTH;
    UnitAddKey cur = UnitAddKey::from_raw(army.m_hd);
    while (cur.is_valid()) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u && u->m_health < worst) {
            worst = u->m_health;
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    if (worst >= UNIT_HEALTH) {
        return 0u;
    }
    const u16 pct = s.m_statics->config().get_unit_heal_in_city();
    const u16 add = static_cast<u16>((static_cast<u32>(UNIT_HEALTH) * static_cast<u32>(pct)) / 100u);
    if (add == 0u) {
        return 0u;
    }
    const u16 need = static_cast<u16>(UNIT_HEALTH - worst);
    const u16 turns = static_cast<u16>((need + add - 1u) / add);
    for (u16 t = 0; t < turns; ++t) {
        refill_grp(s, army.m_hd);
        UnitTurnHandler::handle(s, army.m_hd);
    }
    return turns;
}

bool ArmyTurnHandler::can_fight (const GameState& s, const WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    if (s.m_statics == nullptr || army.m_hd == U16_KEY_NULL) {
        return false;
    }
    const UnitTypeActionMap& am = s.m_statics->unit_type_action_map();
    const u16 typ_n = s.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(army.m_hd);
    while (cur.is_valid()) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u && u->m_unit_typ_idx < typ_n) {
            const u16 ut = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
            if (am.unit_type_can_do(ut, static_cast<u16>(UnitAction::canAttack))
                || am.unit_type_can_do(ut, static_cast<u16>(UnitAction::canBarrage))) {
                return true;
            }
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return false;
}

bool ArmyTurnHandler::can_cont (const GameState& s, const WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    if (s.m_statics == nullptr || army.m_hd == U16_KEY_NULL) {
        return false;
    }
    const UnitTypeActionMap& am = s.m_statics->unit_type_action_map();
    const u16 typ_n = s.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(army.m_hd);
    while (cur.is_valid()) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u && u->m_unit_typ_idx < typ_n) {
            const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
            const u16 ut = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
            if (pts > 0u && am.unit_type_can_do(ut, static_cast<u16>(UnitAction::canAttack))) {
                return true;
            }
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return false;
}

ArmyTurnRslt ArmyTurnHandler::handle (GameState& s, u16 seat, u16 enemy, WarArmy& army) {
    PROF_TH_WAR_ENTER(__func__);
    if (army.m_hd == U16_KEY_NULL) {
        army.m_phase = ArmyPhase::Idle;
        return ArmyTurnRslt::Stop;
    }
    if (army.m_phase == ArmyPhase::Idle) {
        army.m_phase = ArmyPhase::March;
    }
    switch (army.m_phase) {
    case ArmyPhase::March:
        if (walk(s, army)) {
            return ArmyTurnRslt::Busy;
        }
        army.m_phase = ArmyPhase::Assault;
        return ArmyTurnRslt::Busy;
    case ArmyPhase::Assault: {
        const WarAssault ar = assault(s, seat, army, army.m_tx, army.m_ty);
        if (ar == WarAssault::Stall) {
            if (!can_cont(s, army)) {
                LOG_WAR_ASSAULT_STALL_STOP::LOG(
                    SC_U32(seat), SC_U32(enemy), SC_U32(army.m_tx), SC_U32(army.m_ty), SC_U32(s.m_current_turn));
                army.m_phase = ArmyPhase::Idle;
                return ArmyTurnRslt::Stop;
            }
            return ArmyTurnRslt::Busy;
        }
        if (ar != WarAssault::Ok) {
            LOG_WAR_ASSAULT_FAIL_STOP::LOG(
                SC_U32(seat), SC_U32(enemy), SC_U32(army.m_tx), SC_U32(army.m_ty), SC_U32(s.m_current_turn));
            army.m_phase = ArmyPhase::Idle;
            return ArmyTurnRslt::Stop;
        }
        army.m_phase = ArmyPhase::Rejoin;
        return ArmyTurnRslt::Busy;
    }
    case ArmyPhase::Rejoin: {
        if (army.m_split != U16_KEY_NULL) {
            const UnitAddStruct* occ = s.m_units.get_unit_add(UnitAddKey::from_raw(army.m_hd));
            const UnitAddStruct* spl = s.m_units.get_unit_add(UnitAddKey::from_raw(army.m_split));
            if (occ != nullptr && spl != nullptr && occ->m_x != U16_KEY_NULL && spl->m_x != U16_KEY_NULL
                && (occ->m_x != spl->m_x || occ->m_y != spl->m_y)) {
                if (!rejoin_move(s, seat, enemy, army)) {
                    army.m_phase = ArmyPhase::Idle;
                    return ArmyTurnRslt::Stop;
                }
                return ArmyTurnRslt::Busy;
            }
        }
        if (!rejoin_link(s, army)) {
            LOG_WAR_REJOIN_LINK_FAIL::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx), SC_U32(s.m_current_turn));
            army.m_phase = ArmyPhase::Idle;
            return ArmyTurnRslt::Stop;
        }
        (void)rest_heal(s, army);
        if (!can_fight(s, army)) {
            LOG_WAR_ARMY_CANT_FIGHT::LOG(SC_U32(seat), SC_U32(enemy), SC_U32(army.m_idx), SC_U32(s.m_current_turn));
            army.m_phase = ArmyPhase::Idle;
            return ArmyTurnRslt::Stop;
        }
        army.m_phase = ArmyPhase::Idle;
        return ArmyTurnRslt::NeedTarget;
    }
    case ArmyPhase::Idle:
    default:
        return ArmyTurnRslt::Stop;
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
