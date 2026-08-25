//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_array_simple.h"
#include "assert_log.h"
#include "build_adds_array.h"
#include "map_overlay_enum.h"
#include "runtime_trace_dbg.h"
#include "add_access_helper.h"
#include "std_add_helper.h"
#include "tile_imp_helper.h"

static bool ov_catalog_ok (u16 ov) {
    if (ov == U16_KEY_NULL) {
        return true;
    }
    return ov <= static_cast<u16>(MapOverlay::Plantation);
}

static bool add_idx_ok (u16 ov, u16 add_idx) {
    return TileImpHelper::add_idx_ok(ov, add_idx);
}

static bool apply_tile_ov (GameTileSimple* t, u16 ov, u16 add_idx) {
    if (t == nullptr || !ov_catalog_ok(ov) || !add_idx_ok(ov, add_idx)) {
        return false;
    }
    t->m_ov = ov;
    t->m_add_idx = add_idx;
    return true;
}

//================================================================================================================================
//=> - GameArraySimple -
//================================================================================================================================

GameArraySimple::GameArraySimple () :
    m_w(0),
    m_h(0),
    m_tiles(nullptr) {
}

GameArraySimple::~GameArraySimple () {
    clear();
}

void GameArraySimple::clear () {
    delete[] m_tiles;
    m_tiles = nullptr;
    m_w = 0;
    m_h = 0;
}

u16 GameArraySimple::width () const {
    return m_w;
}

u16 GameArraySimple::height () const {
    return m_h;
}

u32 GameArraySimple::tile_n () const {
    return static_cast<u32>(m_w) * static_cast<u32>(m_h);
}

u32 GameArraySimple::tidx (u16 x, u16 y) const {
    return static_cast<u32>(y) * static_cast<u32>(m_w) + static_cast<u32>(x);
}

u8 GameArraySimple::get_terrain (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_terr);
}

u8 GameArraySimple::get_climate (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_clim);
}

u16 GameArraySimple::get_overlay (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u16>(m_tiles[tidx(x, y)].m_ov);
}

u8 GameArraySimple::get_river (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_riv);
}

u16 GameArraySimple::get_unit_hd (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u16>(m_tiles[tidx(x, y)].m_unit_hd);
}

u16 GameArraySimple::get_add_idx (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u16>(m_tiles[tidx(x, y)].m_add_idx);
}

u8 GameArraySimple::get_add_typ (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    const u16 ov = static_cast<u16>(m_tiles[tidx(x, y)].m_ov);
    if (ov == static_cast<u16>(MapOverlay::City)) {
        return BUILD_ADD_CITY;
    }
    if (ov == static_cast<u16>(MapOverlay::Mine)) {
        return BUILD_ADD_MINE;
    }
    if (ov == static_cast<u16>(MapOverlay::Plantation)) {
        return BUILD_ADD_PLANTATION;
    }
    if (ov == static_cast<u16>(MapOverlay::Fort)) {
        return BUILD_ADD_FORT;
    }
    if (ov == static_cast<u16>(MapOverlay::Farm) || ov == static_cast<u16>(MapOverlay::Forest)) {
        return BUILD_ADD_STD;
    }
    return 0u;
}

u16 GameArraySimple::get_res (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u16>(m_tiles[tidx(x, y)].m_res);
}

u16 GameArraySimple::get_city_worker (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u16>(m_tiles[tidx(x, y)].m_city_worker);
}

u8 GameArraySimple::get_civ_owner (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_civ_owner);
}

u8 GameArraySimple::get_settler_blocked (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_settler_blocked);
}

u8 GameArraySimple::get_planned_city (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_planned_city);
}

u8 GameArraySimple::get_tile_usage (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_tile_usage);
}

u8 GameArraySimple::get_road_typ (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_road_typ);
}

GameTileSimple* GameArraySimple::tile (u16 x, u16 y) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return &m_tiles[tidx(x, y)];
}

const GameTileSimple* GameArraySimple::tile (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return &m_tiles[tidx(x, y)];
}

bool GameArraySimple::set_unit_hd (u16 x, u16 y, u16 unit_hd) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_unit_hd = unit_hd;
    return true;
}

bool GameArraySimple::set_tile_add (u16 x, u16 y, u16 add_idx, u8 add_typ) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    GameTileSimple* t = &m_tiles[tidx(x, y)];
    if (add_typ == BUILD_ADD_CITY) {
        return apply_tile_ov(t, static_cast<u16>(MapOverlay::City), add_idx);
    }
    if (add_typ == BUILD_ADD_MINE) {
        return apply_tile_ov(t, static_cast<u16>(MapOverlay::Mine), 0u);
    }
    if (add_typ == BUILD_ADD_PLANTATION) {
        return apply_tile_ov(t, static_cast<u16>(MapOverlay::Plantation), 0u);
    }
    if (add_typ == BUILD_ADD_FORT) {
        return apply_tile_ov(t, static_cast<u16>(MapOverlay::Fort), 0u);
    }
    if (add_typ == BUILD_ADD_STD) {
        const u16 idx = (add_idx == U16_KEY_NULL) ? 0u : add_idx;
        return apply_tile_ov(t, static_cast<u16>(MapOverlay::Farm), idx);
    }
    if (!add_idx_ok(static_cast<u16>(t->m_ov), add_idx)) {
        return false;
    }
    t->m_add_idx = add_idx;
    return true;
}

bool GameArraySimple::set_overlay (u16 x, u16 y, u16 ov) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    if (!ov_catalog_ok(ov)) {
        return false;
    }
    GameTileSimple* t = &m_tiles[tidx(x, y)];
    const u16 cur = static_cast<u16>(t->m_ov);
    u16 idx = static_cast<u16>(t->m_add_idx);
    if (ov != cur) {
        idx = AddAccessHelper::empty_add(ov);
    }
    return apply_tile_ov(t, ov, idx);
}

bool GameArraySimple::set_add_idx (u16 x, u16 y, u16 add_idx) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    GameTileSimple* t = &m_tiles[tidx(x, y)];
    if (!add_idx_ok(static_cast<u16>(t->m_ov), add_idx)) {
        return false;
    }
    t->m_add_idx = add_idx;
    return true;
}

bool GameArraySimple::set_road_typ (u16 x, u16 y, u8 road) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_road_typ = road;
    return true;
}

bool GameArraySimple::set_city_worker (u16 x, u16 y, u16 city_idx) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_city_worker = city_idx;
    return true;
}

bool GameArraySimple::set_civ_owner (u16 x, u16 y, u8 owner) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_civ_owner = owner;
    return true;
}

bool GameArraySimple::set_settler_blocked (u16 x, u16 y, u8 blocked) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_settler_blocked = blocked != 0 ? 1u : 0u;
    return true;
}

bool GameArraySimple::set_planned_city (u16 x, u16 y, u8 planned) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_planned_city = planned != 0 ? 1u : 0u;
    return true;
}

bool GameArraySimple::set_tile_usage (u16 x, u16 y, u8 usage) {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    m_tiles[tidx(x, y)].m_tile_usage = static_cast<u64>(usage & 3u);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
