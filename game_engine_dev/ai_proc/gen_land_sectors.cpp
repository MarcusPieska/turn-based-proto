//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_land_sectors.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - GenLandSectors -
//================================================================================================================================

GenLandSectors::GenLandSectors () :
    m_map(nullptr),
    m_sec("GenLandSectors", "sec", 0u),
    m_sec_n(0u),
    m_paint_n(0u),
    m_ok(false) {
}

GenLandSectors::~GenLandSectors () {
}

void GenLandSectors::free_seeds (LandSectorSeeds* seeds) {
    if (seeds == nullptr) {
        return;
    }
    delete[] seeds->m_pts;
    seeds->m_pts = nullptr;
    seeds->m_n = 0u;
}

bool GenLandSectors::begin (const GameArraySimple& map) {
    m_map = &map;
    m_ok = m_sec.ok() && map.width() == m_sec.w() && map.height() == m_sec.h();
    GAME_EXPECT(m_ok, "GenLandSectors begin failed");
    clr();
    return m_ok;
}

void GenLandSectors::clr () {
    m_sec_n = 0u;
    m_paint_n = 0u;
    if (!m_sec.ok()) {
        m_ok = false;
        return;
    }
    std::memset(m_sec.get_iter_ptr(), 0, static_cast<size_t>(WhiteboardMng::tile_n()) * sizeof(u16));
}

bool GenLandSectors::ok () const {
    return m_ok;
}

u16 GenLandSectors::sector_n () const {
    return m_sec_n;
}

u32 GenLandSectors::paint_n () const {
    return m_paint_n;
}

const Whiteboard_2B& GenLandSectors::sectors () const {
    return m_sec;
}

//================================================================================================================================
//=> - Impl select -
//================================================================================================================================

#ifndef GEN_LAND_SECTORS_IMPL
#define GEN_LAND_SECTORS_IMPL "impl/gen_land_sectors_impl_mk01.cpp"
#endif

#include GEN_LAND_SECTORS_IMPL

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
