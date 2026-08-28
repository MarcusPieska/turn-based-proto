//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_bottleneck_markers.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_walkable_sectors.h"
#include "sector_network.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_land_center (const GameArraySimple& map, u16 x, u16 y) {
    const u8 t = map.get_terrain(x, y);
    if (overlay_is_water_terr(t) || t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0] || t == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static bool sector_is_bottleneck (const SectorNetwork& net, const GameArraySimple& map, u16 id) {
    const Sector* s = net.get(id);
    if (s == nullptr || !is_land_center(map, s->m_x, s->m_y)) {
        return false;
    }
    static const u8 ring[6] = {0u, 3u, 5u, 1u, 4u, 2u};
    bool gap[6] = {};
    u32 gn = 0;
    for (u8 b = 0; b < 6u; ++b) {
        const u16 ni = net.nbr(id, b);
        const bool miss = (ni == U16_KEY_NULL);
        const bool open = ((s->m_mask & static_cast<u8>(1u << b)) != 0u);
        if (miss || !open) {
            gap[b] = true;
            ++gn;
        }
    }
    if (gn < 2u) {
        return false;
    }
    if (gn == 6u) {
        return false;
    }
    u32 clust = 0;
    for (u32 i = 0; i < 6u; ++i) {
        const u8 cur = ring[i];
        const u8 prv = ring[(i + 5u) % 6u];
        if (gap[cur] && !gap[prv]) {
            ++clust;
        }
    }
    return clust >= 2u;
}

//================================================================================================================================
//=> - GenBottleneckMarkers -
//================================================================================================================================

GenBottleneckMarkers::GenBottleneckMarkers () :
    m_net(nullptr),
    m_walk(nullptr),
    m_map(nullptr),
    m_mark("GenBottleneckMarkers", "mark", 0u),
    m_mark_n(0),
    m_ok(false) {
}

GenBottleneckMarkers::~GenBottleneckMarkers () {
}

bool GenBottleneckMarkers::begin (
    const SectorNetwork& net,
    const GenWalkableSectors& walk,
    const GameArraySimple& map)
{
    m_net = &net;
    m_walk = &walk;
    m_map = &map;
    m_ok = m_mark.ok() && net.is_valid() && walk.ok() && map.width() == m_mark.w() &&
        map.height() == m_mark.h();
    GAME_EXPECT(m_ok, "GenBottleneckMarkers begin failed");
    clr();
    return m_ok;
}

void GenBottleneckMarkers::clr () {
    m_mark_n = 0;
    if (!m_mark.ok()) {
        m_ok = false;
        return;
    }
    std::memset(m_mark.get_iter_ptr(), 0, static_cast<size_t>(WhiteboardMng::tile_n()) * sizeof(u8));
}

bool GenBottleneckMarkers::ok () const {
    return m_ok;
}

u32 GenBottleneckMarkers::mark_n () const {
    return m_mark_n;
}

const Whiteboard_1B& GenBottleneckMarkers::marks () const {
    return m_mark;
}

bool GenBottleneckMarkers::mark () {
    GAME_EXPECT_RET(m_ok && m_net != nullptr && m_walk != nullptr && m_map != nullptr, false,
        "GenBottleneckMarkers mark not begun");
    clr();
    const u16 sn = m_net->sector_n();
    u8* hit = new u8[sn];
    std::memset(hit, 0, static_cast<size_t>(sn));
    for (u16 id = 0; id < sn; ++id) {
        if (!sector_is_bottleneck(*m_net, *m_map, id)) {
            continue;
        }
        hit[id] = 1u;
        ++m_mark_n;
    }
    const Whiteboard_2B& sec = m_walk->sectors();
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 tag = sec.rd(x, y);
            if (tag == GWS_IDX_NONE) {
                continue;
            }
            const u16 id = static_cast<u16>(tag - 1u);
            if (id >= sn || hit[id] == 0u) {
                continue;
            }
            m_mark.wr(x, y, 1u);
        }
    }
    delete[] hit;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
