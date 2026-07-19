//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "settler_turn_mng.h"
#include "city.h"
#include "city_blocking_mask.h"
#include "city_border.h"
#include "game_state.h"
#include "point_seq_flood_walker.h"
#include "runtime_statics.h"
#include "sense_settling_pts_opt.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u16 k_slot_n = static_cast<u16>(SETTLER_MISSION_SLOTS);
static const u16 k_tgt_sites = static_cast<u16>(SETTLER_MISSION_SLOTS);
static const u16 k_tgt_none = 2u;
static const u16 k_claim_cult = 25u;
static const u8 k_add_typ_city = 1u;

//================================================================================================================================
//=> - Slot state -
//================================================================================================================================

struct StmSlot {
    PointSeqFloodWalker m_walk;
    u16 m_tx;
    u16 m_ty;
    u8 m_has;
};

static bool g_ok = false;
static GameState* g_st = nullptr;
static u8* g_terr = nullptr;
static u16 g_w = 0;
static u16 g_h = 0;
static u16 g_player_n = 0;
static StmSlot* g_slot = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16* slot_idx_ptr (PlayerState& ps, u16 i) {
    return &ps.m_settler_idx[i];
}

static StmSlot* slot_at (u16 player, u16 i) {
    return &g_slot[static_cast<u32>(player) * static_cast<u32>(k_slot_n) + static_cast<u32>(i)];
}

static bool is_settler_typ (const GameState& state, u16 typ_idx) {
    if (state.m_statics == nullptr) {
        return false;
    }
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = state.m_statics->unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(state.m_statics->unit_type().get_name(tk), "LAND_SETTLER") == 0;
}

static UnitAddStruct* unit_at (GameState& state, u16 unit_idx) {
    return state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
}

static bool find_unit_slot (u16 player, u16 unit_idx, u16* out_i) {
    if (player >= g_player_n || out_i == nullptr) {
        return false;
    }
    PlayerState& ps = g_st->m_player_states[player];
    for (u16 i = 0; i < k_slot_n; ++i) {
        if (*slot_idx_ptr(ps, i) == unit_idx) {
            *out_i = i;
            return true;
        }
    }
    return false;
}

static bool take_free_slot (u16 player, u16 unit_idx, u16* out_i) {
    if (player >= g_player_n || out_i == nullptr) {
        return false;
    }
    PlayerState& ps = g_st->m_player_states[player];
    for (u16 i = 0; i < k_slot_n; ++i) {
        u16* p = slot_idx_ptr(ps, i);
        if (*p == U16_KEY_NULL) {
            *p = unit_idx;
            *out_i = i;
            slot_at(player, i)->m_has = 0;
            slot_at(player, i)->m_tx = U16_KEY_NULL;
            slot_at(player, i)->m_ty = U16_KEY_NULL;
            return true;
        }
    }
    return false;
}

static void clear_slot (u16 player, u16 i) {
    if (player >= g_player_n || i >= k_slot_n) {
        return;
    }
    *slot_idx_ptr(g_st->m_player_states[player], i) = U16_KEY_NULL;
    StmSlot* sl = slot_at(player, i);
    sl->m_has = 0;
    sl->m_tx = U16_KEY_NULL;
    sl->m_ty = U16_KEY_NULL;
}

static void stamp_block (GameState& state, u16 cx, u16 cy) {
    CityBlockingMask::stamp(state.m_map, cx, cy);
}

static void stamp_all_cities (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        stamp_block(state, c->get_x(), c->get_y());
    }
}

static bool tgt_taken (u16 player, u16 x, u16 y) {
    for (u16 s = 0; s < k_slot_n; ++s) {
        StmSlot* o = slot_at(player, s);
        if (o->m_has != 0 && o->m_tx == x && o->m_ty == y) {
            return true;
        }
    }
    return false;
}

static bool try_assign (GameState& state, u16 player, u16 i, UnitAddStruct* unit) {
    StmSlot* sl = slot_at(player, i);
    if (sl->m_has != 0) {
        return true;
    }
    PlayerState& ps = state.m_player_states[player];
    if (ps.m_target_settlements == 0) {
        return false;
    }
    if (!sl->m_walk.is_valid()) {
        return false;
    }
    const SmSettlerBestPts best = SenseSettlingPtsOpt::select_and_pick_pts(state.m_map, state.m_cities, player);
    for (u16 k = 0; k < best.n; ++k) {
        const u16 tx = best.pts[k].x;
        const u16 ty = best.pts[k].y;
        if (tx == U16_KEY_NULL || ty == U16_KEY_NULL) {
            continue;
        }
        if (tgt_taken(player, tx, ty)) {
            continue;
        }
        if (state.m_map.get_settler_blocked(tx, ty) != 0) {
            continue;
        }
        if (sl->m_walk.start(unit->m_x, unit->m_y, tx, ty)) {
            sl->m_tx = tx;
            sl->m_ty = ty;
            sl->m_has = 1;
            return true;
        }
    }
    return false;
}

static bool found_city (GameState& state, u16 x, u16 y, u16 player) {
    if (state.m_map.get_add_idx(x, y) != U16_KEY_NULL) {
        return false;
    }
    if (state.m_map.get_settler_blocked(x, y) != 0) {
        return false;
    }
    const u16 city_idx = state.m_cities.get_next_new_city_idx();
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(player, x, y);
    if (!state.m_map.set_tile_add(x, y, city_idx, k_add_typ_city)) {
        return false;
    }
    CityBorder::claim_expand(x, y, 0, k_claim_cult, static_cast<u8>(player));
    stamp_block(state, x, y);
    return true;
}

static void finish_mission (GameState& state, u16 player, u16 i, UnitAddStruct* unit) {
    StmSlot* sl = slot_at(player, i);
    const u16 tx = sl->m_tx;
    const u16 ty = sl->m_ty;
    if (unit != nullptr && tx != U16_KEY_NULL && ty != U16_KEY_NULL) {
        if (unit->m_x == tx && unit->m_y == ty) {
            found_city(state, tx, ty, player);
        }
    }
    clear_slot(player, i);
}

static void step_mission (GameState& state, u16 player, u16 i, u16 unit_idx, UnitAddStruct* unit) {
    StmSlot* sl = slot_at(player, i);
    if (sl->m_has == 0) {
        return;
    }
    if (sl->m_walk.done()) {
        finish_mission(state, player, i, unit);
        return;
    }
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    if (!sl->m_walk.step()) {
        if (sl->m_walk.done()) {
            finish_mission(state, player, i, unit);
        } else {
            clear_slot(player, i);
        }
        return;
    }
    const u16 nx = sl->m_walk.x();
    const u16 ny = sl->m_walk.y();
    if (nx == unit->m_x && ny == unit->m_y) {
        if (sl->m_walk.done()) {
            finish_mission(state, player, i, unit);
        }
        return;
    }
    if (!UnitMovementMng::apply_step(state, key, nx, ny)) {
        clear_slot(player, i);
        return;
    }
    if (sl->m_walk.done()) {
        finish_mission(state, player, i, unit);
    }
}

//================================================================================================================================
//=> - SettlerTurnMng -
//================================================================================================================================

bool SettlerTurnMng::begin (GameState& state) {
    clear();
    if (state.m_player_states == nullptr || state.m_player_n == 0) {
        return false;
    }
    if (state.m_statics == nullptr) {
        return false;
    }
    if (!UnitMovementMng::mvt_ready() && !UnitMovementMng::setup_mvt_costs(*state.m_statics)) {
        return false;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 slot_n = static_cast<u32>(state.m_player_n) * static_cast<u32>(k_slot_n);
    g_terr = new u8[n];
    g_slot = new StmSlot[slot_n];
    if (g_terr == nullptr || g_slot == nullptr) {
        clear();
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            g_terr[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = state.m_map.get_terrain(x, y);
        }
    }
    g_st = &state;
    g_w = w;
    g_h = h;
    g_player_n = state.m_player_n;
    for (u32 i = 0; i < slot_n; ++i) {
        g_slot[i].m_has = 0;
        g_slot[i].m_tx = U16_KEY_NULL;
        g_slot[i].m_ty = U16_KEY_NULL;
        if (!g_slot[i].m_walk.begin(state.m_sector_net, state.m_sector_rt, g_terr, w, h)) {
            clear();
            return false;
        }
    }
    stamp_all_cities(state);
    g_ok = true;
    return true;
}

void SettlerTurnMng::clear () {
    delete[] g_terr;
    delete[] g_slot;
    g_terr = nullptr;
    g_slot = nullptr;
    g_st = nullptr;
    g_w = 0;
    g_h = 0;
    g_player_n = 0;
    g_ok = false;
}

void SettlerTurnMng::refresh_targets (GameState& state) {
    if (!g_ok || g_st != &state || state.m_player_states == nullptr) {
        return;
    }
    for (u16 p = 0; p < g_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_target_settlements == 0) {
            continue;
        }
        const SmSettlerBestPts best = SenseSettlingPtsOpt::select_and_pick_pts(state.m_map, state.m_cities, p);
        ps.m_target_settlements = (best.n > 0) ? k_tgt_sites : k_tgt_none;
    }
}

void SettlerTurnMng::begin_unit_pass (GameState& state) {
    if (!g_ok || g_st != &state || state.m_player_states == nullptr) {
        return;
    }
    for (u16 p = 0; p < g_player_n; ++p) {
        state.m_player_states[p].m_last_turn_settler_count = 0;
    }
}

bool SettlerTurnMng::need_settler (GameState& state, u16 player) {
    if (!g_ok || g_st != &state || player >= g_player_n) {
        return false;
    }
    PlayerState& ps = state.m_player_states[player];
    if (ps.m_target_settlements == 0) {
        return false;
    }
    return ps.m_last_turn_settler_count < ps.m_target_settlements;
}

void SettlerTurnMng::handle (GameState& state, u16 unit_idx) {
    if (!g_ok || g_st != &state) {
        return;
    }
    UnitAddStruct* unit = unit_at(state, unit_idx);
    if (unit == nullptr || unit->m_x == U16_KEY_NULL) {
        return;
    }
    if (!is_settler_typ(state, unit->m_unit_typ_idx)) {
        return;
    }
    const u16 player = unit->m_player_idx;
    if (player >= g_player_n) {
        return;
    }
    PlayerState& ps = state.m_player_states[player];
    ps.m_last_turn_settler_count = static_cast<u16>(ps.m_last_turn_settler_count + 1u);
    if (ps.m_target_settlements == 0) {
        return;
    }
    u16 i = 0;
    if (!find_unit_slot(player, unit_idx, &i)) {
        if (!take_free_slot(player, unit_idx, &i)) {
            return;
        }
    }
    StmSlot* sl = slot_at(player, i);
    if (sl->m_has == 0) {
        if (!try_assign(state, player, i, unit)) {
            return;
        }
    }
    step_mission(state, player, i, unit_idx, unit);
}

bool SettlerTurnMng::tgt_xy (u16 player, u16 slot, u16* x, u16* y) {
    if (!g_ok || player >= g_player_n || slot >= k_slot_n || x == nullptr || y == nullptr) {
        return false;
    }
    StmSlot* sl = slot_at(player, slot);
    if (sl->m_has == 0 || sl->m_tx == U16_KEY_NULL || sl->m_ty == U16_KEY_NULL) {
        return false;
    }
    *x = sl->m_tx;
    *y = sl->m_ty;
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
