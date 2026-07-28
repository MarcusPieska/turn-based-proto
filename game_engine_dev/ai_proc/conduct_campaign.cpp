//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "conduct_campaign.h"

#include "attack_city.h"
#include "city.h"
#include "city_array.h"
#include "civ_relations.h"
#include "civ_static_key.h"
#include "game_array_simple.h"
#include "generate_access_mask.h"
#include "generate_distance_p2p.h"
#include "generate_exposure.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool fill_own_mask (const GameState& s, u16 seat, Whiteboard_1B& out) {
    if (!out.ok()) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    if (out.w() != w || out.h() != h) {
        return false;
    }
    const u8 self = static_cast<u8>(seat);
    const u32 n = s.m_map.tile_n();
    u8* m = out.get_iter_ptr();
    for (u32 i = 0; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        m[i] = (s.m_map.get_civ_owner(x, y) == self) ? GenerateAccessMask::k_open : GenerateAccessMask::k_block;
    }
    return true;
}

static bool mob_hit (const WalkP2P& mob, u16 x, u16 y) {
    if (!mob.ok() || x >= mob.w() || y >= mob.h()) {
        return false;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(mob.w()) + static_cast<u32>(x);
    return mob.turn()[i] != GenerateDistanceP2P::k_turn_sent;
}

//================================================================================================================================
//=> - ConductCampaign -
//================================================================================================================================

ConductCampaign::ConductCampaign (GameState& s, u16 seat) :
    m_st(s),
    m_seat(seat),
    m_walk(),
    m_mob(),
    m_grp_n(0),
    m_atk_n(0),
    m_sx(0),
    m_sy(0),
    m_tx(0),
    m_ty(0),
    m_ready(false),
    m_mob_ok(false) {
    for (u16 i = 0; i < k_atk_cap; ++i) {
        m_atk[i] = U16_KEY_NULL;
        m_split[i] = U16_KEY_NULL;
    }
}

ConductCampaign::~ConductCampaign () {
}

bool ConductCampaign::ok () const {
    return m_st.m_statics != nullptr
        && m_st.m_player_states != nullptr
        && m_seat < m_st.m_player_n
        && m_walk.ok()
        && m_mob.ok();
}

bool ConductCampaign::set_goal (u16 x1, u16 y1, u16 x2, u16 y2) {
    m_ready = false;
    if (!ok()) {
        return false;
    }
    Whiteboard_1B acc("ConductCampaign", "acc", 0u);
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

bool ConductCampaign::make_muster_gradient (u16 x, u16 y) {
    m_mob_ok = false;
    m_grp_n = 0;
    if (!ok()) {
        return false;
    }
    Whiteboard_1B own("ConductCampaign", "own", 0u);
    if (!fill_own_mask(m_st, m_seat, own)) {
        return false;
    }
    u16 dmax = 0;
    if (!GenerateDistanceP2P::generate(m_st, *m_st.m_statics, x, y, m_mob, &own, &dmax)) {
        return false;
    }
    m_sx = x;
    m_sy = y;
    m_mob_ok = true;
    return true;
}

u16 ConductCampaign::do_total_muster () {
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

bool ConductCampaign::determine_exposure (u16 enemy) {
    if (!ok() || m_grp_n == 0u) {
        return false;
    }
    Whiteboard_1B board("ConductCampaign", "exp", 0u);
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

void ConductCampaign::refill_grp (u16 head_idx) {
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

bool ConductCampaign::walk_muster () {
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

bool ConductCampaign::form_army () {
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

bool ConductCampaign::set_target_city (u16 enemy, u16* ox, u16* oy) {
    m_ready = false;
    if (!ok() || enemy >= m_st.m_player_n || enemy == m_seat) {
        return false;
    }
    u16 tx = 0;
    u16 ty = 0;
    if (!pick_target_city(m_st, m_seat, enemy, m_sx, m_sy, &tx, &ty)) {
        return false;
    }
    const u16 self_civ = m_st.m_player_states[m_seat].m_civ_index;
    const u16 enemy_civ = m_st.m_player_states[enemy].m_civ_index;
    m_st.m_civ_relations.set(
        CivStaticDataKey::from_raw(self_civ),
        CivStaticDataKey::from_raw(enemy_civ),
        CivRel::CIV_REL_WAR);
    if (!set_goal(m_sx, m_sy, tx, ty)) {
        return false;
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

bool ConductCampaign::walk_army () {
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

bool ConductCampaign::assault_city (u16 city_x, u16 city_y, u16 army_i) {
    if (!ok() || army_i >= k_atk_cap || m_atk[army_i] == U16_KEY_NULL) {
        return false;
    }
    const UnitAddKey army = UnitAddKey::from_raw(m_atk[army_i]);
    UnitAddKey stay = UnitAddKey::None();
    UnitAddKey occupy = UnitAddKey::None();
    if (!AttackCity::assault(m_st, army, city_x, city_y, &stay, &occupy)) {
        return false;
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
    return true;
}

void ConductCampaign::claim_city (u16 x, u16 y) {
    const u16 cn = m_st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = m_st.m_cities.get_city(i);
        if (c == nullptr || c->get_x() != x || c->get_y() != y) {
            continue;
        }
        c->set_owner(m_seat);
        m_st.m_map.set_civ_owner(x, y, static_cast<u8>(m_seat));
        return;
    }
}

bool ConductCampaign::rejoin_move (u16 army_i) {
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

bool ConductCampaign::rejoin_link (u16 army_i) {
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

bool ConductCampaign::army_can_fight (u16 army_i) const {
    if (m_st.m_statics == nullptr || army_i >= k_atk_cap || m_atk[army_i] == U16_KEY_NULL) {
        return false;
    }
    UnitAddKey cur = UnitAddKey::from_raw(m_atk[army_i]);
    while (cur.is_valid()) {
        const UnitAddStruct* u = m_st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        if (u->m_health > 0u) {
            const u16 t = u->m_unit_typ_idx;
            const u16 n = m_st.m_statics->unit().get_item_count();
            if (t < n) {
                const u16 ut = m_st.m_statics->unit().get_item(UnitStaticDataKey::from_raw(t)).type;
                if (ut == m_st.m_land_attack_type_idx || ut == m_st.m_land_artillery_type_idx) {
                    return true;
                }
            }
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return false;
}

bool ConductCampaign::pick_staging_city (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy) {
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
    if (!found) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

bool ConductCampaign::pick_target_city (
    const GameState& s,
    u16 seat,
    u16 enemy,
    u16 from_x,
    u16 from_y,
    u16* ox,
    u16* oy) {
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
    u32 best_d = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
            continue;
        }
        const u16 ex = c->get_x();
        const u16 ey = c->get_y();
        const u32 adx = from_x > ex ? static_cast<u32>(from_x - ex) : static_cast<u32>(ex - from_x);
        const u32 ady = from_y > ey ? static_cast<u32>(from_y - ey) : static_cast<u32>(ey - from_y);
        const u32 d = adx + ady;
        if (!found || d < best_d) {
            best_d = d;
            bx = ex;
            by = ey;
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

u16 ConductCampaign::muster_n () const {
    return m_grp_n;
}

bool ConductCampaign::is_exposed (u16 i) const {
    return i < m_grp_n && m_grp[i].m_exp < k_exp_lim;
}

u16 ConductCampaign::atk_hd (u16 i) const {
    return (i < k_atk_cap) ? m_atk[i] : U16_KEY_NULL;
}

u16 ConductCampaign::split_hd (u16 i) const {
    return (i < k_atk_cap) ? m_split[i] : U16_KEY_NULL;
}

u16 ConductCampaign::staging_x () const {
    return m_sx;
}

u16 ConductCampaign::staging_y () const {
    return m_sy;
}

u16 ConductCampaign::target_x () const {
    return m_tx;
}

u16 ConductCampaign::target_y () const {
    return m_ty;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
