//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "unit_movement_mng.h"

#include "civ_relations.h"
#include "unit_add_vector.h"
#include "civ_static_key.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "mvt_cost_static_data.h"
#include "res_placement_defs.h"
#include "runtime_statics.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "unit_type_action_map.h"

#include <cstring>

//================================================================================================================================
//=> - Mvt cost tables -
//================================================================================================================================

static const u8 k_mvt_kind_terr = 0u;
static const u8 k_mvt_kind_clim = 1u;
static const u8 k_mvt_kind_ov = 2u;
static const u8 k_mvt_kind_riv = 3u;
static const u8 k_mvt_kind_road = 4u;

static u16* g_terr_cost = nullptr;
static u16* g_clim_cost = nullptr;
static u16* g_ov_cost = nullptr;
static u16 g_terr_n = 0u;
static u16 g_clim_n = 0u;
static u16 g_ov_n = 0u;
static u16 g_riv_cost = 0u;
static u16 g_road_cost = 0u;
static bool g_mvt_ready = false;
static u16 g_mvt_mapped_n = 0u;
static u16 g_mvt_cost_n = 0u;

static const u16 k_act_is_land = 0u;
static const u16 k_act_is_sea = 2u;

//================================================================================================================================
//=> - Internal helpers -
//================================================================================================================================

static UnitAddStruct* u_get (GameState& s, UnitAddKey key) {
    return s.m_units.get_unit_add(key);
}

static const UnitAddStruct* u_get (const GameState& s, UnitAddKey key) {
    return s.m_units.get_unit_add(key);
}

static u16 mvt_cost_at (const RuntimeStatics& st, u16 idx) {
    const u16 n = st.mvt_cost().get_item_count();
    if (idx >= n) {
        return 0u;
    }
    return st.mvt_cost().get_item(MvtCostStaticDataKey::from_raw(idx)).cost;
}

static bool resolve_mvt_name (cstr name, u8* out_id, u8* out_kind) {
    if (name == nullptr || out_id == nullptr || out_kind == nullptr) {
        return false;
    }
    if (std::strcmp(name, "TERR_NONE") == 0) {
        *out_id = TERR_NONE[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "TERR_OCEAN") == 0) {
        *out_id = TERR_OCEAN[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "TERR_SEA") == 0) {
        *out_id = TERR_SEA[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "TERR_COASTAL") == 0) {
        *out_id = TERR_COASTAL[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "TERR_PLAINS") == 0) {
        *out_id = TERR_PLAINS[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "TERR_HILLS") == 0) {
        *out_id = TERR_HILLS[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "TERR_MOUNTAINS") == 0) {
        *out_id = TERR_MOUNTAINS[0];
        *out_kind = k_mvt_kind_terr;
        return true;
    }
    if (std::strcmp(name, "CLIMATE_NONE") == 0) {
        *out_id = static_cast<u8>(CLIMATE_NONE);
        *out_kind = k_mvt_kind_clim;
        return true;
    }
    if (std::strcmp(name, "CLIMATE_GRASSLAND") == 0) {
        *out_id = static_cast<u8>(CLIMATE_GRASSLAND);
        *out_kind = k_mvt_kind_clim;
        return true;
    }
    if (std::strcmp(name, "CLIMATE_PLAINS") == 0) {
        *out_id = static_cast<u8>(CLIMATE_PLAINS);
        *out_kind = k_mvt_kind_clim;
        return true;
    }
    if (std::strcmp(name, "CLIMATE_DESERT") == 0) {
        *out_id = static_cast<u8>(CLIMATE_DESERT);
        *out_kind = k_mvt_kind_clim;
        return true;
    }
    if (std::strcmp(name, "CLIMATE_TUNDRA") == 0) {
        *out_id = static_cast<u8>(CLIMATE_TUNDRA);
        *out_kind = k_mvt_kind_clim;
        return true;
    }
    if (std::strcmp(name, "OVERLAY_NONE") == 0) {
        *out_id = static_cast<u8>(OVERLAY_NONE);
        *out_kind = k_mvt_kind_ov;
        return true;
    }
    if (std::strcmp(name, "NO_OVERLAYS") == 0) {
        *out_id = static_cast<u8>(OVERLAY_NONE);
        *out_kind = k_mvt_kind_ov;
        return true;
    }
    if (std::strcmp(name, "OVERLAY_SWAMPS") == 0) {
        *out_id = static_cast<u8>(RES_OV_SWAMPS);
        *out_kind = k_mvt_kind_ov;
        return true;
    }
    if (std::strcmp(name, "OVERLAY_FORESTS") == 0) {
        *out_id = static_cast<u8>(RES_OV_FORESTS);
        *out_kind = k_mvt_kind_ov;
        return true;
    }
    if (std::strcmp(name, "OVERLAY_JUNGLES") == 0) {
        *out_id = static_cast<u8>(RES_OV_JUNGLES);
        *out_kind = k_mvt_kind_ov;
        return true;
    }
    if (std::strcmp(name, "OVERLAY_RIVERS") == 0) {
        *out_id = 0u;
        *out_kind = k_mvt_kind_riv;
        return true;
    }
    if (std::strcmp(name, "OVERLAY_ROADS") == 0) {
        *out_id = 0u;
        *out_kind = k_mvt_kind_road;
        return true;
    }
    return false;
}

static u16 mvt_lookup (const u16* tbl, u16 n, u8 id) {
    if (tbl == nullptr || id >= n) {
        return 0u;
    }
    return tbl[id];
}

static void mvt_free_tables () {
    delete[] g_terr_cost;
    delete[] g_clim_cost;
    delete[] g_ov_cost;
    g_terr_cost = nullptr;
    g_clim_cost = nullptr;
    g_ov_cost = nullptr;
    g_terr_n = 0u;
    g_clim_n = 0u;
    g_ov_n = 0u;
}

static u32 mvt_heap_sz () {
    return static_cast<u32>(g_terr_n) * 2u
        + static_cast<u32>(g_clim_n) * 2u
        + static_cast<u32>(g_ov_n) * 2u;
}

static bool mvt_pass_scan (const RuntimeStatics& st, u8* max_terr, u8* max_clim, u8* max_ov, u16* mapped_n) {
    *max_terr = 0u;
    *max_clim = 0u;
    *max_ov = 0u;
    *mapped_n = 0u;
    const u16 n = st.mvt_cost().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.mvt_cost().get_name(MvtCostStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            continue;
        }
        u8 gid = 0u;
        u8 kind = 0u;
        if (!resolve_mvt_name(nm, &gid, &kind)) {
            continue;
        }
        *mapped_n = static_cast<u16>(*mapped_n + 1u);
        if (kind == k_mvt_kind_terr && gid > *max_terr) {
            *max_terr = gid;
        } else if (kind == k_mvt_kind_clim && gid > *max_clim) {
            *max_clim = gid;
        } else if (kind == k_mvt_kind_ov && gid > *max_ov) {
            *max_ov = gid;
        }
    }
    return *mapped_n == n;
}

static bool mvt_pass_fill (const RuntimeStatics& st) {
    const u16 n = st.mvt_cost().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.mvt_cost().get_name(MvtCostStaticDataKey::from_raw(i));
        if (nm == nullptr) {
            continue;
        }
        u8 gid = 0u;
        u8 kind = 0u;
        if (!resolve_mvt_name(nm, &gid, &kind)) {
            continue;
        }
        const u16 cost = mvt_cost_at(st, i);
        if (kind == k_mvt_kind_terr) {
            g_terr_cost[gid] = cost;
        } else if (kind == k_mvt_kind_clim) {
            g_clim_cost[gid] = cost;
        } else if (kind == k_mvt_kind_ov) {
            g_ov_cost[gid] = cost;
        } else if (kind == k_mvt_kind_riv) {
            g_riv_cost = cost;
        } else if (kind == k_mvt_kind_road) {
            g_road_cost = cost;
        }
    }
    return true;
}

static bool tile_has_river (const GameState& s, u16 x, u16 y) {
    return s.m_map.get_river(x, y) != 0u;
}

static bool is_water_terr (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
}

static bool is_inland_wtr_terr (u8 terr) {
    return terr == TERR_INLAND_SEA[0] || terr == TERR_INLAND_LAKE[0];
}

static bool is_mountain_terr (u8 terr) {
    return terr == TERR_MOUNTAINS[0];
}

static u16 unit_type_of (const GameState& s, UnitAddKey key) {
    const UnitAddStruct* u = u_get(s, key);
    if (u == nullptr || s.m_statics == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 unit_n = s.m_statics->unit().get_item_count();
    if (u->m_unit_typ_idx >= unit_n) {
        return U16_KEY_NULL;
    }
    return s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
}

static bool unit_is_land (const GameState& s, u16 unit_type_idx) {
    if (s.m_statics == nullptr || unit_type_idx == U16_KEY_NULL) {
        return false;
    }
    return s.m_statics->unit_type_action_map().unit_type_can_do(unit_type_idx, k_act_is_land);
}

static bool unit_is_sea (const GameState& s, u16 unit_type_idx) {
    if (s.m_statics == nullptr || unit_type_idx == U16_KEY_NULL) {
        return false;
    }
    return s.m_statics->unit_type_action_map().unit_type_can_do(unit_type_idx, k_act_is_sea);
}

static bool unit_is_land_scout (const GameState& s, u16 unit_type_idx) {
    if (s.m_statics == nullptr || unit_type_idx == U16_KEY_NULL) {
        return false;
    }
    cstr nm = s.m_statics->unit_type().get_name(UnitTypeStaticDataKey::from_raw(unit_type_idx));
    return nm != nullptr && std::strcmp(nm, "LAND_SCOUT") == 0;
}

static bool dest_allows_unit_domain (const GameState& s, UnitAddKey key, u8 dest_terr) {
    if (is_mountain_terr(dest_terr)) {
        return false;
    }
    const u16 ut = unit_type_of(s, key);
    if (ut == U16_KEY_NULL) {
        return false;
    }
    if (unit_is_land(s, ut) && is_water_terr(dest_terr)) {
        return false;
    }
    if (unit_is_land(s, ut) && is_inland_wtr_terr(dest_terr) && !unit_is_land_scout(s, ut)) {
        return false;
    }
    if (unit_is_sea(s, ut) && !is_water_terr(dest_terr)) {
        return false;
    }
    return true;
}

static bool in_bounds (const GameState& s, u16 x, u16 y) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    return w > 0 && h > 0 && x < w && y < h;
}

static bool is_grp_tail (const UnitAddStruct& u) {
    return u.m_x == U16_KEY_NULL && u.m_y == U16_KEY_NULL;
}

static bool has_grp_followers (const UnitAddStruct& u) {
    return u.m_next_unit_in_group != U16_KEY_NULL;
}

static CivStaticDataKey seat_civ (const GameState& s, u16 seat) {
    if (s.m_player_states == nullptr || seat >= s.m_player_n) {
        return CivStaticDataKey::None();
    }
    return CivStaticDataKey::from_raw(s.m_player_states[seat].m_civ_index);
}

static bool seats_ally (const GameState& s, u16 seat_a, u16 seat_b) {
    if (seat_a == seat_b) {
        return true;
    }
    const CivStaticDataKey ca = seat_civ(s, seat_a);
    const CivStaticDataKey cb = seat_civ(s, seat_b);
    if (!ca.is_valid() || !cb.is_valid()) {
        return false;
    }
    const CivRel rel = s.m_civ_relations.get(ca, cb);
    return rel == CivRel::CIV_REL_ALLY || rel == CivRel::CIV_REL_SUBJECT;
}

static bool dest_allows_entry (const GameState& s, u16 mover_seat, u16 dest_x, u16 dest_y, bool grp_move) {
    const u16 hd = s.m_map.get_unit_hd(dest_x, dest_y);
    if (hd == U16_KEY_NULL) {
        return true;
    }
    if (grp_move) {
        return false;
    }
    const UnitAddStruct* du = u_get(s, UnitAddKey::from_raw(hd));
    if (du == nullptr) {
        return false;
    }
    return seats_ally(s, mover_seat, du->m_player_idx);
}

static void grp_decr_mvt (GameState& s, UnitAddKey key, i16 cost) {
    UnitAddStruct* u = u_get(s, key);
    while (u != nullptr) {
        u->m_mvt_points = static_cast<i16>(u->m_mvt_points - cost);
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        u = u_get(s, UnitAddKey::from_raw(u->m_next_unit_in_group));
    }
}

static i16 grp_min_mvt_walk (const GameState& s, UnitAddKey key) {
    const UnitAddStruct* u = u_get(s, key);
    if (u == nullptr) {
        return 0;
    }
    i16 mn = u->m_mvt_points;
    UnitAddKey cur = key;
    while (true) {
        const UnitAddStruct* cu = u_get(s, cur);
        if (cu == nullptr) {
            break;
        }
        if (cu->m_mvt_points < mn) {
            mn = cu->m_mvt_points;
        }
        if (cu->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(cu->m_next_unit_in_group);
    }
    return mn;
}

static void tile_stack_unlink (GameState& s, UnitAddKey key) {
    UnitAddStruct* u = u_get(s, key);
    if (u == nullptr || is_grp_tail(*u)) {
        return;
    }
    const u16 ox = u->m_x;
    const u16 oy = u->m_y;
    if (!in_bounds(s, ox, oy)) {
        u->m_next_unit_on_tile = U16_KEY_NULL;
        return;
    }
    const u16 hd = s.m_map.get_unit_hd(ox, oy);
    if (hd == key.value()) {
        if (u->m_next_unit_on_tile != U16_KEY_NULL) {
            s.m_map.set_unit_hd(ox, oy, u->m_next_unit_on_tile);
        } else {
            s.m_map.set_unit_hd(ox, oy, U16_KEY_NULL);
        }
    } else {
        UnitAddKey cur = UnitAddKey::from_raw(hd);
        while (cur.is_valid()) {
            UnitAddStruct* cu = u_get(s, cur);
            if (cu == nullptr) {
                break;
            }
            if (cu->m_next_unit_on_tile == key.value()) {
                cu->m_next_unit_on_tile = u->m_next_unit_on_tile;
                break;
            }
            if (cu->m_next_unit_on_tile == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(cu->m_next_unit_on_tile);
        }
    }
    u->m_next_unit_on_tile = U16_KEY_NULL;
}

static bool tile_stack_append (GameState& s, UnitAddKey key, u16 x, u16 y) {
    UnitAddStruct* u = u_get(s, key);
    if (u == nullptr) {
        return false;
    }
    u->m_x = x;
    u->m_y = y;
    u->m_next_unit_on_tile = U16_KEY_NULL;
    const u16 hd = s.m_map.get_unit_hd(x, y);
    if (hd == U16_KEY_NULL) {
        return s.m_map.set_unit_hd(x, y, key.value());
    }
    UnitAddKey cur = UnitAddKey::from_raw(hd);
    while (cur.is_valid()) {
        UnitAddStruct* cu = u_get(s, cur);
        if (cu == nullptr) {
            return false;
        }
        if (cu->m_next_unit_on_tile == U16_KEY_NULL) {
            cu->m_next_unit_on_tile = key.value();
            return true;
        }
        cur = UnitAddKey::from_raw(cu->m_next_unit_on_tile);
    }
    return false;
}

static void init_mvt_pts (GameState& s, UnitAddStruct* u) {
    u->m_mvt_points = 0;
    if (s.m_statics == nullptr || u == nullptr) {
        return;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    if (u->m_unit_typ_idx >= typ_n) {
        return;
    }
    const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * PATH_MP_TURN);
}

static UnitAddKey grp_find_prev (const GameState& s, UnitAddKey tail) {
    for (u16 pg = 0; pg < UnitAddVector::MAX_PAGES; ++pg) {
        if (s.m_units.get_page(pg) == nullptr) {
            continue;
        }
        for (u16 sl = 0; sl < UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE; ++sl) {
            const u16 idx = static_cast<u16>(pg * UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE + sl);
            const UnitAddKey k = UnitAddKey::from_raw(idx);
            const UnitAddStruct* u = u_get(s, k);
            if (u != nullptr && u->m_next_unit_in_group == tail.value()) {
                return k;
            }
        }
    }
    return UnitAddKey::None();
}

static void tile_stack_remove (GameState& s, UnitAddKey key) {
    tile_stack_unlink(s, key);
}

static bool tile_has_road (const GameState& s, u16 x, u16 y) {
    (void)s;
    (void)x;
    (void)y;
    return false;
}

static i16 pair_transport_cost (const GameState& s, u16 fx, u16 fy, u16 tx, u16 ty) {
    i16 best = 0;
    bool hit = false;
    const bool fr = tile_has_river(s, fx, fy);
    const bool tr = tile_has_river(s, tx, ty);
    if (fr && tr && g_riv_cost > 0u) {
        best = static_cast<i16>(g_riv_cost);
        hit = true;
    }
    const bool frd = tile_has_road(s, fx, fy);
    const bool trd = tile_has_road(s, tx, ty);
    if (frd && trd && g_road_cost > 0u) {
        const i16 ic = static_cast<i16>(g_road_cost);
        if (!hit || ic < best) {
            best = ic;
            hit = true;
        }
    }
    return hit ? best : 0;
}

static i16 land_tile_cost (const GameState& s, u16 x, u16 y) {
    if (!g_mvt_ready) {
        return 0;
    }
    const u8 terr = s.m_map.get_terrain(x, y);
    const u16 terr_cost = mvt_lookup(g_terr_cost, g_terr_n, terr);
    if (terr_cost == 0u) {
        return 0;
    }
    i16 sum = static_cast<i16>(terr_cost);
    const u16 clim_cost = mvt_lookup(g_clim_cost, g_clim_n, s.m_map.get_climate(x, y));
    if (clim_cost > 0u) {
        sum = static_cast<i16>(sum + static_cast<i16>(clim_cost));
    }
    const u16 ov_cost = mvt_lookup(g_ov_cost, g_ov_n, s.m_map.get_overlay(x, y));
    if (ov_cost > 0u) {
        sum = static_cast<i16>(sum + static_cast<i16>(ov_cost));
    }
    return sum;
}

//================================================================================================================================
//=> - UnitMovementMng -
//================================================================================================================================

bool UnitMovementMng::setup_mvt_costs (const RuntimeStatics& st) {
    mvt_free_tables();
    g_mvt_ready = false;
    g_mvt_mapped_n = 0u;
    g_riv_cost = 0u;
    g_road_cost = 0u;
    g_mvt_cost_n = st.mvt_cost().get_item_count();
    u8 max_terr = 0u;
    u8 max_clim = 0u;
    u8 max_ov = 0u;
    if (!mvt_pass_scan(st, &max_terr, &max_clim, &max_ov, &g_mvt_mapped_n)) {
        return false;
    }
    g_terr_n = static_cast<u16>(max_terr + 1u);
    g_clim_n = static_cast<u16>(max_clim + 1u);
    g_ov_n = static_cast<u16>(max_ov + 1u);
    if (g_terr_n > 0u) {
        g_terr_cost = new u16[g_terr_n]();
    }
    if (g_clim_n > 0u) {
        g_clim_cost = new u16[g_clim_n]();
    }
    if (g_ov_n > 0u) {
        g_ov_cost = new u16[g_ov_n]();
    }
    mvt_pass_fill(st);
    g_mvt_ready = true;
    return true;
}

bool UnitMovementMng::mvt_ready () {
    return g_mvt_ready;
}

u16 UnitMovementMng::mvt_mapped_count () {
    return g_mvt_mapped_n;
}

u16 UnitMovementMng::mvt_cost_count () {
    return g_mvt_cost_n;
}

void UnitMovementMng::mvt_tables (UnitMovementMngMvtTbl* out) {
    if (out == nullptr) {
        return;
    }
    out->m_terr_n = g_terr_n;
    out->m_clim_n = g_clim_n;
    out->m_ov_n = g_ov_n;
    out->m_riv = g_riv_cost;
    out->m_road = g_road_cost;
    out->m_bytes = mvt_heap_sz();
}

u32 UnitMovementMng::mvt_heap_bytes () {
    return mvt_heap_sz();
}

u16 UnitMovementMng::mvt_cost_terr (u8 id) {
    return mvt_lookup(g_terr_cost, g_terr_n, id);
}

u16 UnitMovementMng::mvt_cost_clim (u8 id) {
    return mvt_lookup(g_clim_cost, g_clim_n, id);
}

u16 UnitMovementMng::mvt_cost_ov (u8 id) {
    return mvt_lookup(g_ov_cost, g_ov_n, id);
}

bool UnitMovementMng::map_mvt_cost_name (cstr name, u8* out_id, u8* out_kind) {
    return resolve_mvt_name(name, out_id, out_kind);
}

i16 UnitMovementMng::tile_cost (const GameState& s, u16 from_x, u16 from_y, u16 to_x, u16 to_y) {
    if (!in_bounds(s, from_x, from_y) || !in_bounds(s, to_x, to_y)) {
        return 0;
    }
    const i16 transport = pair_transport_cost(s, from_x, from_y, to_x, to_y);
    if (transport > 0) {
        return transport;
    }
    return land_tile_cost(s, to_x, to_y);
}

i16 UnitMovementMng::grp_min_mvt (const GameState& s, UnitAddKey key) {
    return grp_min_mvt_walk(s, key);
}

bool UnitMovementMng::can_step (const GameState& s, UnitAddKey key, u16 dest_x, u16 dest_y, i16* out_cost) {
    const UnitAddStruct* u = u_get(s, key);
    if (u == nullptr || is_grp_tail(*u)) {
        return false;
    }
    if (!in_bounds(s, dest_x, dest_y)) {
        return false;
    }
    if (u->m_x == dest_x && u->m_y == dest_y) {
        return false;
    }
    const u8 dest_terr = s.m_map.get_terrain(dest_x, dest_y);
    if (!dest_allows_unit_domain(s, key, dest_terr)) {
        return false;
    }
    const i16 cost = tile_cost(s, u->m_x, u->m_y, dest_x, dest_y);
    if (cost <= 0) {
        return false;
    }
    if (grp_min_mvt_walk(s, key) <= 0) {
        return false;
    }
    const bool grp_move = has_grp_followers(*u);
    if (!dest_allows_entry(s, u->m_player_idx, dest_x, dest_y, grp_move)) {
        return false;
    }
    if (out_cost != nullptr) {
        *out_cost = cost;
    }
    return true;
}

bool UnitMovementMng::apply_step (GameState& s, UnitAddKey key, u16 dest_x, u16 dest_y) {
    i16 cost = 0;
    if (!can_step(s, key, dest_x, dest_y, &cost)) {
        return false;
    }
    UnitAddStruct* u = u_get(s, key);
    if (u == nullptr) {
        return false;
    }
    const u16 ox = u->m_x;
    const u16 oy = u->m_y;
    const bool grp_move = has_grp_followers(*u);
    if (grp_move) {
        if (in_bounds(s, ox, oy) && s.m_map.get_unit_hd(ox, oy) == key.value()) {
            s.m_map.set_unit_hd(ox, oy, U16_KEY_NULL);
        }
        u->m_x = dest_x;
        u->m_y = dest_y;
        s.m_map.set_unit_hd(dest_x, dest_y, key.value());
        grp_decr_mvt(s, key, cost);
        return true;
    }
    tile_stack_unlink(s, key);
    if (!tile_stack_append(s, key, dest_x, dest_y)) {
        tile_stack_append(s, key, ox, oy);
        return false;
    }
    grp_decr_mvt(s, key, cost);
    return true;
}

bool UnitMovementMng::place_on_tile (GameState& s, u16 x, u16 y, u16 player_idx, u16 typ_idx, UnitAddKey* out) {
    if (out == nullptr || s.m_player_n == 0 || player_idx >= s.m_player_n) {
        return false;
    }
    if (!in_bounds(s, x, y)) {
        return false;
    }
    const u16 hd = s.m_map.get_unit_hd(x, y);
    if (hd != U16_KEY_NULL) {
        const UnitAddStruct* eu = u_get(s, UnitAddKey::from_raw(hd));
        if (eu == nullptr || !seats_ally(s, player_idx, eu->m_player_idx)) {
            return false;
        }
    }
    const UnitAddKey key = s.m_units.get_next_new_unit_add_key();
    if (!key.is_valid()) {
        return false;
    }
    UnitAddStruct* unit = u_get(s, key);
    if (unit == nullptr) {
        s.m_units.return_unit_add(key);
        return false;
    }
    unit->m_player_idx = player_idx;
    unit->m_unit_typ_idx = typ_idx;
    unit->m_health = 0;
    unit->m_level = 0;
    unit->m_next_unit_in_group = U16_KEY_NULL;
    init_mvt_pts(s, unit);
    if (!tile_stack_append(s, key, x, y)) {
        s.m_units.return_unit_add(key);
        return false;
    }
    *out = key;
    return true;
}

bool UnitMovementMng::link_group (GameState& s, UnitAddKey head, UnitAddKey tail) {
    UnitAddStruct* hu = u_get(s, head);
    UnitAddStruct* tu = u_get(s, tail);
    if (hu == nullptr || tu == nullptr) {
        return false;
    }
    if (is_grp_tail(*hu) || is_grp_tail(*tu)) {
        return false;
    }
    if (hu->m_x != tu->m_x || hu->m_y != tu->m_y) {
        return false;
    }
    if (tu->m_next_unit_in_group != U16_KEY_NULL) {
        return false;
    }
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        if (cur == tail) {
            return false;
        }
        const UnitAddStruct* cu = u_get(s, cur);
        if (cu == nullptr || cu->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(cu->m_next_unit_in_group);
    }
    UnitAddStruct* end_u = u_get(s, cur);
    if (end_u == nullptr) {
        return false;
    }
    tile_stack_remove(s, tail);
    end_u->m_next_unit_in_group = tail.value();
    tu->m_next_unit_in_group = U16_KEY_NULL;
    tu->m_x = U16_KEY_NULL;
    tu->m_y = U16_KEY_NULL;
    if (s.m_map.get_unit_hd(hu->m_x, hu->m_y) == tail.value()) {
        s.m_map.set_unit_hd(hu->m_x, hu->m_y, head.value());
    }
    return true;
}

bool UnitMovementMng::unlink_group (GameState& s, UnitAddKey tail) {
    UnitAddStruct* tu = u_get(s, tail);
    if (tu == nullptr || !is_grp_tail(*tu)) {
        return false;
    }
    const UnitAddKey prev = grp_find_prev(s, tail);
    if (!prev.is_valid()) {
        return false;
    }
    UnitAddStruct* pu = u_get(s, prev);
    if (pu == nullptr) {
        return false;
    }
    pu->m_next_unit_in_group = tu->m_next_unit_in_group;
    tu->m_next_unit_in_group = U16_KEY_NULL;
    tu->m_x = pu->m_x;
    tu->m_y = pu->m_y;
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
