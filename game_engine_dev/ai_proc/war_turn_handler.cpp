//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_turn_handler.h"

#include "city_attack_manager.h"
#include "city.h"
#include "city_array.h"
#include "civ_relations.h"
#include "civ_static_key.h"
#include "game_array_simple.h"
#include "generate_access_mask.h"
#include "generate_distance_p2p.h"
#include "generate_exposure.h"
#include "game_state.h"
#include "game_map_defs.h"
#include "runtime_statics.h"
#include "target_ordering_flood.h"
#include "tile_transfer.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_turn_handler.h"
#include "unit_type_action_map.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mob_hit (const WalkP2P& mob, u16 x, u16 y) {
    if (!mob.ok() || x >= mob.w() || y >= mob.h()) {
        return false;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(mob.w()) + static_cast<u32>(x);
    return mob.turn()[i] != GenerateDistanceP2P::k_turn_sent;
}

static bool is_wtr_tile (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]
        || t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool land_ok (const GameState& s, u16 x, u16 y) {
    const u8 t = s.m_map.get_terrain(x, y);
    return t != TERR_NONE[0] && t != TERR_MOUNTAINS[0] && !is_wtr_tile(t);
}

static bool own_free_open (const GameState& s, u16 seat, u16 x, u16 y) {
    const u8 own = s.m_map.get_civ_owner(x, y);
    return own == U8_KEY_NULL || own == static_cast<u8>(seat);
}

static bool label_own_free_comp (const GameState& s, u16 seat, u32* tile_comp, u32* out_n) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 n = s.m_map.tile_n();
    if (w == 0 || h == 0 || tile_comp == nullptr || out_n == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        tile_comp[i] = 0xFFFFFFFFu;
    }
    u32* q = new u32[n];
    if (q == nullptr) {
        return false;
    }
    u32 next_id = 0;
    static const i32 k_dx[4] = {-1, 1, 0, 0};
    static const i32 k_dy[4] = {0, 0, -1, 1};
    for (u32 i = 0; i < n; ++i) {
        if (tile_comp[i] != 0xFFFFFFFFu) {
            continue;
        }
        const u16 x0 = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y0 = static_cast<u16>(i / static_cast<u32>(w));
        if (!land_ok(s, x0, y0) || !own_free_open(s, seat, x0, y0)) {
            continue;
        }
        const u32 cid = next_id++;
        u32 qn = 0;
        tile_comp[i] = cid;
        q[qn++] = i;
        while (qn > 0) {
            const u32 cur = q[--qn];
            const u16 x = static_cast<u16>(cur % static_cast<u32>(w));
            const u16 y = static_cast<u16>(cur / static_cast<u32>(w));
            for (u16 k = 0; k < 4u; ++k) {
                const i32 nx = static_cast<i32>(x) + k_dx[k];
                const i32 ny = static_cast<i32>(y) + k_dy[k];
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(nx);
                const u16 uy = static_cast<u16>(ny);
                const u32 ni = static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux);
                if (tile_comp[ni] != 0xFFFFFFFFu) {
                    continue;
                }
                if (!land_ok(s, ux, uy) || !own_free_open(s, seat, ux, uy)) {
                    continue;
                }
                tile_comp[ni] = cid;
                q[qn++] = ni;
            }
        }
    }
    delete[] q;
    if (next_id == 0u) {
        return false;
    }
    *out_n = next_id;
    return true;
}

//================================================================================================================================
//=> - WarTurnHandler -
//================================================================================================================================

WarTurnHandler::WarTurnHandler (GameState& s, u16 seat) :
    m_st(s),
    m_seat(seat),
    m_walk(),
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
    m_mob_ok(false),
    m_stall(false),
    m_br_tot(0),
    m_br_last(0) {
    for (u16 i = 0; i < k_atk_cap; ++i) {
        m_atk[i] = U16_KEY_NULL;
        m_split[i] = U16_KEY_NULL;
    }
}

WarTurnHandler::~WarTurnHandler () {
}

bool WarTurnHandler::ok () const {
    return m_st.m_statics != nullptr
        && m_st.m_player_states != nullptr
        && m_seat < m_st.m_player_n
        && m_walk.ok()
        && m_mob.ok();
}

bool WarTurnHandler::set_goal (u16 x1, u16 y1, u16 x2, u16 y2) {
    m_ready = false;
    if (!ok()) {
        return false;
    }
    Whiteboard_1B acc("WarTurnHandler", "acc", 0u);
    if (!GenerateAccessMask::generate(m_st, m_seat, acc)) {
        return false;
    }
    u16 dmax = 0;
    if (!GenerateDistanceP2P::generate(
            m_st, *m_st.m_statics, x1, y1, x2, y2, m_walk, &acc, &dmax)) {
        return false;
    }
    m_ready = true;
    return true;
}

bool WarTurnHandler::make_muster_gradient (u16 x, u16 y) {
    m_mob_ok = false;
    m_grp_n = 0;
    if (!ok()) {
        return false;
    }
    Whiteboard_1B acc("WarTurnHandler", "muster_acc", 0u);
    if (!GenerateAccessMask::generate_own_free(m_st, m_seat, acc)) {
        return false;
    }
    u16 dmax = 0;
    if (!GenerateDistanceP2P::generate(m_st, *m_st.m_statics, x, y, m_mob, &acc, &dmax)) {
        return false;
    }
    m_sx = x;
    m_sy = y;
    m_mob_ok = true;
    return true;
}

u16 WarTurnHandler::do_total_muster () {
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
        m_grp[m_grp_n].m_hd = head.value();
        m_grp[m_grp_n].m_exp = GenerateExposure::k_none;
        m_grp_n++;
    }
    return m_grp_n;
}

bool WarTurnHandler::determine_exposure (u16 enemy) {
    if (!ok() || m_grp_n == 0u) {
        return false;
    }
    Whiteboard_1B board("WarTurnHandler", "exp", 0u);
    if (!GenerateExposure::generate(m_st, m_seat, enemy, board)) {
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
    if (!ok() || !m_mob_ok || m_grp_n == 0u) {
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
            const WalkP2P::StepRes step = m_mob.peek(m_st, u->m_x, u->m_y);
            if (!step.have) {
                break;
            }
            if (!UnitMovementMng::can_step(m_st, key, step.nx, step.ny, nullptr)) {
                break;
            }
            if (!UnitMovementMng::apply_step(m_st, key, step.nx, step.ny)) {
                break;
            }
            any = true;
        }
    }
    return any;
}

bool WarTurnHandler::form_army () {
    if (!ok() || !m_mob_ok) {
        return false;
    }
    UnitAddKey head = UnitAddKey::None();
    if (!UnitMovementMng::campaign_leave_five_defense(m_st, m_sx, m_sy, m_seat, &head)) {
        return false;
    }
    m_grp_n = 0;
    m_atk[0] = head.value();
    m_atk_n = 1u;
    for (u16 i = 1; i < k_atk_cap; ++i) {
        m_atk[i] = U16_KEY_NULL;
    }
    for (u16 i = 0; i < k_atk_cap; ++i) {
        m_split[i] = U16_KEY_NULL;
    }
    return true;
}

bool WarTurnHandler::find_enemy_seed (u16 enemy, u16* ox, u16* oy) const {
    if (ox == nullptr || oy == nullptr) {
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
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

bool WarTurnHandler::refill_targets (u16 enemy) {
    m_tgt_n = 0;
    m_tgt_i = 0;
    m_enemy = U8_KEY_NULL;
    u16 sx = 0;
    u16 sy = 0;
    if (!find_enemy_seed(enemy, &sx, &sy)) {
        return false;
    }
    TargetOrderingFlood flood;
    flood.set_enemy(static_cast<u8>(enemy));
    m_tgt_n = flood.fill(m_st, sx, sy, m_tgts, k_tgt_cap);
    if (m_tgt_n == 0u) {
        return false;
    }
    m_enemy = static_cast<u8>(enemy);
    return true;
}

bool WarTurnHandler::set_target_city (u16 enemy, u16* ox, u16* oy) {
    m_ready = false;
    if (!ok() || enemy >= m_st.m_player_n || enemy == m_seat) {
        return false;
    }
    for (u8 pass = 0; pass < 2u; ++pass) {
        if (pass > 0u || m_enemy != static_cast<u8>(enemy) || m_tgt_i >= m_tgt_n) {
            if (!refill_targets(enemy)) {
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
            if (ox != nullptr) {
                *ox = tx;
            }
            if (oy != nullptr) {
                *oy = ty;
            }
            return true;
        }
    }
    return false;
}

bool WarTurnHandler::walk_army () {
    if (!ok() || !m_ready || m_atk_n == 0u) {
        return false;
    }
    bool any = false;
    for (u16 i = 0; i < m_atk_n; ++i) {
        if (m_atk[i] == U16_KEY_NULL) {
            continue;
        }
        refill_grp(m_atk[i]);
        const UnitAddKey key = UnitAddKey::from_raw(m_atk[i]);
        UnitAddStruct* u = m_st.m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (u->m_x == m_tx && u->m_y == m_ty) {
            continue;
        }
        for (;;) {
            u = m_st.m_units.get_unit_add(key);
            if (u == nullptr || u->m_x == U16_KEY_NULL) {
                break;
            }
            if (u->m_x == m_tx && u->m_y == m_ty) {
                break;
            }
            const WalkP2P::StepRes step = m_walk.peek(m_st, u->m_x, u->m_y);
            if (!step.have) {
                break;
            }
            if (!UnitMovementMng::can_step(m_st, key, step.nx, step.ny, nullptr)) {
                break;
            }
            if (!UnitMovementMng::apply_step(m_st, key, step.nx, step.ny)) {
                break;
            }
            any = true;
        }
    }
    return any;
}

WarAssault WarTurnHandler::assault_city (u16 city_x, u16 city_y, u16 army_i) {
    m_stall = false;
    m_br_tot = 0u;
    m_br_last = 0u;
    if (!ok() || army_i >= k_atk_cap || m_atk[army_i] == U16_KEY_NULL) {
        return WarAssault::Fail;
    }
    UnitAddKey head = UnitAddKey::from_raw(m_atk[army_i]);
    const CityAttackResult ar = CityAttackManager::assault(m_st, &head, city_x, city_y);
    m_br_tot = ar.m_br_tot;
    m_br_last = ar.m_br_last;
    const UnitAddKey stay = ar.m_stay;
    const UnitAddKey occupy = ar.m_occupy;
    if (ar.m_r == CityAssault::Stall) {
        if (stay.is_valid()) {
            m_atk[army_i] = stay.value();
        } else if (head.is_valid()) {
            m_atk[army_i] = head.value();
        }
        m_stall = true;
        return WarAssault::Stall;
    }
    if (ar.m_r != CityAssault::Ok) {
        if (stay.is_valid()) {
            m_atk[army_i] = stay.value();
        }
        if (occupy.is_valid()) {
            m_atk[army_i] = occupy.value();
            m_split[army_i] = stay.is_valid() ? stay.value() : U16_KEY_NULL;
        } else if (head.is_valid()) {
            m_atk[army_i] = head.value();
        }
        return WarAssault::Fail;
    }
    if (occupy.is_valid()) {
        m_atk[army_i] = occupy.value();
        m_split[army_i] = stay.is_valid() ? stay.value() : U16_KEY_NULL;
    } else {
        m_atk[army_i] = stay.is_valid() ? stay.value() : U16_KEY_NULL;
        m_split[army_i] = U16_KEY_NULL;
    }
    if (army_i >= m_atk_n && m_atk[army_i] != U16_KEY_NULL) {
        m_atk_n = static_cast<u16>(army_i + 1u);
    }
    claim_city(city_x, city_y);
    m_sx = city_x;
    m_sy = city_y;
    m_tx = city_x;
    m_ty = city_y;
    return WarAssault::Ok;
}

void WarTurnHandler::claim_city (u16 x, u16 y) {
    const u16 cn = m_st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = m_st.m_cities.get_city(i);
        if (c == nullptr || c->get_x() != x || c->get_y() != y) {
            continue;
        }
        const u8 from = static_cast<u8>(c->get_owner());
        c->set_owner(m_seat);
        m_st.m_map.set_civ_owner(x, y, static_cast<u8>(m_seat));
        TileTransfer::apply(m_st, i, from, static_cast<u8>(m_seat), nullptr);
        return;
    }
}

bool WarTurnHandler::rejoin_move (u16 army_i) {
    if (!ok() || army_i >= k_atk_cap) {
        return false;
    }
    if (m_split[army_i] == U16_KEY_NULL) {
        return true;
    }
    if (m_atk[army_i] == U16_KEY_NULL) {
        return false;
    }
    const UnitAddStruct* occ = m_st.m_units.get_unit_add(UnitAddKey::from_raw(m_atk[army_i]));
    if (occ == nullptr || occ->m_x == U16_KEY_NULL) {
        return false;
    }
    const u16 dx = occ->m_x;
    const u16 dy = occ->m_y;
    refill_grp(m_split[army_i]);
    const UnitAddKey key = UnitAddKey::from_raw(m_split[army_i]);
    UnitAddStruct* u = m_st.m_units.get_unit_add(key);
    if (u == nullptr || u->m_x == U16_KEY_NULL) {
        return false;
    }
    if (u->m_x == dx && u->m_y == dy) {
        return true;
    }
    if (!UnitMovementMng::can_step(m_st, key, dx, dy, nullptr)) {
        return false;
    }
    return UnitMovementMng::apply_step(m_st, key, dx, dy);
}

bool WarTurnHandler::rejoin_link (u16 army_i) {
    if (!ok() || army_i >= k_atk_cap) {
        return false;
    }
    if (m_split[army_i] == U16_KEY_NULL) {
        return true;
    }
    if (m_atk[army_i] == U16_KEY_NULL) {
        return false;
    }
    const UnitAddKey occ = UnitAddKey::from_raw(m_atk[army_i]);
    const UnitAddKey sty = UnitAddKey::from_raw(m_split[army_i]);
    const UnitAddStruct* ou = m_st.m_units.get_unit_add(occ);
    UnitAddStruct* su = m_st.m_units.get_unit_add(sty);
    if (ou == nullptr || su == nullptr || ou->m_x == U16_KEY_NULL || su->m_x == U16_KEY_NULL) {
        return false;
    }
    if (ou->m_x != su->m_x || ou->m_y != su->m_y) {
        return false;
    }
    while (su->m_next_unit_in_group != U16_KEY_NULL) {
        const UnitAddKey nxt = UnitAddKey::from_raw(su->m_next_unit_in_group);
        if (!UnitMovementMng::unlink_group(m_st, nxt)) {
            return false;
        }
        if (!UnitMovementMng::link_group(m_st, occ, nxt)) {
            return false;
        }
        su = m_st.m_units.get_unit_add(sty);
        if (su == nullptr) {
            return false;
        }
    }
    if (!UnitMovementMng::link_group(m_st, occ, sty)) {
        return false;
    }
    m_split[army_i] = U16_KEY_NULL;
    return true;
}

u16 WarTurnHandler::rest_heal (u16 army_i) {
    if (!ok() || army_i >= k_atk_cap || m_atk[army_i] == U16_KEY_NULL || m_st.m_statics == nullptr) {
        return 0u;
    }
    u8 worst = UNIT_HEALTH;
    UnitAddKey cur = UnitAddKey::from_raw(m_atk[army_i]);
    while (cur.is_valid()) {
        const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
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
    const u16 pct = m_st.m_statics->config().get_unit_heal_in_city();
    const u16 add = static_cast<u16>((static_cast<u32>(UNIT_HEALTH) * static_cast<u32>(pct)) / 100u);
    if (add == 0u) {
        return 0u;
    }
    const u16 need = static_cast<u16>(UNIT_HEALTH - worst);
    const u16 turns = static_cast<u16>((need + add - 1u) / add);
    for (u16 t = 0; t < turns; ++t) {
        refill_grp(m_atk[army_i]);
        UnitTurnHandler::handle(m_st, m_atk[army_i]);
    }
    return turns;
}

bool WarTurnHandler::army_can_fight (u16 army_i) const {
    if (m_st.m_statics == nullptr || army_i >= k_atk_cap || m_atk[army_i] == U16_KEY_NULL) {
        return false;
    }
    const UnitTypeActionMap& am = m_st.m_statics->unit_type_action_map();
    const u16 typ_n = m_st.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(m_atk[army_i]);
    while (cur.is_valid()) {
        const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u && u->m_unit_typ_idx < typ_n) {
            const u16 ut = m_st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
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

bool WarTurnHandler::can_cont (u16 army_i) const {
    if (m_st.m_statics == nullptr || army_i >= k_atk_cap || m_atk[army_i] == U16_KEY_NULL) {
        return false;
    }
    const UnitTypeActionMap& am = m_st.m_statics->unit_type_action_map();
    const u16 typ_n = m_st.m_statics->unit().get_item_count();
    UnitAddKey cur = UnitAddKey::from_raw(m_atk[army_i]);
    while (cur.is_valid()) {
        const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u && u->m_unit_typ_idx < typ_n) {
            const u16 pts = m_st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
            const u16 ut = m_st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
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

bool WarTurnHandler::stalled () const {
    return m_stall;
}

u32 WarTurnHandler::br_tot () const {
    return m_br_tot;
}

u32 WarTurnHandler::br_last () const {
    return m_br_last;
}

bool WarTurnHandler::pick_staging_city (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr || s.m_player_states == nullptr) {
        return false;
    }
    if (seat >= s.m_player_n || enemy >= s.m_player_n || seat == enemy) {
        return false;
    }
    const u16 cn = s.m_cities.get_city_count();
    if (cn == 0u) {
        return false;
    }
    const u32 tn = s.m_map.tile_n();
    u32* tile_comp = new u32[tn];
    if (tile_comp == nullptr) {
        return false;
    }
    u32 comp_n = 0;
    if (!label_own_free_comp(s, seat, tile_comp, &comp_n)) {
        delete[] tile_comp;
        return false;
    }
    u32* city_n = new u32[comp_n];
    if (city_n == nullptr) {
        delete[] tile_comp;
        return false;
    }
    for (u32 i = 0; i < comp_n; ++i) {
        city_n[i] = 0;
    }
    const u16 w = s.m_map.width();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u32 ti = static_cast<u32>(c->get_y()) * static_cast<u32>(w) + static_cast<u32>(c->get_x());
        if (ti >= tn) {
            continue;
        }
        const u32 cid = tile_comp[ti];
        if (cid < comp_n) {
            city_n[cid]++;
        }
    }
    u32 best_comp = 0xFFFFFFFFu;
    u32 best_cn = 0;
    for (u32 i = 0; i < comp_n; ++i) {
        if (city_n[i] > best_cn) {
            best_cn = city_n[i];
            best_comp = i;
        }
    }
    delete[] city_n;
    if (best_comp == 0xFFFFFFFFu || best_cn == 0u) {
        delete[] tile_comp;
        return false;
    }
    u32 best_sc = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u16 sx = c->get_x();
        const u16 sy = c->get_y();
        const u32 ti = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
        if (ti >= tn || tile_comp[ti] != best_comp) {
            continue;
        }
        u32 min_d = 0xFFFFFFFFu;
        u64 esx = 0;
        u64 esy = 0;
        u32 en = 0;
        for (u16 j = 0; j < cn; ++j) {
            const City* e = s.m_cities.get_city(j);
            if (e == nullptr || e->get_owner() != enemy) {
                continue;
            }
            const u16 ex = e->get_x();
            const u16 ey = e->get_y();
            esx += ex;
            esy += ey;
            en++;
            const u32 adx = sx > ex ? static_cast<u32>(sx - ex) : static_cast<u32>(ex - sx);
            const u32 ady = sy > ey ? static_cast<u32>(sy - ey) : static_cast<u32>(ey - sy);
            const u32 d = adx + ady;
            if (d < min_d) {
                min_d = d;
            }
        }
        if (en == 0u || min_d == 0xFFFFFFFFu) {
            continue;
        }
        const u16 ecx = static_cast<u16>(esx / en);
        const u16 ecy = static_cast<u16>(esy / en);
        const u32 cdx = sx > ecx ? static_cast<u32>(sx - ecx) : static_cast<u32>(ecx - sx);
        const u32 cdy = sy > ecy ? static_cast<u32>(sy - ecy) : static_cast<u32>(ecy - sy);
        const u32 sc = min_d * 64u + cdx + cdy;
        if (!found || sc < best_sc) {
            best_sc = sc;
            bx = sx;
            by = sy;
            found = true;
        }
    }
    delete[] tile_comp;
    if (!found) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

u16 WarTurnHandler::muster_n () const {
    return m_grp_n;
}

bool WarTurnHandler::is_exposed (u16 i) const {
    return i < m_grp_n && m_grp[i].m_exp < k_exp_lim;
}

u16 WarTurnHandler::atk_hd (u16 i) const {
    return (i < k_atk_cap) ? m_atk[i] : U16_KEY_NULL;
}

u16 WarTurnHandler::split_hd (u16 i) const {
    return (i < k_atk_cap) ? m_split[i] : U16_KEY_NULL;
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
//=> - End of file -
//================================================================================================================================
