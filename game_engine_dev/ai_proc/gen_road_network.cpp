//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_road_network.h"

#include "assert_log.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - GenRoadNetwork -
//================================================================================================================================

GenRoadNetwork::GenRoadNetwork () :
    m_map(nullptr),
    m_road("GenRoadNetwork", "road", 0u),
    m_foll("GenRoadNetwork", "foll", 0u),
    m_term_n(0),
    m_road_n(0),
    m_seg_n(0),
    m_ok(false) {
}

GenRoadNetwork::~GenRoadNetwork () {
    clr();
}

void GenRoadNetwork::clr () {
    m_term_n = 0;
    m_road_n = 0;
    m_seg_n = 0;
    m_map = nullptr;
    m_ok = false;
}

bool GenRoadNetwork::ok () const {
    return m_ok && m_road.ok() && m_foll.ok();
}

u32 GenRoadNetwork::term_n () const {
    return m_term_n;
}

u32 GenRoadNetwork::road_n () const {
    return m_road_n;
}

u16 GenRoadNetwork::seg_n () const {
    return m_seg_n;
}

const Whiteboard_2B& GenRoadNetwork::roads () const {
    return m_road;
}

bool GenRoadNetwork::begin (GameArraySimple& map) {
    m_map = &map;
    m_term_n = 0;
    m_road_n = 0;
    m_seg_n = 0;
    m_ok = map.width() > 0u && map.height() > 0u && m_road.ok() && m_foll.ok();
    GAME_EXPECT(m_ok, "GenRoadNetwork begin whiteboard checkout failed");
    return m_ok;
}

//================================================================================================================================
//=> - Impl select -
//================================================================================================================================

#ifndef GEN_ROAD_NETWORK_IMPL
#define GEN_ROAD_NETWORK_IMPL "impl/gen_road_network_impl_mk03.cpp"
#endif

#include GEN_ROAD_NETWORK_IMPL

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
