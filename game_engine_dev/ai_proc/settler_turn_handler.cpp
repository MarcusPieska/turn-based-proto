//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "settler_turn_handler.h"
#include "assert_log.h"
#include "build_adds_array.h"
#include "city.h"
#include "city_blocking_mask.h"
#include "city_border.h"
#include "game_state.h"
#include "gen_ai_helpers.h"
#include "gen_settlement_order.h"
#include "settler_mission_manager.h"
#include "runtime_statics.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Limits -
//================================================================================================================================

static const u16 k_slot_n = static_cast<u16>(SETTLER_MISSION_SLOTS);
static const u16 k_min_settlers = 3u;
static const u16 k_claim_cult = 25u;

//================================================================================================================================
//=> - Slot state -
//================================================================================================================================

struct StmSlot {
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
static SettlerMissionManager* g_mgrs = nullptr;
static GenSettlementOrder* g_ord = nullptr;
static bool g_ord_ok = false;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16* slot_idx_ptr (PlayerState& ps, u16 i) {
    return &ps.m_settler_idx[i];
}

static StmSlot* slot_at (u16 player, u16 i) {
    return &g_slot[static_cast<u32>(player) * static_cast<u32>(k_slot_n) + static_cast<u32>(i)];
}

static UnitAddStruct* unit_at (GameState& state, u16 unit_idx) {
    return state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
}

static bool find_unit_slot (u16 player, u16 unit_idx, u16* out_i) {
    GAME_EXPECT(out_i != nullptr, "find_unit_slot got nullptr out_i");
    GAME_EXPECT(g_st != nullptr, "find_unit_slot missing game state");
    GAME_EXPECT(player < g_player_n, "find_unit_slot player out of range");
    PlayerState& ps = g_st->m_player_states[player];
    for (u16 i = 0; i < k_slot_n; ++i) {
        if (*slot_idx_ptr(ps, i) == unit_idx) {
            *out_i = i;
            return true;
        }
    }
    return false;
}

static void clear_slot (u16 player, u16 i) {
    GAME_EXPECT(g_st != nullptr, "clear_slot missing game state");
    GAME_EXPECT(player < g_player_n, "clear_slot player out of range");
    GAME_EXPECT(i < k_slot_n, "clear_slot slot out of range");
    PlayerState& ps = g_st->m_player_states[player];
    StmSlot* sl = slot_at(player, i);
    *slot_idx_ptr(ps, i) = U16_KEY_NULL;
    sl->m_has = 0;
    sl->m_tx = U16_KEY_NULL;
    sl->m_ty = U16_KEY_NULL;
}

static u16 count_elig_sites (GameState& state, u16 player) {
    GAME_EXPECT(g_ord != nullptr, "count_elig_sites missing order");
    u16 n = 0;
    const u32 sn = g_ord->n(player);
    for (u32 i = 0; i < sn; ++i) {
        const SpgCoordPair pt = g_ord->at(player, i);
        if (state.m_map.get_planned_city(pt.x, pt.y) == 0u) {
            continue;
        }
        if (state.m_map.get_settler_blocked(pt.x, pt.y) != 0u) {
            continue;
        }
        if (state.m_map.get_add_typ(pt.x, pt.y) == BUILD_ADD_CITY) {
            continue;
        }
        const u8 ow = state.m_map.get_civ_owner(pt.x, pt.y);
        if (ow != U8_KEY_NULL && ow != static_cast<u8>(player)) {
            continue;
        }
        n = static_cast<u16>(n + 1u);
    }
    return n;
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

static bool found_city (GameState& state, u16 x, u16 y, u16 player) {
    GAME_EXPECT(state.m_map.get_add_typ(x, y) != BUILD_ADD_CITY, "found_city tile already city");
    const u16 city_idx = state.m_cities.get_next_new_city_idx();
    City* city = state.m_cities.get_city(city_idx);
    GAME_EXPECT(city != nullptr, "found_city city slot unavailable");
    city->init(player, x, y);
    GAME_EXPECT(state.m_map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY), "found_city set_tile_add failed");
    GAME_EXPECT(state.city_net_on_found(city_idx), "found_city city_net_on_found failed");
    CityBorder::claim_expand(x, y, 0, k_claim_cult, static_cast<u8>(player));
    stamp_block(state, x, y);
    return true;
}

//================================================================================================================================
//=> - SettlerTurnHandler -
//================================================================================================================================

bool SettlerTurnHandler::begin (GameState& state) {
    clear();
    GAME_EXPECT(state.m_player_n <= 200u, "SettlerTurnHandler begin player count exceeds start buffer");
    GAME_EXPECT(state.m_player_states != nullptr, "SettlerTurnHandler begin missing player states");
    GAME_EXPECT(state.m_player_n != 0, "SettlerTurnHandler begin got zero players");
    GAME_EXPECT(state.m_statics != nullptr, "SettlerTurnHandler begin missing statics");
    if (!UnitMovementMng::mvt_ready() && !UnitMovementMng::setup_mvt_costs(*state.m_statics)) {
        return false;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    GAME_EXPECT(w != 0 && h != 0, "SettlerTurnHandler begin got empty map dimensions");
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 slot_n = static_cast<u32>(state.m_player_n) * static_cast<u32>(k_slot_n);
    g_terr = new u8[n];
    g_slot = new StmSlot[slot_n];
    g_mgrs = new SettlerMissionManager[state.m_player_n];
    GAME_EXPECT(g_terr != nullptr, "SettlerTurnHandler begin terrain alloc failed");
    GAME_EXPECT(g_slot != nullptr, "SettlerTurnHandler begin slot alloc failed");
    GAME_EXPECT(g_mgrs != nullptr, "SettlerTurnHandler begin manager alloc failed");
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            g_terr[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = state.m_map.get_terrain(x, y);
        }
    }
    g_st = &state;
    g_w = w;
    g_h = h;
    g_player_n = state.m_player_n;
    GAME_EXPECT(state.m_ai_help.ok(), "SettlerTurnHandler begin missing GenAiHelpers");
    g_ord = &state.m_ai_help.order();
    GAME_EXPECT(g_ord != nullptr && g_ord->ok(), "SettlerTurnHandler begin order checkout failed");
    for (u16 p = 0; p < g_player_n; ++p) {
        if (!g_mgrs[p].begin(state.m_sector_net, state.m_sector_rt, g_terr, w, h)) {
            clear();
            return false;
        }
        g_mgrs[p].opp(false);
    }
    for (u32 i = 0; i < slot_n; ++i) {
        g_slot[i].m_has = 0;
        g_slot[i].m_tx = U16_KEY_NULL;
        g_slot[i].m_ty = U16_KEY_NULL;
    }
    stamp_all_cities(state);
    g_ok = true;
    return true;
}

void SettlerTurnHandler::clear () {
    delete[] g_terr;
    delete[] g_slot;
    delete[] g_mgrs;
    g_terr = nullptr;
    g_slot = nullptr;
    g_mgrs = nullptr;
    g_ord = nullptr;
    g_st = nullptr;
    g_w = 0;
    g_h = 0;
    g_player_n = 0;
    g_ord_ok = false;
    g_ok = false;
}

void SettlerTurnHandler::refresh_targets (GameState& state) {
    GAME_EXPECT(g_ok, "SettlerTurnHandler refresh_targets got invalid state");
    GAME_EXPECT(g_st == &state, "SettlerTurnHandler refresh_targets got wrong state");
    GAME_EXPECT(state.m_player_states != nullptr, "SettlerTurnHandler refresh_targets missing player states");
    GAME_EXPECT(g_ord != nullptr, "SettlerTurnHandler refresh_targets missing order object");
    if (!g_ord_ok) {
        GAME_EXPECT(g_ord != nullptr && g_ord->ok(), "SettlerTurnHandler refresh_targets missing order");
        SettlerMissionManager::punch(state.m_map);
        g_ord_ok = true;
    }
    for (u16 p = 0; p < g_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_target_settlements == 0) {
            continue;
        }
        u16 t = count_elig_sites(state, p);
        if (t < k_min_settlers) {
            t = k_min_settlers;
        }
        if (t > k_slot_n) {
            t = k_slot_n;
        }
        ps.m_target_settlements = t;
    }
}

u16 SettlerTurnHandler::elig_sites (GameState& state, u16 player) {
    GAME_EXPECT(g_ok, "SettlerTurnHandler elig_sites got invalid state");
    GAME_EXPECT(g_st == &state, "SettlerTurnHandler elig_sites got wrong state");
    GAME_EXPECT(player < g_player_n, "SettlerTurnHandler elig_sites player out of range");
    return count_elig_sites(state, player);
}

bool SettlerTurnHandler::need_settler (GameState& state, u16 player) {
    GAME_EXPECT(g_ok, "SettlerTurnHandler need_settler got invalid state");
    GAME_EXPECT(g_st == &state, "SettlerTurnHandler need_settler got wrong state");
    GAME_EXPECT(player < g_player_n, "SettlerTurnHandler need_settler player out of range");
    PlayerState& ps = state.m_player_states[player];
    if (ps.m_target_settlements == 0) {
        return false;
    }
    const u32 have = static_cast<u32>(ps.m_last_turn_settler_count)
        + static_cast<u32>(ps.m_last_turn_settler_build_n)
        + static_cast<u32>(ps.m_this_turn_settler_build_n);
    return have < ps.m_target_settlements;
}

void SettlerTurnHandler::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(g_ok, "SettlerTurnHandler handle got invalid state");
    GAME_EXPECT(g_st == &state, "SettlerTurnHandler handle got invalid state");
    UnitAddStruct* unit = unit_at(state, unit_idx);
    GAME_EXPECT(unit != nullptr, "SettlerTurnHandler handle got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "SettlerTurnHandler handle unit has null x");
    const u16 player = unit->m_player_idx;
    GAME_EXPECT(player < g_player_n, "SettlerTurnHandler handle player out of bounds");
    GAME_EXPECT(g_mgrs != nullptr, "SettlerTurnHandler handle missing manager array");
    GAME_EXPECT(g_ord != nullptr, "SettlerTurnHandler handle missing order object");
    PlayerState& ps = state.m_player_states[player];
    ps.m_last_turn_settler_count = static_cast<u16>(ps.m_last_turn_settler_count + 1u);
    if (ps.m_target_settlements == 0) {
        return;
    }
    u16 i = U16_KEY_NULL;
    if (!find_unit_slot(player, unit_idx, &i)) {
        GAME_EXPECT(g_ord_ok, "SettlerTurnHandler handle missing exclusive order generation");
        const u16 s = g_mgrs[player].asgn(state.m_map, *g_ord, player, unit->m_x, unit->m_y);
        if (s == U16_KEY_NULL) {
            return;
        }
        GAME_EXPECT(s < k_slot_n, "SettlerTurnHandler handle assigned slot out of range");
        ps.m_settler_idx[s] = unit_idx;
        StmSlot* sl = slot_at(player, s);
        sl->m_has = 1;
        sl->m_tx = g_mgrs[player].tx(s);
        sl->m_ty = g_mgrs[player].ty(s);
        i = s;
    } else {
        StmSlot* sl = slot_at(player, i);
        if (sl->m_has == 0) {
            return;
        }
    }

    const u8 ev = g_mgrs[player].step(state.m_map, i);
    const u16 nx = g_mgrs[player].x(i);
    const u16 ny = g_mgrs[player].y(i);
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);

    if (ev == SMM_GO) {
        if (nx != unit->m_x || ny != unit->m_y) {
            if (!UnitMovementMng::apply_step(state, key, nx, ny)) {
                g_mgrs[player].drop(i);
                clear_slot(player, i);
                return;
            }
        }
        return;
    }

    if (ev == SMM_FOUND) {
        if (nx != unit->m_x || ny != unit->m_y) {
            if (!UnitMovementMng::apply_step(state, key, nx, ny)) {
                g_mgrs[player].drop(i);
                clear_slot(player, i);
                return;
            }
        }
        const u8 ow = state.m_map.get_civ_owner(nx, ny);
        if (ow != U8_KEY_NULL && ow != static_cast<u8>(player)) {
            clear_slot(player, i);
            return;
        }
        found_city(state, nx, ny, player);
        clear_slot(player, i);
        GAME_EXPECT(UnitMovementMng::destroy_unit(state, key), "SettlerTurnHandler handle destroy after found failed");
        return;
    }

    if (ev == SMM_DROP) {
        clear_slot(player, i);
        return;
    }
}

bool SettlerTurnHandler::tgt_xy (u16 player, u16 slot, u16* x, u16* y) {
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
