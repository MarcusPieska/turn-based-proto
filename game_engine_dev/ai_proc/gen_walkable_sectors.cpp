//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_walkable_sectors.h"

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "sector_network.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_walk_land (u8 terr) {
    if (overlay_is_water_terr(terr) || terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0] ||
        terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - GenWalkableSectors -
//================================================================================================================================

GenWalkableSectors::GenWalkableSectors () :
    m_net(nullptr),
    m_map(nullptr),
    m_sec("GenWalkableSectors", "sec", 0u),
    m_sec_n(0),
    m_paint_n(0),
    m_ok(false) {
}

GenWalkableSectors::~GenWalkableSectors () {
}

bool GenWalkableSectors::begin (const SectorNetwork& net, const GameArraySimple& map) {
    m_net = &net;
    m_map = &map;
    m_ok = m_sec.ok() && net.is_valid() && map.width() == m_sec.w() && map.height() == m_sec.h();
    GAME_EXPECT(m_ok, "GenWalkableSectors begin failed");
    m_sec_n = net.sector_n();
    clr();
    return m_ok;
}

void GenWalkableSectors::clr () {
    m_paint_n = 0;
    if (!m_sec.ok()) {
        m_ok = false;
        return;
    }
    std::memset(m_sec.get_iter_ptr(), 0, static_cast<size_t>(WhiteboardMng::tile_n()) * sizeof(u16));
}

bool GenWalkableSectors::ok () const {
    return m_ok;
}

u16 GenWalkableSectors::sector_n () const {
    return m_sec_n;
}

u32 GenWalkableSectors::paint_n () const {
    return m_paint_n;
}

const Whiteboard_2B& GenWalkableSectors::sectors () const {
    return m_sec;
}

bool GenWalkableSectors::build () {
    GAME_EXPECT_RET(m_ok && m_net != nullptr && m_map != nullptr, false, "GenWalkableSectors build not begun");
    clr();
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    u32* q = new u32[n];
    u32 qn = 0;
    for (u16 id = 0; id < m_sec_n; ++id) {
        const Sector* s = m_net->get(id);
        if (s == nullptr) {
            continue;
        }
        if (!is_walk_land(m_map->get_terrain(s->m_x, s->m_y))) {
            continue;
        }
        const u32 ti = static_cast<u32>(s->m_y) * wi + static_cast<u32>(s->m_x);
        if (m_sec.rd_i(ti) != GWS_IDX_NONE) {
            continue;
        }
        const u16 tag = static_cast<u16>(id + 1u);
        m_sec.wr_i(ti, tag);
        q[qn++] = ti;
        ++m_paint_n;
    }
    static const i32 dx4[4] = {-1, 1, 0, 0};
    static const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 sid = m_sec.rd_i(i);
        const u32 py = i / wi;
        const u32 px = i - py * wi;
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + dx4[d];
            const i32 ny = static_cast<i32>(py) + dy4[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const u32 ni = static_cast<u32>(uy) * wi + static_cast<u32>(ux);
            if (m_sec.rd_i(ni) != GWS_IDX_NONE) {
                continue;
            }
            if (!is_walk_land(m_map->get_terrain(ux, uy))) {
                continue;
            }
            m_sec.wr_i(ni, sid);
            q[qn++] = ni;
            ++m_paint_n;
        }
    }
    delete[] q;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
