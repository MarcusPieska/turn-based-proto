//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "gen_watershed.h"

#include <cstring>

#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const i8 k_dx[4] = {0, 1, 0, -1};
static const i8 k_dy[4] = {-1, 0, 1, 0};
static constexpr u16 k_lake_blk = 40u;
static constexpr u8 k_lake_pass = 1u;
static constexpr u8 k_lake_stop = 2u;

static bool is_inland_wtr (u8 t) {
    return t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool is_coast_wtr (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool land_riv (const GameArraySimple& map, u16 x, u16 y) {
    const u8 t = map.get_terrain(x, y);
    return map.get_river(x, y) != 0u && !is_inland_wtr(t) && !is_coast_wtr(t);
}

static void tag_lake (
    const GameArraySimple& map,
    u16 sx,
    u16 sy,
    Whiteboard_1B& tag,
    Whiteboard_4B& lq) {
    if (tag.rd(sx, sy) != 0u) {
        return;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    u32 qh = 0;
    u32 qt = 0;
    u32 n = 0;
    const u32 sidx = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    lq.wr_i(qt++, sidx);
    tag.wr(sx, sy, k_lake_pass);
    ++n;
    while (qh < qt) {
        const u32 i = lq.rd_i(qh++);
        const u32 py = i / static_cast<u32>(w);
        const u32 px = i - py * static_cast<u32>(w);
        for (u8 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(px) + static_cast<i32>(k_dx[d]);
            const i32 ny = static_cast<i32>(py) + static_cast<i32>(k_dy[d]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (tag.rd(ux, uy) != 0u || !is_inland_wtr(map.get_terrain(ux, uy))) {
                continue;
            }
            tag.wr(ux, uy, k_lake_pass);
            ++n;
            lq.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    const u8 cls = n > static_cast<u32>(k_lake_blk) ? k_lake_stop : k_lake_pass;
    if (cls == k_lake_pass) {
        return;
    }
    for (u32 i = 0; i < qt; ++i) {
        const u32 ti = lq.rd_i(i);
        const u32 py = ti / static_cast<u32>(w);
        const u32 px = ti - py * static_cast<u32>(w);
        tag.wr(static_cast<u16>(px), static_cast<u16>(py), k_lake_stop);
    }
}

static bool can_enter (
    const GameArraySimple& map,
    u16 x,
    u16 y,
    Whiteboard_1B& tag,
    Whiteboard_4B& lq) {
    const u8 t = map.get_terrain(x, y);
    if (is_coast_wtr(t)) {
        return false;
    }
    if (is_inland_wtr(t)) {
        tag_lake(map, x, y, tag, lq);
        return tag.rd(x, y) == k_lake_pass;
    }
    return land_riv(map, x, y);
}

//================================================================================================================================
//=> - GenWatershed -
//================================================================================================================================

GenWatershed::GenWatershed ()
    : m_map(nullptr)
    , m_ov("GenWatershed", "ov", 0u)
    , m_riv_n(0)
    , m_ok(false) {
}

GenWatershed::~GenWatershed () {
}

bool GenWatershed::begin (const GameArraySimple& map) {
    m_map = &map;
    m_ok = m_ov.ok() && map.width() == m_ov.w() && map.height() == m_ov.h();
    clr();
    return m_ok;
}

void GenWatershed::clr () {
    m_riv_n = 0;
    if (!m_ov.ok()) {
        m_ok = false;
        return;
    }
    std::memset(m_ov.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
}

bool GenWatershed::ok () const {
    return m_ok;
}

u32 GenWatershed::river_n () const {
    return m_riv_n;
}

const Whiteboard_1B& GenWatershed::overlay () const {
    return m_ov;
}

u32 GenWatershed::fill_rivers (u16 sx, u16 sy) {
    clr();
    if (!m_ok || m_map == nullptr) {
        return 0;
    }
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    if (sx >= w || sy >= h || !land_riv(*m_map, sx, sy)) {
        return 0;
    }
    Whiteboard_4B que("GenWatershed", "que", 0u);
    Whiteboard_4B lq("GenWatershed", "lq", 0u);
    Whiteboard_1B tag("GenWatershed", "tag", 0u);
    Whiteboard_1B vis("GenWatershed", "vis", 0u);
    if (!que.ok() || !lq.ok() || !tag.ok() || !vis.ok()) {
        return 0;
    }
    std::memset(tag.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
    std::memset(vis.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
    u32 qh = 0;
    u32 qt = 0;
    const u32 sidx = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    que.wr_i(qt++, sidx);
    vis.wr(sx, sy, 1u);
    m_ov.wr(sx, sy, 1u);
    m_riv_n = 1u;
    while (qh < qt) {
        const u32 i = que.rd_i(qh++);
        const u32 py = i / static_cast<u32>(w);
        const u32 px = i - py * static_cast<u32>(w);
        for (u8 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(px) + static_cast<i32>(k_dx[d]);
            const i32 ny = static_cast<i32>(py) + static_cast<i32>(k_dy[d]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (vis.rd(ux, uy) != 0u || !can_enter(*m_map, ux, uy, tag, lq)) {
                continue;
            }
            vis.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
            if (land_riv(*m_map, ux, uy) || is_inland_wtr(m_map->get_terrain(ux, uy))) {
                m_ov.wr(ux, uy, 1u);
                ++m_riv_n;
            }
        }
    }
    return m_riv_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
