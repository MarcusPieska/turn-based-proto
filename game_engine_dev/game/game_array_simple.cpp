//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_array_simple.h"
#include "runtime_trace_dbg.h"

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
    return m_tiles[tidx(x, y)].m_terr;
}

u8 GameArraySimple::get_climate (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_clim;
}

u8 GameArraySimple::get_overlay (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_ov;
}

u8 GameArraySimple::get_river (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_riv;
}

u16 GameArraySimple::get_unit_hd (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_unit_hd;
}

u16 GameArraySimple::get_add_idx (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_add_idx;
}

u8 GameArraySimple::get_add_typ (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_add_typ;
}

u16 GameArraySimple::get_res (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_res;
}

u16 GameArraySimple::get_city_worker (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return m_tiles[tidx(x, y)].m_city_worker;
}

u8 GameArraySimple::get_civ_owner (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_civ_owner);
}

u8 GameArraySimple::get_settler_blocked (u16 x, u16 y) const {
    CHECK_MAP_ARRAY_ACCESS((m_w, m_h, x, y));
    return static_cast<u8>(m_tiles[tidx(x, y)].m_settler_blocked);
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
    m_tiles[tidx(x, y)].m_add_idx = add_idx;
    m_tiles[tidx(x, y)].m_add_typ = add_typ;
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

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
